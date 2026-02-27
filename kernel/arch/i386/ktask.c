#include "ktask.h"
#include "timer.h"
#include "kmalloc.h"
#include <stdio.h>

extern void context_switch(uint32_t* current_esp, uint32_t next_esp);

static ktcb_t* current_ktask = NULL;
static ktcb_t* zombie_list = NULL;
static uint32_t next_ktid = 0;
static int32_t scheduler_trigger_index = -1;

/**
 * cleanup_zombies - Frees memory of tasks that have exited.
 */
static void cleanup_zombies() {
    while (zombie_list) {
        ktcb_t* z = zombie_list;
        zombie_list = z->next;

        if (z->stack_limit) {
            printf("[Scheduler] Cleaning up stack for Task %d\n", z->ktid);
            kfree(z->stack_limit);
        }
        printf("[Scheduler] Cleaning up KTCB for Task %d\n", z->ktid);
        kfree(z);
    }
}

/**
 * ktask_wrapper - A wrapper function that calls the task's entry point
 * and ensures ktask_exit is called when the entry point returns.
 */
static void ktask_wrapper(void (*entry)(void)) {
    entry();
    ktask_exit();
}

void ktask_init_scheduler() {
    ktcb_t* main_ktcb = kmalloc(sizeof(ktcb_t));
    main_ktcb->ktid = next_ktid++;
    main_ktcb->stack_limit = NULL; // Main kernel stack is already allocated
    main_ktcb->next = main_ktcb;
    current_ktask = main_ktcb;
    scheduler_trigger_index = insert_triger(10, &ktask_yield, 0); // Use local ticks

    printf("Scheduler initialized. Main task KTID: %d\n", main_ktcb->ktid);
}

ktcb_t* ktask_create(void (*entry)(void)) {
    cleanup_zombies(); // Opportunity to clean up

    ktcb_t* ktcb = kmalloc(sizeof(ktcb_t));
    if (!ktcb) return NULL;

    // Allocate 4KB stack
    uint32_t stack_size = 4096;
    void* stack = kmalloc(stack_size);
    if (!stack) {
        kfree(ktcb);
        return NULL;
    }

    ktcb->stack_limit = stack;
    ktcb->ktid = next_ktid++;

    // Set up the initial stack frame
    // The stack grows downwards, so start at the top
    uint32_t* stack_ptr = (uint32_t*)((uint32_t)stack + stack_size);

    // Prepare the stack so that context_switch "returns" to ktask_wrapper
    // and ktask_wrapper receives 'entry' as its argument.

    // Note: On i386, arguments are passed on the stack.
    // ktask_wrapper(entry)
    *(--stack_ptr) = (uint32_t)entry;      // Argument to ktask_wrapper
    *(--stack_ptr) = 0;                    // Dummy return address for ktask_wrapper

    *(--stack_ptr) = (uint32_t)ktask_wrapper; // Return address for context_switch

    // Initial values for callee-saved registers popped by context_switch
    *(--stack_ptr) = 0; // EBP
    *(--stack_ptr) = 0; // EBX
    *(--stack_ptr) = 0; // ESI
    *(--stack_ptr) = 0; // EDI

    ktcb->esp = (uint32_t)stack_ptr;

    // Add to the circular ready queue
    ktcb->next = current_ktask->next;
    current_ktask->next = ktcb;

    printf("Task created. KTID: %d, ESP: %x\n", ktcb->ktid, ktcb->esp);

    return ktcb;
}

void ktask_yield() {
    cleanup_zombies(); // Opportunity to clean up

    ktcb_t* last = current_ktask;
    current_ktask = current_ktask->next;

    if (last == current_ktask) {
        return; // Only one task, nothing to switch to
    }

    if (scheduler_trigger_index != -1) {
        set_triger_ticks(scheduler_trigger_index, 0);
    }

    context_switch(&last->esp, current_ktask->esp);
}

void ktask_exit() {
    printf("Task %d exiting...\n", current_ktask->ktid);

    // Remove current_ktask from the circular list
    ktcb_t* prev = current_ktask;
    while (prev->next != current_ktask) {
        prev = prev->next;
    }

    if (prev == current_ktask) {
        // Last task exiting
        printf("No more tasks. Halting system.\n");
        while(1) {
            __asm__ volatile("hlt");
        }
    }

    prev->next = current_ktask->next;
    ktcb_t* exiting_ktask = current_ktask;
    current_ktask = current_ktask->next;

    // Add to zombie list to be cleaned up by the next task that runs
    exiting_ktask->next = zombie_list;
    zombie_list = exiting_ktask;

    uint32_t dummy_esp = 0;
    context_switch(&dummy_esp, current_ktask->esp);
}

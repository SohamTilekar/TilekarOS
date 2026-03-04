#include "task.h"
#include "idt.h"
#include "local_config.h"
#include "timer.h"
#include "kmalloc.h"
#include "utils.h"
#include "gdt.h"
#include <stdio.h>

extern void context_switch(uint32_t** current_esp, uint32_t* next_esp, uint32_t next_cr3, uint32_t intr_num);

extern uint32_t get_cr3(void);



static uint32_t next_tid = 0;
task_t* current_task = NULL;
static task_t* zombie_list = NULL;
static int32_t scheduler_trigger_index = -1;

/**
 * cleanup_zombies - Frees memory of tasks that have exited.
 */
static void cleanup_zombies() {
    while (zombie_list) {
        task_t* z = zombie_list;
        zombie_list = z->next;

        if (z->stack_limit) {
            printf("[Scheduler] Cleaning up stack for Task %d\n", z->id);
            kfree(z->stack_limit);
        }
        printf("[Scheduler] Cleaning up task for Task %d\n", z->id);
        kfree(z);
    }
}

/**
 * task_wrapper - A wrapper function that calls the task's entry point
 * and ensures task_exit is called when the entry point returns.
 */
static void task_wrapper(void (*entry)(void)) {
    if (current_task && current_task->privilege_level == 3) {
        // Switch to user mode via iret
        // We push a fake interrupt frame onto the stack to iret into ring 3
        uint32_t user_esp = (uint32_t)kmalloc(4096) + 4096; // Basic user stack allocation (needs proper paging in future)
        asm volatile(
            "cli \n\t"
            "mov $0x23, %%ax \n\t" // User data segment
            "mov %%ax, %%ds \n\t"
            "mov %%ax, %%es \n\t"
            "mov %%ax, %%fs \n\t"
            "mov %%ax, %%gs \n\t"
            "pushl $0x23 \n\t"     // SS
            "pushl %0 \n\t"        // ESP
            "pushl $0x202 \n\t"    // EFLAGS (Interrupts enabled)
            "pushl $0x1B \n\t"     // CS (User code segment)
            "pushl %1 \n\t"        // EIP (Entry point)
            "iret"
            : : "r"(user_esp), "r"(entry) : "memory", "eax"
        );
        // We will never return here
    } else {
        entry();
    }
    task_exit();
}

void task_init_scheduler() {
    task_t* main_task = kmalloc(sizeof(task_t));
    main_task->id = next_tid++;
    main_task->stack_limit = NULL; // Main kernel stack is already allocated
    main_task->state = TASK_RUNNING;
    main_task->regs = NULL;
    main_task->page_directory = get_cr3();
    main_task->next = main_task;
    main_task->privilege_level = 0;
    current_task = main_task;
    scheduler_trigger_index = insert_triger(100, &task_yield, 0); // Use local ticks

    printf("Scheduler initialized. Main task TID: %d\n", main_task->id);
}

task_t* task_create(void (*entry)(void), uint8_t privilege_level) {
    // Disable interrupts to prevent scheduler from ticking while we modify the ready queue
    uint32_t flags = interrupt_save();
    cleanup_zombies(); // Opportunity to clean up

    task_t* task = kmalloc(sizeof(task_t));
    if (!task) {
        interrupt_restore(flags);
        return NULL;
    }

    // Allocate 4KB stack for kernel-mode execution of the task (even for user tasks, they need a kernel stack for interrupts)
    uint32_t stack_size = 4096;
    void* stack = kmalloc(stack_size);
    if (!stack) {
        kfree(task);
        interrupt_restore(flags);
        return NULL;
    }

    task->stack_limit = stack;
    task->id = next_tid++;
    task->state = TASK_READY;
    task->page_directory = get_cr3(); // Inherit current page directory
    task->privilege_level = privilege_level;

    // Set up the initial stack frame
    // The stack grows downwards, so start at the top
    uint32_t* stack_ptr = (uint32_t*)((uint32_t)stack + stack_size);

    // Prepare the stack so that context_switch (which now returns via iret)
    // properly "returns" to task_wrapper and task_wrapper receives 'entry'.

    // task_wrapper(entry)
    *(--stack_ptr) = (uint32_t)entry;      // Argument to task_wrapper
    *(--stack_ptr) = 0;                    // Dummy return address for task_wrapper

    // Hardware interrupt frame (popped by iret inside context_switch)
    *(--stack_ptr) = 0x202; // EFLAGS (Interrupts enabled)
    *(--stack_ptr) = GDT_KERNEL_CS_OFFSET;  // CS (Kernel Code Segment)
    *(--stack_ptr) = (uint32_t)task_wrapper; // EIP (Return address)

    // pushad registers
    *(--stack_ptr) = 0; // EAX
    *(--stack_ptr) = 0; // ECX
    *(--stack_ptr) = 0; // EDX
    *(--stack_ptr) = 0; // EBX
    *(--stack_ptr) = 0; // ESP (dummy)
    *(--stack_ptr) = 0; // EBP
    *(--stack_ptr) = 0; // ESI
    *(--stack_ptr) = 0; // EDI

    // Segment registers
    *(--stack_ptr) = 0x10; // DS
    *(--stack_ptr) = 0x10; // ES
    *(--stack_ptr) = 0x10; // FS
    *(--stack_ptr) = 0x10; // GS

    task->kernel_stack = stack_ptr;
    task->regs = (registers_t*)stack_ptr;

    // Add to the circular ready queue
    task->next = current_task->next;
    current_task->next = task;

    printf("Task created. TID: %d, Privilege: %d, ESP: %x\n", task->id, task->privilege_level, (uint32_t)task->kernel_stack);

    // We restore interrupts for THIS currently running task that just created a new one
    interrupt_restore(flags);
    return task;
}

void task_yield(InteruptReg *regs) {
    uint32_t flags = interrupt_save();
    cleanup_zombies();

    task_t* last = current_task;
    current_task = current_task->next;

    if (last == current_task) {
        if (regs && regs->intr_num >= 32) {
            pic_send_eoi(regs->intr_num);
            regs->intr_num = 0;
        }
        interrupt_restore(flags);
        return;
    }

    if (scheduler_trigger_index != -1) {
        set_triger_ticks(scheduler_trigger_index, 0);
    }

    last->state = TASK_READY;
    current_task->state = TASK_RUNNING;

    uint32_t intr_num = 0;
    if (regs && regs->intr_num >= 32) {
        intr_num = regs->intr_num;
        // Prevent calling handler from sending EOI again
        regs->intr_num = 0;
    }

    if (current_task->stack_limit) {
        tss_set_kernel_stack((uint32_t)current_task->stack_limit + 4096); // 4096 is stack_size
    }

    context_switch(&last->kernel_stack, current_task->kernel_stack, current_task->page_directory, intr_num);

    interrupt_restore(flags);
}

void task_exit() {
    uint32_t flags = interrupt_save();
    printf("Task %d exiting...\n", current_task->id);

    // Remove current_task from the circular list
    task_t* prev = current_task;
    while (prev->next != current_task) {
        prev = prev->next;
    }

    if (prev == current_task) {
        // Last task exiting
        printf("No more tasks. Halting system.\n");
        while(1) {
            __asm__ volatile("hlt");
        }
    }

    prev->next = current_task->next;
    task_t* exiting_task = current_task;
    current_task = current_task->next;

    // Add to zombie list to be cleaned up by the next task that runs
    exiting_task->next = zombie_list;
    zombie_list = exiting_task;

    current_task->state = TASK_RUNNING;

    if (current_task->stack_limit) {
        tss_set_kernel_stack((uint32_t)current_task->stack_limit + 4096);
    }

    uint32_t* dummy_esp = 0;

    context_switch(&dummy_esp, current_task->kernel_stack, current_task->page_directory, 0);

    // We should never reach here, but in case we do somehow
    interrupt_restore(flags);
}

// ============================================================================
// Faults
// ============================================================================

void protection_fault() {
    printf("Task %d Did Inlegal things as a user process Terminating it\n", current_task->id);
    task_exit();
}

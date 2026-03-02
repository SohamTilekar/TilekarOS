#include "ktask.h"
#include "local_config.h"
#include "timer.h"
#include "kmalloc.h"
#include "utils.h"
#include "idt.h"
#include <stdio.h>

extern void context_switch(uint32_t** current_esp, uint32_t* next_esp, uint32_t next_cr3);

static inline uint32_t get_cr3(void) {
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

static task_t* current_ktask = NULL;
static task_t* zombie_list = NULL;
static uint32_t next_ktid = 0;
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
 * ktask_wrapper - A wrapper function that calls the task's entry point
 * and ensures ktask_exit is called when the entry point returns.
 */
static void ktask_wrapper(void (*entry)(void)) {
    entry();
    ktask_exit();
}

void ktask_init_scheduler() {
    task_t* main_task = kmalloc(sizeof(task_t));
    main_task->id = next_ktid++;
    main_task->stack_limit = NULL; // Main kernel stack is already allocated
    main_task->state = TASK_RUNNING;
    main_task->regs = NULL;
    main_task->page_directory = get_cr3();
    main_task->next = main_task;
    current_ktask = main_task;
    scheduler_trigger_index = insert_triger(100, &ktask_yield, 0); // Use local ticks

    printf("Scheduler initialized. Main task KTID: %d\n", main_task->id);
}

task_t* ktask_create(void (*entry)(void)) {
    // Disable interrupts to prevent scheduler from ticking while we modify the ready queue
    uint32_t flags = interrupt_save();
    cleanup_zombies(); // Opportunity to clean up

    task_t* task = kmalloc(sizeof(task_t));
    if (!task) {
        interrupt_restore(flags);
        return NULL;
    }

    // Allocate 4KB stack
    uint32_t stack_size = 4096;
    void* stack = kmalloc(stack_size);
    if (!stack) {
        kfree(task);
        interrupt_restore(flags);
        return NULL;
    }

    task->stack_limit = stack;
    task->id = next_ktid++;
    task->state = TASK_READY;
    task->page_directory = get_cr3(); // Inherit current page directory

    // Set up the initial stack frame
    // The stack grows downwards, so start at the top
    uint32_t* stack_ptr = (uint32_t*)((uint32_t)stack + stack_size);

    // Prepare the stack so that context_switch (which now returns via iret)
    // properly "returns" to ktask_wrapper and ktask_wrapper receives 'entry'.

    // ktask_wrapper(entry)
    *(--stack_ptr) = (uint32_t)entry;      // Argument to ktask_wrapper
    *(--stack_ptr) = 0;                    // Dummy return address for ktask_wrapper

    // Hardware interrupt frame (popped by iret inside context_switch)
    // 0x202 = (IF) Interrupt Enable Flag set to 1.
    // When context_switch iret's into this task for the very first time,
    // the CPU will load this EFLAGS value, instantly turning interrupts back on!
    *(--stack_ptr) = 0x202; // EFLAGS (Interrupts enabled)
    *(--stack_ptr) = GDT_KERNEL_CS_OFFSET;  // CS (Kernel Code Segment)
    *(--stack_ptr) = (uint32_t)ktask_wrapper; // EIP (Return address)

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
    task->next = current_ktask->next;
    current_ktask->next = task;

    printf("Task created. KTID: %d, ESP: %x\n", task->id, (uint32_t)task->kernel_stack);

    // We restore interrupts for THIS currently running task that just created a new one
    interrupt_restore(flags);
    return task;
}

void ktask_yield(InteruptReg *regs) {
  // 1. Save current EFLAGS and disable interrupts.
  // This EFLAGS value (with IF=1, assuming they were on) is stored in 'flags'.
  uint32_t flags = interrupt_save();
  cleanup_zombies(); // Opportunity to clean up

  task_t *last = current_ktask;
  current_ktask = current_ktask->next;

  if (last == current_ktask) {
    interrupt_restore(flags);
    return; // Only one task, nothing to switch to
  }

  if (scheduler_trigger_index != -1) {
    set_triger_ticks(scheduler_trigger_index, 0);
  }

  last->state = TASK_READY;
  current_ktask->state = TASK_RUNNING;

  // Send EOI if yielding from inside an interrupt handler
  if (regs) {
    pic_send_eoi(regs->intr_num);
  }

  // 2. The context_switch will:
  //    - Push the current CPU state (including EFLAGS where IF=0) onto 'last's
  //    stack.
  //    - Load 'current_ktask's stack pointer.
  //    - Use 'iret' to pop 'current_ktask's previously saved state.
  //    - This 'iret' replaces the CPU's EFLAGS with the one saved by
  //    'current_ktask'
  //      when IT called ktask_yield. (At that time, IF was 0 on the stack
  //      frame).
  context_switch(&last->kernel_stack, current_ktask->kernel_stack,
                 current_ktask->page_directory);

  // 3. We reach here when THIS task is eventually resumed by another task.
  // We restore 'flags' (which has IF=1 from step 1), turning interrupts back
  // on!
  interrupt_restore(flags);
}
void ktask_exit() {
    // 1. Disable interrupts and save state. We don't want to be interrupted
    // while dismantling our own task structure.
    uint32_t flags = interrupt_save();
    printf("Task %d exiting...\n", current_ktask->id);

    // Remove current_ktask from the circular list
    task_t* prev = current_ktask;
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
    task_t* exiting_ktask = current_ktask;
    current_ktask = current_ktask->next;

    // Add to zombie list to be cleaned up by the next task that runs
    exiting_ktask->next = zombie_list;
    zombie_list = exiting_ktask;

    current_ktask->state = TASK_RUNNING;

    uint32_t* dummy_esp = 0;

    // 2. Switch to the next task.
    // We pass a dummy_esp because this task is dead; we don't care about saving its ESP.
    // The next task will resume, and IT will call interrupt_restore() (because it
    // was suspended in ktask_yield), turning interrupts back on for the system.
    context_switch(&dummy_esp, current_ktask->kernel_stack, current_ktask->page_directory);

    // We should never reach here, but in case we do somehow
    interrupt_restore(flags);
}

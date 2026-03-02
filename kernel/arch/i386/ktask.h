#ifndef ARCH_I386_KTASK_H
#define ARCH_I386_KTASK_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_SLEEPING
} task_state_t;

typedef struct {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags;
} registers_t;

typedef struct task {
    uint32_t id;
    uint32_t *kernel_stack;
    registers_t *regs;      // saved context
    task_state_t state;
    uint32_t page_directory; // physical address of CR3

    // Internal scheduler fields
    struct task* next;      // Next task in the ready queue
    void* stack_limit;      // Base of the stack (for freeing)
} task_t;

#include "utils.h"

/**
 * ktask_init_scheduler - Initializes the multitasking system.
 * This should be called once the main kernel execution is ready to be a task.
 */
void ktask_init_scheduler();

/**
 * ktask_create - Creates a new kernel task.
 * @entry: The entry point function for the task.
 *
 * Return: The created task, or NULL on failure.
 */
task_t* ktask_create(void (*entry)(void));

/**
 * ktask_yield - Yields the CPU to the next ready task.
 */
void ktask_yield(InteruptReg *regs);

/**
 * ktask_exit - Terminates the current task.
 */
void ktask_exit();

#endif // ARCH_I386_KTASK_H

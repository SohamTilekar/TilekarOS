#ifndef ARCH_I386_KTASK_H
#define ARCH_I386_KTASK_H

#include <stdint.h>
#include <stddef.h>

typedef struct ktcb {
    uint32_t esp;           // Saved stack pointer
    uint32_t ktid;          // Kernel Task ID
    struct ktcb* next;      // Next task in the ready queue
    void* stack_limit;      // Base of the stack (for freeing)
} ktcb_t;

/**
 * ktask_init_scheduler - Initializes the multitasking system.
 * This should be called once the main kernel execution is ready to be a task.
 */
void ktask_init_scheduler();

/**
 * ktask_create - Creates a new kernel task.
 * @entry: The entry point function for the task.
 *
 * Return: The created KTCB, or NULL on failure.
 */
ktcb_t* ktask_create(void (*entry)(void));

/**
 * ktask_yield - Yields the CPU to the next ready task.
 */
void ktask_yield();

/**
 * ktask_exit - Terminates the current task.
 */
void ktask_exit();

#endif // ARCH_I386_KTASK_H

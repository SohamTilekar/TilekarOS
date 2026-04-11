#ifndef ARCH_I386_TASK_H
#define ARCH_I386_TASK_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../utils/utils.h"

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
    uint8_t privilege_level; // 0 or 3

    // Internal scheduler fields
    struct task* next;      // Next task in the ready queue
    void* stack_limit;      // Base of the stack (for freeing)
} task_t;

extern task_t* current_task;


/**
 * task_init_scheduler - Initializes the multitasking system.
 * This should be called once the main kernel execution is ready to be a task.
 */
void task_init_scheduler();

/**
 * task_create - Creates a new task.
 * @entry: The entry point function for the task.
 * @privilege_level: The privilege level of the task (0 for kernel, 3 for user).
 *
 * Return: The created task, or NULL on failure.
 */
task_t* task_create(void (*entry)(void), uint8_t privilege_level);

/**
 * task_create_user - Creates a new user task by copying a code block.
 * @start_addr: The start address of the code to copy.
 * @end_addr: The end address of the code to copy.
 *
 * Return: The created task, or NULL on failure.
 */
task_t* task_create_user(void* start_addr, void* end_addr);

/**
 * task_create_elf - Creates a new task from an ELF image.
 * @elf_data: Pointer to the ELF image in memory.
 * @privilege_level: The privilege level (0 for kernel, 3 for user).
 *
 * Return: The created task, or NULL on failure.
 */
task_t* task_create_elf(void* elf_data, uint8_t privilege_level);

/**
 * task_create_elf_from_file - Loads an ELF from disk and creates a task.
 * @path: Path to the ELF file.
 * @privilege_level: The privilege level.
 */
task_t* task_create_elf_from_file(const char* path, uint8_t privilege_level);

/**
 * task_yield - Yields the CPU to the next ready task.
 */
void task_yield(InteruptReg *regs);

/**
 * task_exit - Terminates the current task.
 */
void task_exit();

#endif // ARCH_I386_TASK_H

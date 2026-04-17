#ifndef ARCH_I386_TASK_H
#define ARCH_I386_TASK_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../utils/utils.h"
#include "../fs/vfs.h"

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
    uint32_t useresp, ss; // Only if privilege_level == 3
} registers_t;

typedef struct task {
    uint32_t id;
    uint32_t *kernel_stack;
    registers_t *regs;      // saved context
    task_state_t state;
    uint32_t preempt_count;  // scheduler preemption disable nesting
    uint32_t page_directory; // physical address of CR3
    uint8_t privilege_level; // 0 or 3
    uint32_t heap_start;     // lowest valid heap address in userspace
    uint32_t heap_break;     // current program break
    uint32_t heap_mapped_end;// end (exclusive) of heap pages already mapped
    file_t* file_table[MAX_FILES_PER_PROCESS];

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
 * task_fork - Creates a copy of the current process.
 * @regs: The register state of the parent at the time of fork.
 */
task_t* task_fork(InterruptReg_t *regs);

/**
 * task_execve - Replaces the current process with a new one from an ELF.
 * @path: Path to the ELF file.
 * @argv: Argument vector.
 * @envp: Environment vector.
 * @regs: The register state to be modified for the new process.
 */
int task_execve(const char* path, char *const argv[], char *const envp[], InterruptReg_t *regs);

/**
 * task_yield - Yields the CPU to the next ready task.
 */
void task_yield(InterruptReg_t *regs);
void task_block_current();
void task_unblock(task_t* task);

/**
 * task_exit - Terminates the current task.
 */
void task_exit();

void task_stop_scheduler();
void task_start_scheduler();
void task_switch_to(task_t* task);
void task_preempt_disable(void);
void task_preempt_enable(void);

int task_file_table_copy(task_t* dst, const task_t* src);
int task_file_table_set(task_t* task, int fd, const file_t* src_file);

#endif // ARCH_I386_TASK_H

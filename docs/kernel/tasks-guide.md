# Task Management

TilekarOS supports preemptive multitasking for both kernel and user-mode tasks.

## 1. Creating a Kernel Task

Kernel tasks run with full privileges (Ring 0) and share the kernel's address space.

```c
/**
 * task_create - Creates a new kernel task.
 * @entry: The entry point function for the task.
 * @privilege_level: 0 for kernel.
 */
task_t* task = task_create(my_kernel_function, 0);
```

### What happens:
- A unique Task ID (TID) is assigned.
- A 4KB kernel stack is allocated.
- The task is added to the circular ready queue.
- It will begin execution from `my_kernel_function` when scheduled.

---

## 2. Creating a User Task

User tasks run with restricted privileges (Ring 3) in their own isolated virtual address space.

### Why User Tasks?
In TilekarOS, user tasks provide **Fault Isolation** and **Security**:

- **Protection**: A bug in a user task (like a null pointer dereference) will only crash that task, not the whole kernel.
- **Controlled Access**: User tasks must go through a formal **System Call** interface to perform privileged actions like writing to the terminal or managing files.
- **Address Space Isolation**: Each user task gets its own Page Directory, meaning `0x08048000` points to different physical memory for every process.

### Example: Defining and Launching a User Task

1.  **Define the code** (typically in an assembly file):
    ```asm
    _start_user_task:
        ; syscall: sys_write(1, msg, 19)
        mov eax, 1
        mov ebx, 1
        mov ecx, msg
        mov edx, 19
        int 0x80

        ; syscall: sys_exit(0)
        mov eax, 0
        mov ebx, 0
        int 0x80

    msg db 'Hello from Ring 3!', 0xA, 0
    _end_user_task:
    ```

2.  **Launch from the kernel**:
    ```c
    extern char _start_user_task;
    extern char _end_user_task;

    task_t* user_task = task_create_user(&_start_user_task, &_end_user_task);
    ```

### Implementation Details:

- **Isolated Address Space**: `task_create_user` creates a new page directory for the task.
- **Memory Mapping**:
   - The user's code is mapped at **`0x08048000`** in its virtual address space.
   - A 4KB user stack is mapped at **`0xB0000000`**.
- **Privilege Transition**: The kernel prepares a stack frame and uses the `iret` instruction to jump to the code at `0x08048000` while dropping the CPU to Ring 3.
- **Protection**: User tasks cannot execute privileged instructions (like `hlt`) or access kernel memory (`> 0xC0000000`). They must use the `int 0x80` interface for all system services.

---

## 3. Task Lifecycle

- **Ready**: Task is waiting for its turn in the scheduler.
- **Running**: Task currently owns the CPU.
- **Exiting**: When a task returns from its entry point, `task_exit()` is called automatically.
- **Zombie**: Exited tasks remain as "zombies" until the scheduler cleans up their allocated memory (stack and `task_t` structure).

---

## 4. Scheduling

The scheduler uses a **Round-Robin** algorithm triggered by the Timer (PIT) every 10ms.

- **Yielding**: A task can voluntarily give up the CPU by calling `task_yield()`.
- **Preemption**: The timer interrupt forces a context switch if the current task's time slice has expired.

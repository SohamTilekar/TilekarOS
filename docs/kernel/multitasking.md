# Multitasking & Scheduling

TilekarOS supports preemptive multitasking for Ring 0 (Kernel) and Ring 3 (User) tasks.

## 1. Task Structure (`task_t`)

A task tracks:

- **ID**: Unique process identifier (TID).
- **State**: READY, RUNNING, ZOMBIE.
- **Kernel Stack**: Pointer to the current stack pointer of the task.
- **Address Space**: Page Directory physical address (`CR3`).
- **Privilege**: Kernel (0) or User (3).

---

## 2. Task Lifecycle

### Creation (`task_create`)
When a task is created, the kernel:

1.  Allocates a `task_t` and a **4KB kernel stack**.
2.  Prepares the stack frame to simulate an interrupt return (`iret`).
3.  Initializes registers so the task starts execution at `task_wrapper`.

### Execution Wrapper (`task_wrapper`)
Every task begins in `task_wrapper`.

- For **User Tasks**, it handles the transition to Ring 3 using `iret`.
- For **Kernel Tasks**, it simply calls the entry point.
- When the task finishes, it automatically calls `task_exit()`.

### Exiting (`task_exit`)
When a task exits, it is removed from the ready queue and placed into a **Zombie List**.

- **Zombie List**: Exited tasks cannot free their own kernel stack while they are still using it.
- **Delayed Cleanup**: The *next* task to be scheduled calls `cleanup_zombies()`, which performs the actual `kfree` on the exited task's `task_t` and kernel stack.
- **System Halt**: If the last remaining task exits, the kernel enters an infinite `hlt` loop.

---

## 3. Context Switching

A context switch occurs during a timer interrupt (preemption) or a manual `task_yield()`.

### `context_switch(current_esp, next_esp, next_cr3, intr_num)`
The switch is performed in assembly (`task.asm`) and is highly optimized:

1.  **Simulate Interrupt**: Since `context_switch` is a regular call, it pops the return EIP and pushes a "fake" interrupt frame (`EFLAGS`, `CS`, `EIP`) so that it can return via `iret`.
2.  **Save Current Context**: Saves all registers (`pushad`, segment registers) onto the current task's stack.
3.  **Page Directory Switch**: Checks if the `next_cr3` is different from the current one. If so, it loads the new `CR3`. This avoids unnecessary TLB flushes if switching between threads in the same process.
4.  **Stack Switch**: Updates the current task's `task_t` with its current `ESP` and then loads the `next_esp` into the `ESP` register.
5.  **PIC EOI**: If the switch was triggered by a hardware interrupt (`intr_num >= 32`), it calls `pic_send_eoi` before restoring the next task's state.
6.  **Restore Context**: Pops the segment and general-purpose registers of the new task.
7.  **Return**: Executes `iret`, which jumps to the next task's saved `EIP` and restores its `EFLAGS`.

---

## 4. User Mode (Ring 3)

The kernel transitions to User Mode using the `iret` instruction within `task_wrapper` (for new tasks) or when returning from a syscall/interrupt.

### Ring 3 Transition Logic:

- **TSS**: The global Task State Segment is updated with the task's `esp0` (top of its kernel stack) during every context switch. This allows the CPU to find the kernel stack when an interrupt occurs in Ring 3.
- **User Stack**: For user tasks, a separate stack is mapped at `0xB0000000` in their private address space.
- **The Jump**:
    1.  Sets segment registers (`DS`, `ES`, `FS`, `GS`) to the user data selector (`0x23`).
    2.  Pushes the User Stack Segment (`0x23`), the User Stack Pointer, `EFLAGS` (with interrupts enabled), the User Code Segment (`0x1B`), and the entry point `EIP`.
    3.  Executes `iret`.

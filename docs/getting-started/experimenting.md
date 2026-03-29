# Experimenting with TilekarOS

TilekarOS is designed to be a playground for learning OS internals. This guide shows you how to "mess" with the system, create tasks, and intentionally trigger failures to see how the kernel responds.

## 1. Creating Kernel Tasks (Ring 0)

Kernel tasks run with full privileges. They are easy to create and can access any part of the hardware.

### How to add a task:
1.  **Define a function** in `kernel/kernel.c`:
    ```c
    void my_experiment_task() {
        while (true) {
            printf("[Experiment] Running...\n");
            for (volatile int i = 0; i < 1000000; i++); // Busy wait
            task_yield(NULL); // Give up CPU
        }
    }
    ```
2.  **Register it** in `kernel_main()`:
    ```c
    task_create(my_experiment_task, 0);
    ```

### Voluntary vs. Preemptive:
- **`task_yield(NULL)`**: Manually tells the scheduler to pick the next task.
- **Preemption**: If you don't call yield, the **Timer Interrupt** (IRQ 0) will eventually force a context switch after its time slice.

---

## 2. Creating User Tasks (Ring 3)

User tasks are isolated and run in their own virtual address space. They cannot access kernel memory or execute privileged instructions.

### The Lifecycle of a User Task
In TilekarOS, a user task is typically defined as a block of code (assembly or C) that is copied into a new address space.

#### Step 1: Define the User Code
User tasks use **System Calls** to interact with the kernel. See [user_process.asm](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/user_process.asm){: target="_blank" } for a complete example of a user task using syscalls to list directories.

??? example "Code Preview: `user_process.asm`"
    ```nasm
    --8<-- "kernel/user_process.asm"
    ```

#### Step 2: Launch the Task
In `kernel_main`, use the `extern` symbols to pass the code block to `task_create_user`:
```c
extern char _start_user_task;
extern char _end_user_task;

task_create_user(&_start_user_task, &_end_user_task);
```

### What happens under the hood?
1.  **Memory Mapping**: The code block is copied to a new page directory starting at **`0x08048000`**.
2.  **Stack Setup**: A user-mode stack is automatically mapped at **`0xB0000000`**.
3.  **Ring Transition**: The kernel uses the `iret` instruction to drop the CPU's privilege level to Ring 3.
4.  **TSS Update**: The Task State Segment (TSS) is updated with the task's kernel stack pointer (`esp0`) so the CPU can transition back to Ring 0 during interrupts or syscalls.

---

## 3. How to Make Things Fail

Testing the kernel's robustness is part of the fun. Here is how you can trigger common CPU exceptions.

### A. Division by Zero (ISR 0)
Add this to any task:
```c
volatile int a = 5;
volatile int b = 0;
volatile int c = a / b;
```

### B. Page Fault (ISR 14)
- **From Kernel**: `*(int*)0x0 = 123;` (Accessing NULL).
- **From User**: Trying to read kernel memory (addresses above `0xC0000000`).

### C. General Protection Fault (ISR 13)
Triggered in User Mode if you try to execute privileged instructions:
```c
// Inside a User Task
asm volatile("hlt"); // Only allowed in Ring 0
```

---

## 4. Debugging Tips

### Headless Logging (Serial COM1)
TilekarOS mirrors all `printf` output to the serial port `0x3F8`. Add `-serial stdio` to QEMU to see these logs in your terminal.

---

## References
- [OSDev: Testing](https://wiki.osdev.org/Testing)
- [OSDev: Serial Ports](https://wiki.osdev.org/Serial_Ports)
- [Wikipedia: Ring (computer_security)](https://en.wikipedia.org/wiki/Protection_ring)

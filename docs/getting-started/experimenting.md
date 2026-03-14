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
- **Preemption**: If you don't call yield, the **Timer Interrupt** will eventually force a context switch after 10ms.

---

## 2. Creating User Tasks (Ring 3)

User tasks are isolated and run in their own virtual address space. They cannot access kernel memory or execute privileged instructions.

### The Lifecycle of a User Task
In TilekarOS, a user task is typically defined as a block of code (assembly or C) that is copied into a new address space.

#### Step 1: Define the User Code
User tasks use **System Calls** to interact with the kernel. For example, to print "Hello from Ring 3" and exit:

```asm
[bits 32]

section .text

global _start_user_task
global _end_user_task

_start_user_task:
    ; SYS_WRITE (1), STDOUT (1)
    mov eax, 1
    mov ebx, 1
    mov ecx, msg
    mov edx, 19
    int 0x80

    ; SYS_EXIT (0)
    mov eax, 0
    xor ebx, ebx
    int 0x80

msg db 'Hello from Ring 3!', 0xA, 0
_end_user_task:
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

### Technical Deep Dive: The "Why" behind the implementation

You might wonder why we use specific addresses or why we copy code around. Here is the technical rationale:

- **Why `extern char`?**: Since TilekarOS doesn't have a filesystem yet, we "embed" user programs inside the kernel binary. These symbols tell the kernel exactly where that embedded code starts and ends.
- **Why Copying?**: The CPU prevents Ring 3 (User Mode) from accessing any memory marked as "Supervisor." Because the kernel is mapped at `0xC0000000`, the user task cannot "reach" its own code if it stays in the kernel region. We must copy it to a User-accessible page.
- **Why `0x08048000`?**: This is the standard base address for 32-bit x86 executables (ELF). Using it maintains compatibility with traditional toolchains.
- **Why `0xB0000000`?**: We want the stack to be as far away from the code as possible to allow both to grow. This address is high in the user's 3GB space but leaves enough room before hitting the 3GB Kernel boundary.
- **Why `iret`?**: The CPU hardware forbids jumping from Ring 0 to Ring 3. The only way to "drop" privilege is to simulate returning from an interrupt. We push a "fake" user-mode state onto the stack and execute `iret`, which tricks the CPU into switching to Ring 3.

---

## 4. Ring 3 Pitfalls & Restrictions

When experimenting in Ring 3, remember these rules:

- **No Direct I/O**: `outb`/`inb` will trigger a General Protection Fault (GPF).
- **No Privileged Instructions**: `hlt`, `cli`, `sti`, `lgdt`, `lidt`, `mov crX, reg` will all cause a GPF.
- **No Kernel Memory**: Accessing any address above `0xC0000000` will trigger a Page Fault.
- **Strictly Syscalls**: You *must* use `int 0x80` to request services from the kernel.

---

## 3. How to Make Things Fail

Testing the kernel's robustness is part of the fun. Here is how you can trigger common CPU exceptions.

### A. Division by Zero (ISR 0)
Add this to any task to see the "Division By Zero" exception message:
```c
volatile int a = 5;
volatile int b = 0;
volatile int c = a / b;
```

### B. Page Fault (ISR 14)
Triggered when a task tries to access memory it doesn't own.
- **From Kernel**: `*(int*)0x0 = 123;` (Accessing NULL).
- **From User**: Trying to read kernel memory (addresses above `0xC0000000`).

### C. General Protection Fault (ISR 13)
Triggered in User Mode if you try to execute privileged instructions:
```c
// Inside a User Task
asm volatile("hlt"); // Only allowed in Ring 0
```
The kernel will catch this and usually terminate the offending user task.

### D. Out of Memory (OOM)
Stress the heap allocator by requesting more memory than available:
```c
for (int i = 0; i < 100; i++) {
    void* p = kmalloc(1024 * 1024); // Request 1MB repeatedly
    if (!p) printf("Heap exhausted at %d MB\n", i);
}
```

---

## 4. Debugging Tips

### Headless Logging (Serial COM1)
TilekarOS mirrors all `printf` output to the serial port `0x3F8`. If you are using QEMU, you can see these logs in your terminal by adding:
```bash
qemu-system-i386 -cdrom TilekarOS.iso -serial stdio
```

### Stack Overflows
The kernel allocates **4KB** for each task's stack. If you define a massive local array (e.g., `char buf[5000];`), you will overwrite other kernel data or the `task_t` structure, leading to a spectacular (and educational) crash.

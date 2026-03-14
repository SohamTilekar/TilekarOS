# System Call Internals

System calls are the primary interface between user space and the kernel. In TilekarOS, we use the `int 0x80` software interrupt to transition to kernel mode.

## 1. Syscall Dispatcher

The dispatcher is responsible for taking the registers from an `int 0x80` interrupt and calling the corresponding C function.

### Register Mapping
When `int 0x80` is called:

- **EAX**: Syscall Number
- **EBX**: 1st Argument
- **ECX**: 2nd Argument
- **EDX**: 3rd Argument
- **ESI**: 4th Argument
- **EDI**: 5th Argument

The return value of the syscall is placed back into **EAX**.

### Assembly Stub (`syscall_stub`)
The `syscall_stub` in `syscall.asm` handles the low-level transition:

1.  **Context Save**: Pushes a dummy error code, the interrupt number (0x80), all general-purpose registers (`pushad`), and segment registers.
2.  **Kernel Data Setup**: Sets `DS` and `ES` to the kernel data selector (`0x10`).
3.  **Call Dispatch**: Passes the current stack pointer (`ESP`) as a pointer to `InteruptReg` to the C handler `syscall_handler`.
4.  **Restore State**: After the handler returns, it pops the saved context and executes `iretd` to return to user mode.

### Syscall Table
The `syscall_dispatch` function uses the `EAX` register as an index into the `syscall_table`. If the index is valid, it calls the corresponding handler:
```c
uint32_t syscall_dispatch(uint32_t num, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    if (num >= SYS_MAX) return -1;
    return syscall_table[num](a, b, c, d, e);
}
```

## 2. Kernel-User Interaction

Syscalls are handled on the kernel stack of the calling task.

- The kernel can safely access user-space memory (because it is mapped in the lower 3GB), but care must be taken to validate pointers to prevent security issues.
- If a syscall blocks (e.g., waiting for I/O), the task's state is set to `BLOCKED` and the scheduler picks another task.

# System Call Reference

This document describes the system call API available for TilekarOS applications.

## 1. Overview

System calls are accessed via the `int 0x80` instruction. The syscall number is passed in `EAX`, and arguments are passed in `EBX`, `ECX`, `EDX`, `ESI`, and `EDI`.

### Inline Assembly Wrapper (C)

```c
uint32_t syscall(uint32_t num, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    int ret;
    asm volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "b"(a), "c"(b), "d"(c), "S"(d), "D"(e)
    );
    return ret;
}
```

---

## 2. System Calls

### SYS_EXIT (0)
Terminates the current process.

- **EAX**: 0
- **EBX**: Status code (currently ignored)

### SYS_WRITE (1)
Writes data to a file descriptor.

- **EAX**: 1
- **EBX**: File Descriptor (currently only 1 for stdout)
- **ECX**: Buffer Pointer
- **EDX**: Length

**Returns**: Number of bytes written.

### SYS_GET_PID (2)
Returns the Process ID of the current task.

- **EAX**: 2

**Returns**: Current task ID.

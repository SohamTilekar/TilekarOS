# LibC Internals

TilekarOS includes a minimal C Standard Library (`libc`) tailored for kernel development and future user-space applications. This document explains the implementation of core functions.

## 1. Formatted I/O (`stdio.h`)
**Source Files**: [printf.c](https://github.com/SohamTilekar/TilekarOS/blob/main/libc/stdio/printf.c){: target="_blank" }, [sprintf.c](https://github.com/SohamTilekar/TilekarOS/blob/main/libc/stdio/sprintf.c){: target="_blank" }, [putchar.c](https://github.com/SohamTilekar/TilekarOS/blob/main/libc/stdio/putchar.c){: target="_blank" }

The implementation of `printf` is designed to be **Freestanding**.

### LibK vs. User LibC:
- **`__is_libk`**: When defined (during kernel build), `printf` calls `terminal_write` directly.
- **User Mode**: When NOT defined, `printf` uses the `SYS_WRITE` system call to output data to stdout (File Descriptor 1).

??? example "Code Preview: `printf.c`"
    ```c
    <--8<-- "libc/stdio/printf.c"
    ```

### Supported Specifiers:
- `%s`: String
- `%c`: Character
- `%d` / `%i`: Signed Integer
- `%u`: Unsigned Integer
- `%x` / `%X`: Hexadecimal (Lowercase / Uppercase)
- `%p`: Pointer (Formatted as `0x...`)

---

## 2. String Manipulation (`string.h`)
**Source Files**: [string/](https://github.com/SohamTilekar/TilekarOS/tree/main/libc/string){: target="_blank" }

TilekarOS prioritizes correctness and simplicity in its string implementation.

### Key Functions:
- **`memcpy`**: Fast byte-by-byte copying.
- **`memmove`**: Handles overlapping memory regions by checking if the source is before or after the destination.
- **`memset`**: Fills memory with a constant byte.
- **`strcmp` / `strncmp`**: Standard string comparison.

??? example "Code Preview: `memmove.c`"
    ```c
    <--8<-- "libc/string/memmove.c"
    ```

---

## 3. Standard Library (`stdlib.h`)
**Source Files**: [abort.c](https://github.com/SohamTilekar/TilekarOS/blob/main/libc/stdlib/abort.c){: target="_blank" }, [malloc.c](https://github.com/SohamTilekar/TilekarOS/blob/main/libc/stdlib/malloc.c){: target="_blank" }, [free.c](https://github.com/SohamTilekar/TilekarOS/blob/main/libc/stdlib/free.c){: target="_blank" }

- **`abort()`**: In the kernel, this triggers a **Kernel Panic** message and halts the CPU. In user mode, it would eventually send a signal to terminate the process.
- **`malloc()/free()`**: First-fit allocator backed by `SYS_BRK` (`sbrk`) for userspace heap growth.

??? example "Code Preview: `abort.c`"
    ```c
    <--8<-- "libc/stdlib/abort.c"
    ```

---

## 4. Test/Example: Complex Formatting

You can use `sprintf` to build complex strings before outputting them:

```c
#include <stdio.h>

void log_event(const char* module, int code) {
    char log_buf[128];
    sprintf(log_buf, "[LOG] %s: Error Code %d", module, code);
    puts(log_buf);
}
```

---

## References
- [OSDev: C Library](https://wiki.osdev.org/C_Library)
- [Wikipedia: C standard library](https://en.wikipedia.org/wiki/C_standard_library)
- [Wikipedia: stdio.h](https://en.wikipedia.org/wiki/Stdio.h)
- [Wikipedia: string.h](https://en.wikipedia.org/wiki/String.h)

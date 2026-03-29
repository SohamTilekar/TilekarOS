# LibC Reference

TilekarOS includes a minimal `libc` implementation for kernel and future user-space use.

## 1. stdio.h (Standard Input/Output)
**Header File**: [stdio.h](https://github.com/SohamTilekar/TilekarOS/blob/main/libc/include/stdio.h){: target="_blank" }

??? example "Code Preview: `stdio.h`"
    ```c
    --8<-- "libc/include/stdio.h"
    ```

### `printf`
```c
int printf(const char* format, ...);
```
Outputs a formatted string to the default terminal. Supports `%s`, `%c`, `%d`, `%i`, `%u`, `%x`, `%X`, and `%p`.

### `vsnprintf`
```c
int vsnprintf(char* str, size_t size, const char* format, va_list ap);
```
Writes a formatted string to a buffer with a size limit, preventing buffer overflows.

### `puts`
```c
int puts(const char* s);
```
Writes a string and a newline (`\n`) to the default terminal.

---

## 2. string.h (String Manipulation)
**Header File**: [string.h](https://github.com/SohamTilekar/TilekarOS/blob/main/libc/include/string.h){: target="_blank" }

??? example "Code Preview: `string.h`"
    ```c
    --8<-- "libc/include/string.h"
    ```

### `memcpy`
```c
void* memcpy(void* dest, const void* src, size_t n);
```
Copies `n` bytes from `src` to `dest`. The memory areas **must not overlap**.

### `memmove`
```c
void* memmove(void* dest, const void* src, size_t n);
```
Copies `n` bytes from `src` to `dest`. Memory areas **may overlap**.

### `memset`
```c
void* memset(void* s, int c, size_t n);
```
Fills the first `n` bytes of the memory area pointed to by `s` with the constant byte `c`.

### `memcmp`
```c
int memcmp(const void* s1, const void* s2, size_t n);
```
Compares the first `n` bytes of memory areas `s1` and `s2`.

### `strlen`
```c
size_t strlen(const char* s);
```
Calculates the length of the string `s`, excluding the terminating null byte (`\0`).

### `strcmp`
```c
int strcmp(const char* s1, const char* s2);
```
Compares two strings. Returns an integer less than, equal to, or greater than zero if `s1` is found to be less than, match, or be greater than `s2`.

### `strncmp`
```c
int strncmp(const char* s1, const char* s2, size_t n);
```
Compares up to `n` characters of two strings.

### `strcpy`
```c
char* strcpy(char* dest, const char* src);
```
Copies a string from `src` to `dest`.

### `strncpy`
```c
char* strncpy(char* dest, const char* src, size_t n);
```
Copies up to `n` characters from `src` to `dest`.

### `strchr`
```c
char* strchr(const char* s, int c);
```
Finds the first occurrence of character `c` in string `s`.

### `strrchr`
```c
char* strrchr(const char* s, int c);
```
Finds the last occurrence of character `c` in string `s`.

---

## 3. stdlib.h (Standard Library)
**Header File**: [stdlib.h](https://github.com/SohamTilekar/TilekarOS/blob/main/libc/include/stdlib.h){: target="_blank" }

??? example "Code Preview: `stdlib.h`"
    ```c
    --8<-- "libc/include/stdlib.h"
    ```

### `abort`
```c
void abort(void);
```
Causes abnormal program termination. In the kernel, this typically triggers a panic.

---

## 4. Test/Example: Using LibC in Kernel

```c
#include <stdio.h>
#include <string.h>

void libc_demo() {
    char buf[32];
    strcpy(buf, "TilekarOS");
    printf("Welcome to %s!\n", buf);
    
    if (strcmp(buf, "TilekarOS") == 0) {
        puts("String match successful.");
    }
}
```

---

## References
- [Wikipedia: C standard library](https://en.wikipedia.org/wiki/C_standard_library)
- [OSDev: C Library](https://wiki.osdev.org/C_Library)

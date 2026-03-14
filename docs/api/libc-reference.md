# LibC Reference

TilekarOS includes a minimal `libc` implementation for kernel and future user-space use.

## 1. stdio.h (Standard Input/Output)

### `printf`
```c
int printf(const char* format, ...);
```
Supports basic format specifiers: `%s`, `%c`, `%d`, `%x`.

### `puts`
```c
int puts(const char* s);
```
Writes a string and a newline to stdout.

### `putchar`
```c
int putchar(int c);
```
Writes a single character to stdout.

---

## 2. string.h (String Manipulation)

### `memcpy`
```c
void* memcpy(void* dest, const void* src, size_t n);
```
Copies `n` bytes from `src` to `dest`.

### `memmove`
```c
void* memmove(void* dest, const void* src, size_t n);
```
Copies `n` bytes from `src` to `dest`, handling overlapping memory regions safely.

### `memset`
```c
void* memset(void* s, int c, size_t n);
```
Fills memory with a constant byte.

### `memcmp`
```c
int memcmp(const void* s1, const void* s2, size_t n);
```
Compares the first `n` bytes of two memory areas.

### `strlen`
```c
size_t strlen(const char* s);
```
Calculates the length of a string.

### `strcmp`
```c
int strcmp(const char* s1, const char* s2);
```
Compares two strings.

### `strcpy`
```c
char* strcpy(char* dest, const char* src);
```
Copies a string from `src` to `dest`.

---

## 3. stdlib.h (Standard Library)

### `abort`
```c
void abort(void);
```
Causes abnormal program termination. In the kernel, this typically triggers a panic.

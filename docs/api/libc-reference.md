# LibC Reference

TilekarOS includes a comprehensive `libc` implementation for kernel and user-space applications. This reference documents all standard library functions available.

## Table of Contents

1. [stdio.h](#stdio-standard-inputoutput)
2. [stdlib.h](#stdlib-standard-library)
3. [string.h](#string-string-manipulation)
4. [ctype.h](#ctype-character-classification)
5. [unistd.h](#unistd-posix-api)
6. [errno.h](#errno-error-codes)
7. [assert.h](#assert-assertions)

---

## stdio.h (Standard Input/Output)

**Header File**: [stdio.h](https://github.com/SohamTilekar/TilekarOS/blob/main/libc/include/stdio.h){: target="_blank" }

Standard input/output functions for formatted output, file operations, and character I/O.

### Output Functions

#### `printf`
```c
int printf(const char* format, ...);
```
Outputs a formatted string to stdout. Supports format specifiers:
- `%s` - String
- `%c` - Character
- `%d`, `%i` - Signed integer
- `%u` - Unsigned integer
- `%x`, `%X` - Hexadecimal (lowercase, uppercase)
- `%p` - Pointer address
- `%%` - Literal '%'

**Example:**
```c
int value = 42;
printf("The answer is %d\n", value);
printf("Hex: 0x%x, Pointer: %p\n", value, &value);
```

#### `fprintf`
```c
int fprintf(FILE* stream, const char* format, ...);
```
Outputs a formatted string to a file stream. Similar to `printf` but writes to a specific file descriptor.

**Example:**
```c
FILE* f = fopen("log.txt", "w");
fprintf(f, "Log entry: %s\n", "initialized");
```

#### `sprintf`
```c
int sprintf(char* str, const char* format, ...);
```
Formats a string into a buffer. **Warning**: No bounds checking; use `snprintf` when possible.

#### `snprintf`
```c
int snprintf(char* str, size_t size, const char* format, ...);
```
Formats a string into a buffer with size limit, preventing buffer overflows.

**Example:**
```c
char buffer[64];
snprintf(buffer, sizeof(buffer), "Value: %d", 123);
```

#### `vprintf`, `vfprintf`, `vsprintf`, `vsnprintf`
```c
int vprintf(const char* format, va_list ap);
int vfprintf(FILE* stream, const char* format, va_list ap);
int vsprintf(char* str, const char* format, va_list ap);
int vsnprintf(char* str, size_t size, const char* format, va_list ap);
```
Variadic argument versions of printf functions, accepting `va_list` instead of `...`.

**Example:**
```c
void my_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
```

### Character and Line I/O

#### `putchar`
```c
int putchar(int c);
```
Outputs a single character to stdout.

#### `puts`
```c
int puts(const char* s);
```
Outputs a string and a newline to stdout.

**Example:**
```c
puts("Hello, World!");  // Adds newline automatically
```

### File Operations

#### `fopen`
```c
FILE* fopen(const char* path, const char* mode);
```
Opens a file. Supported modes:
- `"r"` - Read
- `"w"` - Write (create if not exists, truncate if exists)
- `"a"` - Append
- `"r+"` - Read and write

**Returns**: `FILE*` pointer on success, `NULL` on failure.

#### `fclose`
```c
int fclose(FILE* fp);
```
Closes a file stream.

**Returns**: 0 on success, EOF (-1) on failure.

---

## stdlib.h (Standard Library)

**Header File**: [stdlib.h](https://github.com/SohamTilekar/TilekarOS/blob/main/libc/include/stdlib.h){: target="_blank" }

General-purpose utility functions including memory allocation, process control, and data conversion.

### Memory Management

#### `malloc`
```c
void* malloc(size_t size);
```
Allocates `size` bytes of uninitialized memory on the heap.

**Returns**: Pointer to allocated memory, or `NULL` on failure.

**Implementation**: Uses a simple first-fit allocator backed by the `sbrk` syscall.

**Example:**
```c
int* array = (int*)malloc(100 * sizeof(int));
if (array == NULL) {
    printf("Memory allocation failed\n");
    exit(1);
}
array[0] = 42;
free(array);
```

#### `calloc`
```c
void* calloc(size_t nmemb, size_t size);
```
Allocates memory for `nmemb` elements of `size` bytes each. **Memory is initialized to zero.**

**Returns**: Pointer to allocated memory, or `NULL` on failure.

**Example:**
```c
int* array = (int*)calloc(100, sizeof(int));
// All elements are initialized to 0
```

#### `realloc`
```c
void* realloc(void* ptr, size_t size);
```
Resizes a previously allocated block. If `ptr` is `NULL`, behaves like `malloc`. If `size` is 0, behaves like `free`.

**Returns**: Pointer to new allocation, or `NULL` on failure. Original pointer may become invalid.

**Example:**
```c
int* p = (int*)malloc(10 * sizeof(int));
p = (int*)realloc(p, 20 * sizeof(int));  // Resize to 20 elements
```

#### `free`
```c
void free(void* ptr);
```
Deallocates memory previously allocated by `malloc`, `calloc`, or `realloc`. Safe to call with `NULL`.

**Example:**
```c
char* str = (char*)malloc(256);
free(str);
str = NULL;  // Good practice
```

### Heap Management

#### `brk`
```c
int brk(void* addr);
```
Sets the process break (end of heap). Direct syscall wrapper for `SYS_BRK`.

**Returns**: 0 on success, -1 on failure.

#### `sbrk`
```c
void* sbrk(intptr_t increment);
```
Increments the process break by `increment` bytes. Positive values grow the heap, negative values shrink it.

**Returns**: Previous break address on success, `(void*)-1` on failure.

**Example:**
```c
// Grow heap by 4096 bytes
void* old_break = sbrk(4096);
if (old_break == (void*)-1) {
    printf("sbrk failed\n");
}
```

### Process Control

#### `exit`
```c
void exit(int status);
```
Terminates the process with the given exit status. Internally calls `SYS_EXIT` syscall.

#### `abort`
```c
void abort(void);
```
Causes abnormal program termination. In kernel context, may trigger a panic.

### Integer Conversion

#### `atoi`
```c
int atoi(const char* nptr);
```
Converts string to integer.

**Example:**
```c
int num = atoi("123");  // num = 123
```

#### `atol`
```c
long atol(const char* nptr);
```
Converts string to long integer.

#### `atoll`
```c
long long atoll(const char* nptr);
```
Converts string to long long integer.

#### `itoa`
```c
char* itoa(int value, char* str, int base);
```
Converts integer to string in specified base (2-36).

**Example:**
```c
char buf[16];
itoa(255, buf, 16);  // buf = "ff"
itoa(255, buf, 2);   // buf = "11111111"
```

### Integer Arithmetic

#### `abs`, `labs`, `llabs`
```c
int abs(int j);
long labs(long j);
long long llabs(long long j);
```
Return absolute values of integers.

### Searching and Sorting

#### `qsort`
```c
void qsort(void* base, size_t nmemb, size_t size,
           int (*compar)(const void*, const void*));
```
Sorts an array using quicksort. The comparison function should return:
- Negative if first arg < second arg
- Zero if equal
- Positive if first arg > second arg

**Example:**
```c
int compare(const void* a, const void* b) {
    return *(int*)a - *(int*)b;
}

int main() {
    int arr[] = {3, 1, 4, 1, 5, 9};
    qsort(arr, 6, sizeof(int), compare);
}
```

#### `bsearch`
```c
void* bsearch(const void* key, const void* base, size_t nmemb, size_t size,
              int (*compar)(const void*, const void*));
```
Performs binary search on a sorted array.

**Returns**: Pointer to matching element, or `NULL` if not found.

---

## string.h (String Manipulation)

**Header File**: [string.h](https://github.com/SohamTilekar/TilekarOS/blob/main/libc/include/string.h){: target="_blank" }

Functions for string and memory manipulation.

### Memory Operations

#### `memcpy`
```c
void* memcpy(void* dest, const void* src, size_t n);
```
Copies `n` bytes from `src` to `dest`. **Memory areas must not overlap.**

**Returns**: Pointer to `dest`.

#### `memmove`
```c
void* memmove(void* dest, const void* src, size_t n);
```
Copies `n` bytes from `src` to `dest`. **Memory areas may overlap** (handles overlap safely).

**Returns**: Pointer to `dest`.

#### `memset`
```c
void* memset(void* s, int c, size_t n);
```
Fills first `n` bytes of memory area with constant byte `c`.

**Example:**
```c
char buffer[100];
memset(buffer, 0, sizeof(buffer));  // Zero-initialize
```

#### `memchr`
```c
void* memchr(const void* s, int c, size_t n);
```
Finds first occurrence of byte `c` in first `n` bytes.

**Returns**: Pointer to byte, or `NULL` if not found.

#### `memcmp`
```c
int memcmp(const void* s1, const void* s2, size_t n);
```
Compares first `n` bytes of two memory areas. Returns negative, zero, or positive as first differs from second.

#### `memccpy`
```c
void* memccpy(void* dest, const void* src, int c, size_t n);
```
Copies bytes from `src` to `dest` until byte `c` is found or `n` bytes are copied.

**Returns**: Pointer to byte following `c` in `dest`, or `NULL` if `c` not found.

### String Length

#### `strlen`
```c
size_t strlen(const char* s);
```
Calculates length of string (excluding null terminator).

#### `strnlen`
```c
size_t strnlen(const char* s, size_t maxlen);
```
Calculates length of string, up to `maxlen` bytes maximum.

### String Copying

#### `strcpy`
```c
char* strcpy(char* dest, const char* src);
```
Copies string from `src` to `dest`. **No bounds checking; use `strncpy` when possible.**

#### `stpcpy`
```c
char* stpcpy(char* dest, const char* src);
```
Copies string and returns pointer to the null terminator in `dest`.

#### `strncpy`
```c
char* strncpy(char* dest, const char* src, size_t n);
```
Copies up to `n` characters from `src` to `dest`. Pads with nulls if necessary.

### String Concatenation

#### `strcat`
```c
char* strcat(char* dest, const char* src);
```
Appends `src` string to `dest` string. **No bounds checking.**

#### `strncat`
```c
char* strncat(char* dest, const char* src, size_t n);
```
Appends up to `n` characters from `src` to `dest`.

**Example:**
```c
char buf[64] = "Hello";
strncat(buf, " World", sizeof(buf) - strlen(buf) - 1);
```

### String Comparison

#### `strcmp`
```c
int strcmp(const char* s1, const char* s2);
```
Compares two strings. Returns negative, zero, or positive if `s1` is less than, equal to, or greater than `s2`.

#### `strncmp`
```c
int strncmp(const char* s1, const char* s2, size_t n);
```
Compares up to `n` characters of two strings.

### String Searching

#### `strchr`
```c
char* strchr(const char* s, int c);
```
Finds first occurrence of character `c` in string `s`.

**Returns**: Pointer to character, or `NULL` if not found.

#### `strrchr`
```c
char* strrchr(const char* s, int c);
```
Finds last occurrence of character `c` in string `s`.

#### `strstr`
```c
char* strstr(const char* haystack, const char* needle);
```
Finds first occurrence of substring `needle` in `haystack`.

**Example:**
```c
const char* pos = strstr("Hello World", "World");
// pos points to "World"
```

#### `strpbrk`
```c
char* strpbrk(const char* s, const char* accept);
```
Finds first character in `s` that matches any character in `accept`.

#### `strcspn`
```c
size_t strcspn(const char* s, const char* reject);
```
Returns length of prefix of `s` containing no characters from `reject`.

#### `strspn`
```c
size_t strspn(const char* s, const char* accept);
```
Returns length of prefix of `s` containing only characters from `accept`.

### String Tokenization

#### `strtok`
```c
char* strtok(char* s, const char* delim);
```
Breaks string into tokens separated by delimiters. **Modifies the string.**

**Important**: State is maintained across calls. First call with string, subsequent calls with `NULL`.

**Example:**
```c
char line[] = "one,two,three";
char* token = strtok(line, ",");
while (token != NULL) {
    printf("%s\n", token);
    token = strtok(NULL, ",");
}
```

#### `strtok_r`
```c
char* strtok_r(char* s, const char* delim, char** lasts);
```
Reentrant version of `strtok`. Maintains state in `lasts` pointer.

**Example:**
```c
char* saveptr;
char* token = strtok_r(line, ",", &saveptr);
while (token != NULL) {
    printf("%s\n", token);
    token = strtok_r(NULL, ",", &saveptr);
}
```

### Error String

#### `strerror`
```c
const char* strerror(int errnum);
```
Returns human-readable error message for error code.

**Example:**
```c
printf("Error: %s\n", strerror(EBADF));
```

---

## ctype.h (Character Classification)

**Header File**: [ctype.h](https://github.com/SohamTilekar/TilekarOS/blob/main/libc/include/ctype.h){: target="_blank" }

Functions for character classification and conversion.

### Classification Functions

All return non-zero (true) if the character matches the classification, zero (false) otherwise.

#### `isalpha`
```c
int isalpha(int c);
```
Checks if `c` is an alphabetic character (A-Z, a-z).

#### `isdigit`
```c
int isdigit(int c);
```
Checks if `c` is a decimal digit (0-9).

#### `isalnum`
```c
int isalnum(int c);
```
Checks if `c` is alphanumeric (alphabetic or digit).

#### `isspace`
```c
int isspace(int c);
```
Checks if `c` is whitespace (space, tab, newline, etc.).

#### `isupper`
```c
int isupper(int c);
```
Checks if `c` is uppercase (A-Z).

#### `islower`
```c
int islower(int c);
```
Checks if `c` is lowercase (a-z).

#### `isprint`
```c
int isprint(int c);
```
Checks if `c` is printable (including space).

#### `isgraph`
```c
int isgraph(int c);
```
Checks if `c` is printable (excluding space).

#### `ispunct`
```c
int ispunct(int c);
```
Checks if `c` is punctuation character.

#### `isblank`
```c
int isblank(int c);
```
Checks if `c` is space or tab.

#### `iscntrl`
```c
int iscntrl(int c);
```
Checks if `c` is a control character.

#### `isxdigit`
```c
int isxdigit(int c);
```
Checks if `c` is hexadecimal digit (0-9, a-f, A-F).

### Conversion Functions

#### `tolower`
```c
int tolower(int c);
```
Converts uppercase letter to lowercase. Non-letters are returned unchanged.

#### `toupper`
```c
int toupper(int c);
```
Converts lowercase letter to uppercase. Non-letters are returned unchanged.

**Example:**
```c
for (int i = 0; i < len; i++) {
    if (isalpha(str[i])) {
        str[i] = toupper(str[i]);
    }
}
```

---

## unistd.h (POSIX API)

**Header File**: [unistd.h](https://github.com/SohamTilekar/TilekarOS/blob/main/libc/include/unistd.h){: target="_blank" }

POSIX-compatible system call wrappers and process control functions.

### Process Control

#### `fork`
```c
int fork(void);
```
Creates a child process by cloning the current process image (calls `SYS_FORK`).

**Returns**: 
- Child process ID in parent process
- 0 in child process
- -1 on error

#### `execve`
```c
int execve(const char* filename, char* const argv[], char* const envp[]);
```
Replaces current process image with new ELF binary (calls `SYS_EXECVE`).

**Arguments**:
- `filename`: Path to executable
- `argv`: Null-terminated argument array
- `envp`: Null-terminated environment array

**Returns**: -1 on error (on success, does not return).

#### `getpid`
```c
uint32_t getpid(void);
```
Returns Process ID of current task (calls `SYS_GET_PID`).

#### `_exit`
```c
void _exit(int status);
```
Terminates process immediately (calls `SYS_EXIT`).

#### `yield`
```c
void yield(void);
```
Yields CPU time to scheduler (calls `SYS_YIELD`).

### File Operations

#### `open`
```c
int open(const char* path, int flags);
```
Opens file or directory (calls `SYS_OPEN`).

**Returns**: File descriptor on success, negative error code on failure.

#### `read`
```c
int read(int fd, void* buf, size_t count);
```
Reads from file descriptor (calls `SYS_READ`).

**Returns**: Number of bytes read, or negative error code.

#### `write`
```c
int write(int fd, const void* buf, size_t count);
```
Writes to file descriptor (calls `SYS_WRITE`).

**Returns**: Number of bytes written, or negative error code.

#### `close`
```c
int close(int fd);
```
Closes file descriptor (calls `SYS_CLOSE`).

### Directory Operations

#### `mkdir`
```c
int mkdir(const char* path);
```
Creates directory (calls `SYS_MKDIR`).

**Returns**: 0 on success, negative error code on failure.

#### `rmdir`
```c
int rmdir(const char* path);
```
Removes empty directory (calls `SYS_RMDIR`).

#### `unlink`
```c
int unlink(const char* path);
```
Deletes file (calls `SYS_UNLINK`).

#### `readdir`
```c
int readdir(int fd, uint32_t index, void* out);
```
Reads directory entry at index (calls `SYS_READDIR`).

**Parameters**:
- `fd`: Directory file descriptor
- `index`: Entry index to read
- `out`: Pointer to `vfs_dirent_t` buffer for output

**Returns**: 0 on success, negative error code on failure.

---

## errno.h (Error Codes)

**Header File**: [errno.h](https://github.com/SohamTilekar/TilekarOS/blob/main/libc/include/errno.h){: target="_blank" }

Standard POSIX error codes for system calls and library functions.

### Error Codes

| Code | Name | Description |
| :--- | :--- | :--- |
| **1** | `EPERM` | Operation not permitted |
| **2** | `ENOENT` | No such file or directory |
| **3** | `ESRCH` | No such process |
| **4** | `EINTR` | Interrupted system call |
| **5** | `EIO` | I/O error |
| **6** | `ENXIO` | No such device or address |
| **7** | `E2BIG` | Argument list too long |
| **8** | `ENOEXEC` | Exec format error |
| **9** | `EBADF` | Bad file number |
| **10** | `ECHILD` | No child processes |
| **11** | `EAGAIN` | Try again |
| **12** | `ENOMEM` | Out of memory |
| **13** | `EACCES` | Permission denied |
| **14** | `EFAULT` | Bad address |
| **15** | `ENOTBLK` | Block device required |
| **16** | `EBUSY` | Device or resource busy |
| **17** | `EEXIST` | File exists |
| **18** | `EXDEV` | Cross-device link |
| **19** | `ENODEV` | No such device |
| **20** | `ENOTDIR` | Not a directory |
| **21** | `EISDIR` | Is a directory |
| **22** | `EINVAL` | Invalid argument |
| **23** | `ENFILE` | File table overflow |
| **24** | `EMFILE` | Too many open files |
| **25** | `ENOTTY` | Not a typewriter |
| **26** | `ETXTBSY` | Text file busy |
| **27** | `EFBIG` | File too large |
| **28** | `ENOSPC` | No space left on device |
| **29** | `ESPIPE` | Illegal seek |
| **30** | `EROFS` | Read-only file system |
| **31** | `EMLINK` | Too many links |
| **32** | `EPIPE` | Broken pipe |
| **33** | `EDOM` | Math argument out of domain |
| **34** | `ERANGE` | Math result not representable |

### Usage

```c
#include <errno.h>
#include <unistd.h>

int fd = open("/nonexistent.txt", 0);
if (fd < 0) {
    printf("Error: %s\n", strerror(errno));
}
```

---

## assert.h (Assertions)

**Header File**: [assert.h](https://github.com/SohamTilekar/TilekarOS/blob/main/libc/include/assert.h){: target="_blank" }

Runtime assertion and debugging support.

### Assert Macro

#### `assert`
```c
void assert(int expression);
```
Evaluates `expression`. If false and `NDEBUG` is not defined, calls `__assert_fail` which prints diagnostic information and terminates the program.

If `NDEBUG` is defined, `assert` expands to nothing.

**Example:**
```c
#include <assert.h>

int divide(int a, int b) {
    assert(b != 0);  // Abort if b is zero
    return a / b;
}
```

**Compile without assertions:**
```bash
gcc -DNDEBUG program.c
```

---

## Example: Complete LibC Demo

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>

int main() {
    // String operations
    char buffer[64];
    strcpy(buffer, "hello world");
    printf("Original: %s\n", buffer);
    
    // Character classification and conversion
    for (int i = 0; buffer[i]; i++) {
        if (isalpha(buffer[i])) {
            buffer[i] = toupper(buffer[i]);
        }
    }
    printf("Uppercase: %s\n", buffer);
    
    // Memory allocation
    int* array = (int*)malloc(10 * sizeof(int));
    assert(array != NULL);
    
    for (int i = 0; i < 10; i++) {
        array[i] = i * i;
    }
    printf("Squares: ");
    for (int i = 0; i < 10; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");
    
    // String tokenization
    char line[] = "apple,banana,cherry";
    char* token = strtok(line, ",");
    printf("Tokens: ");
    while (token != NULL) {
        printf("%s ", token);
        token = strtok(NULL, ",");
    }
    printf("\n");
    
    // File operations
    int fd = open("/test.txt", 0);
    if (fd >= 0) {
        char content[256];
        int n = read(fd, content, sizeof(content) - 1);
        if (n > 0) {
            content[n] = '\0';
            printf("File content: %s\n", content);
        }
        close(fd);
    }
    
    // Process ID
    printf("My PID: %d\n", getpid());
    
    free(array);
    return 0;
}
```

---

## References

- [C Standard Library - Wikipedia](https://en.wikipedia.org/wiki/C_standard_library)
- [POSIX.1-2017 Standard](https://pubs.opengroup.org/onlinepubs/9699919799/)
- [OSDev: C Library](https://wiki.osdev.org/C_Library)

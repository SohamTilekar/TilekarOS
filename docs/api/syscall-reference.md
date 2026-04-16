# System Call Reference

This document describes the system call API available for TilekarOS applications.

## 1. Overview

System calls are accessed via the `int 0x80` instruction. The syscall number is passed in `EAX`, and arguments are passed in `EBX`, `ECX`, `EDX`, `ESI`, and `EDI`.

### Register Mapping:
- **EAX**: Syscall Number
- **EBX**: 1st Argument
- **ECX**: 2nd Argument
- **EDX**: 3rd Argument
- **ESI**: 4th Argument
- **EDI**: 5th Argument

### Inline Assembly Wrapper (C)
See [syscall.h](https://github.com/SohamTilekar/TilekarOS/blob/main/libc/include/sys/syscall.h){: target="_blank" } for the inline wrapper definition.

??? example "Code Preview: `syscall.h`"
    ```c
    --8<-- "libc/include/sys/syscall.h"
    ```

---

## 2. Error Codes

Syscalls return negative values on failure. Common error codes include:

| Code | Name | Description |
| :--- | :--- | :--- |
| **-1** | `EPERM` | Operation not permitted |
| **-2** | `ENOENT` | No such file or directory |
| **-5** | `EIO` | I/O error |
| **-9** | `EBADF` | Bad file number |
| **-12** | `ENOMEM` | Out of memory |
| **-13** | `EACCES` | Permission denied |
| **-17** | `EEXIST` | File exists |
| **-22** | `EINVAL` | Invalid argument |

---

## 3. System Calls

### SYS_EXIT (0)
Terminates the current process.

- **EAX**: 0
- **EBX**: Status code

**C Example:**
```c
#include <stdlib.h>
void main() {
    exit(0); // Internally calls SYS_EXIT
}
```

### SYS_WRITE (1)
Writes data to a file descriptor.

- **EAX**: 1
- **EBX**: File Descriptor
- **ECX**: Buffer Pointer
- **EDX**: Length

**Returns**: Number of bytes written.

**C Example:**
```c
#include <stdio.h>
void main() {
    printf("Hello User Mode!\n"); // Eventually calls SYS_WRITE
}
```

### SYS_GET_PID (2)
Returns the Process ID of the current task.

- **EAX**: 2

**Returns**: Current task ID.

### SYS_OPEN (3)
Opens a file or directory.

- **EAX**: 3
- **EBX**: Path Pointer
- **ECX**: Flags (currently unused)

**Returns**: File Descriptor (>= 0) or negative error code.

**C Example:**
```c
int fd = open("/hello.txt", 0);
if (fd < 0) {
    // Handle error
}
```

### SYS_READ (4)
Reads data from a file descriptor.

- **EAX**: 4
- **EBX**: File Descriptor
- **ECX**: Buffer Pointer
- **EDX**: Length

**Returns**: Number of bytes read.

### SYS_CLOSE (5)
Closes a file descriptor.

- **EAX**: 5
- **EBX**: File Descriptor

### SYS_MKDIR (6)
Creates a new directory.

- **EAX**: 6
- **EBX**: Path Pointer

### SYS_RMDIR (7)
Deletes an empty directory.

- **EAX**: 7
- **EBX**: Path Pointer

### SYS_UNLINK (8)
Deletes a file.

- **EAX**: 8
- **EBX**: Path Pointer

### SYS_READDIR (9)
Reads directory entries.

- **EAX**: 9
- **EBX**: File Descriptor (for a directory)
- **ECX**: Entry Index
- **EDX**: Buffer Pointer (`vfs_dirent_t*`)

**Returns**: 0 on success, negative error code on failure.

### SYS_FORK (10)
Creates a child task by cloning the current process image.

### SYS_EXECVE (11)
Replaces the current process image with a new ELF binary.

### SYS_YIELD (12)
Yields CPU time to the scheduler.

### SYS_BRK (13)
Gets or sets the current process break (userspace heap end).

- **EAX**: 13
- **EBX**: Requested break address (`0` to query current break)

**Returns**: New/current break on success, `-1` on failure.

---

## 4. Test/Example: User-mode File Creation

```asm
[bits 32]
; Simple program to create a directory and exit
mov eax, 6          ; SYS_MKDIR
mov ebx, dirname    ; "/MYDIR"
int 0x80

mov eax, 0          ; SYS_EXIT
int 0x80

dirname db "/MYDIR", 0
```

---

## References
- [OSDev: System Calls](https://wiki.osdev.org/System_Calls)
- [Wikipedia: System Call](https://en.wikipedia.org/wiki/System_call)

---

## 4. Test/Example: User-mode File Creation

```asm
[bits 32]
; Simple program to create a directory and exit
mov eax, 6          ; SYS_MKDIR
mov ebx, dirname    ; "/MYDIR"
int 0x80

mov eax, 0          ; SYS_EXIT
int 0x80

dirname db "/MYDIR", 0
```

---

## References
- [OSDev: System Calls](https://wiki.osdev.org/System_Calls)
- [Wikipedia: System Call](https://en.wikipedia.org/wiki/System_call)

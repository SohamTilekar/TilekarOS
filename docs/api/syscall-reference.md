# System Call Reference

This document provides complete documentation for TilekarOS system calls. System calls are the interface between user-space applications and the kernel.

## Overview

System calls are invoked via the `int 0x80` instruction on i386 architecture. Arguments are passed via CPU registers:

### Register Mapping

| Register | Purpose |
| :--- | :--- |
| **EAX** | Syscall number |
| **EBX** | 1st argument |
| **ECX** | 2nd argument |
| **EDX** | 3rd argument |
| **ESI** | 4th argument |
| **EDI** | 5th argument |

**Returns**: Result in EAX (negative for errors)

### Inline Assembly Wrapper

See [syscall.h](https://github.com/SohamTilekar/TilekarOS/blob/main/libc/include/sys/syscall.h){: target="_blank" } for implementation details.

```c
static inline uint32_t __syscall(uint32_t num, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    uint32_t ret;
    asm volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "b"(a), "c"(b), "d"(c), "S"(d), "D"(e)
    );
    return ret;
}
```

---

## Error Codes

All system calls return negative values on failure. Error codes follow POSIX standards:

| Code | Name | Description |
| :--- | :--- | :--- |
| **-1** | `EPERM` | Operation not permitted |
| **-2** | `ENOENT` | No such file or directory |
| **-5** | `EIO` | I/O error |
| **-9** | `EBADF` | Bad file descriptor |
| **-12** | `ENOMEM` | Out of memory |
| **-13** | `EACCES` | Permission denied |
| **-17** | `EEXIST` | File exists |
| **-22** | `EINVAL` | Invalid argument |

See [errno.h](https://github.com/SohamTilekar/TilekarOS/blob/main/libc/include/errno.h){: target="_blank" } for the complete error codes list.

---

## System Calls (SYS_* Constants)

### SYS_EXIT (0)

Terminates the current process.

**Signature:**
```c
void exit(int status);  // From stdlib.h
```

**Parameters:**
- **EAX**: 0 (SYS_EXIT)
- **EBX**: Exit status code

**Returns**: Never (process terminates)

**Example:**
```c
#include <stdlib.h>

int main() {
    printf("Goodbye!\n");
    exit(0);
}
```

---

### SYS_WRITE (1)

Writes data to a file descriptor (typically stdout/stderr for kernel messages).

**Signature:**
```c
ssize_t write(int fd, const void* buf, size_t count);  // From unistd.h
```

**Parameters:**
- **EAX**: 1 (SYS_WRITE)
- **EBX**: File descriptor
- **ECX**: Buffer pointer
- **EDX**: Number of bytes to write

**Returns:** Number of bytes written, or negative error code

**Example:**
```c
#include <unistd.h>

int main() {
    const char* msg = "Hello User Mode!\n";
    write(1, msg, 16);  // fd=1 is stdout
}
```

---

### SYS_GET_PID (2)

Returns the Process ID (task ID) of the current process.

**Signature:**
```c
uint32_t getpid(void);  // From unistd.h
```

**Parameters:**
- **EAX**: 2 (SYS_GET_PID)

**Returns:** Current process ID

**Example:**
```c
#include <unistd.h>
#include <stdio.h>

int main() {
    uint32_t pid = getpid();
    printf("My PID: %u\n", pid);
}
```

---

### SYS_OPEN (3)

Opens a file or directory by path.

**Signature:**
```c
int open(const char* path, int flags);  // From unistd.h
```

**Parameters:**
- **EAX**: 3 (SYS_OPEN)
- **EBX**: Pointer to path string
- **ECX**: Flags (currently unused)

**Returns:** File descriptor (>= 0) on success, negative error code on failure

**File Descriptors:**
- 0: stdin (standard input)
- 1: stdout (standard output)
- 2: stderr (standard error)
- 3+: User-opened files

**Example:**
```c
#include <unistd.h>
#include <stdio.h>

int main() {
    int fd = open("/hello.txt", 0);
    if (fd < 0) {
        printf("Failed to open file: %d\n", fd);
        return 1;
    }
    
    char buffer[256];
    int bytes = read(fd, buffer, sizeof(buffer) - 1);
    buffer[bytes] = '\0';
    printf("File content: %s\n", buffer);
    
    close(fd);
    return 0;
}
```

---

### SYS_READ (4)

Reads data from a file descriptor.

**Signature:**
```c
ssize_t read(int fd, void* buf, size_t count);  // From unistd.h
```

**Parameters:**
- **EAX**: 4 (SYS_READ)
- **EBX**: File descriptor
- **ECX**: Buffer pointer (destination)
- **EDX**: Maximum bytes to read

**Returns:** Number of bytes read, 0 on EOF, or negative error code

**Example:**
```c
#include <unistd.h>
#include <stdio.h>

int main() {
    char buffer[512];
    int fd = open("/data.txt", 0);
    
    int n = read(fd, buffer, 256);
    if (n > 0) {
        printf("Read %d bytes\n", n);
    }
    
    close(fd);
    return 0;
}
```

---

### SYS_CLOSE (5)

Closes a file descriptor.

**Signature:**
```c
int close(int fd);  // From unistd.h
```

**Parameters:**
- **EAX**: 5 (SYS_CLOSE)
- **EBX**: File descriptor

**Returns:** 0 on success, negative error code on failure

**Example:**
```c
#include <unistd.h>

int main() {
    int fd = open("/test.txt", 0);
    // Use file...
    close(fd);
}
```

---

### SYS_MKDIR (6)

Creates a new directory.

**Signature:**
```c
int mkdir(const char* path);  // From unistd.h
```

**Parameters:**
- **EAX**: 6 (SYS_MKDIR)
- **EBX**: Pointer to directory path

**Returns:** 0 on success, negative error code on failure

**Example:**
```c
#include <unistd.h>
#include <stdio.h>

int main() {
    int ret = mkdir("/MYDIR");
    if (ret == 0) {
        printf("Directory created!\n");
    } else {
        printf("mkdir failed: %d\n", ret);
    }
}
```

---

### SYS_RMDIR (7)

Removes an empty directory.

**Signature:**
```c
int rmdir(const char* path);  // From unistd.h
```

**Parameters:**
- **EAX**: 7 (SYS_RMDIR)
- **EBX**: Pointer to directory path

**Returns:** 0 on success, negative error code on failure

**Errors:**
- `-ENOENT`: Directory does not exist
- `-ENOTEMPTY`: Directory is not empty
- `-ENOTDIR`: Path is not a directory

**Example:**
```c
#include <unistd.h>

int main() {
    int ret = rmdir("/MYDIR");
    if (ret == 0) {
        printf("Directory removed!\n");
    }
}
```

---

### SYS_UNLINK (8)

Deletes (unlinks) a file.

**Signature:**
```c
int unlink(const char* path);  // From unistd.h
```

**Parameters:**
- **EAX**: 8 (SYS_UNLINK)
- **EBX**: Pointer to file path

**Returns:** 0 on success, negative error code on failure

**Example:**
```c
#include <unistd.h>

int main() {
    int ret = unlink("/oldfile.txt");
    if (ret == 0) {
        printf("File deleted!\n");
    }
}
```

---

### SYS_READDIR (9)

Reads a directory entry at a specific index.

**Signature:**
```c
int readdir(int fd, uint32_t index, void* out);  // From unistd.h
```

**Parameters:**
- **EAX**: 9 (SYS_READDIR)
- **EBX**: Directory file descriptor
- **ECX**: Entry index (0 for first entry, increment to iterate)
- **EDX**: Pointer to vfs_dirent_t output buffer

**Returns:** 0 on success, -1 when no more entries, negative error code on failure

**Structure:**
```c
typedef struct vfs_dirent {
    char name[256];  // Entry name
    uint32_t size;   // File size (bytes)
    uint8_t type;    // File type (directory, regular file, etc.)
} vfs_dirent_t;
```

**Example:**
```c
#include <unistd.h>
#include <stdio.h>

int main() {
    int fd = open("/", 0);
    if (fd < 0) return 1;
    
    vfs_dirent_t entry;
    for (int i = 0; readdir(fd, i, &entry) == 0; i++) {
        printf("Entry: %s (size: %u)\n", entry.name, entry.size);
    }
    
    close(fd);
    return 0;
}
```

---

### SYS_FORK (10)

Creates a child process by cloning the current process image.

**Signature:**
```c
pid_t fork(void);  // From unistd.h
```

**Parameters:**
- **EAX**: 10 (SYS_FORK)

**Returns:**
- In parent: Child process ID
- In child: 0
- On error: Negative error code

**Behavior:**
- Copies parent's memory, registers, and file descriptors
- Child starts execution after fork() call
- Both processes continue from the same point

**Example:**
```c
#include <unistd.h>
#include <stdio.h>

int main() {
    pid_t pid = fork();
    
    if (pid < 0) {
        printf("fork failed!\n");
    } else if (pid == 0) {
        // Child process
        printf("I am the child (PID: %u)\n", getpid());
    } else {
        // Parent process
        printf("I am the parent, child PID: %d\n", pid);
    }
    
    return 0;
}
```

---

### SYS_EXECVE (11)

Replaces the current process image with a new ELF binary.

**Signature:**
```c
int execve(const char* filename, char* const argv[], char* const envp[]);  // From unistd.h
```

**Parameters:**
- **EAX**: 11 (SYS_EXECVE)
- **EBX**: Pointer to executable path
- **ECX**: Pointer to argument array (null-terminated)
- **EDX**: Pointer to environment array (null-terminated)

**Returns:** Never on success (process image replaced). Returns negative error code on failure.

**Arguments Format:**
- argv: NULL-terminated array of pointers to strings
- argv[0]: Typically the program name
- argv[...]: Command-line arguments
- envp: NULL-terminated array of environment variables

**Example:**
```c
#include <unistd.h>
#include <stdio.h>

int main() {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process
        char* argv[] = {"/bin/ls", "-la", "/", NULL};
        char* envp[] = {NULL};
        
        int ret = execve("/bin/ls", argv, envp);
        if (ret < 0) {
            printf("execve failed: %d\n", ret);
        }
    }
    
    return 0;
}
```

---

### SYS_YIELD (12)

Yields the CPU to the scheduler, allowing other processes to run.

**Signature:**
```c
void yield(void);  // From unistd.h
```

**Parameters:**
- **EAX**: 12 (SYS_YIELD)

**Returns:** None

**Purpose:** Cooperative multitasking - allows the current process to give up its time slice.

**Example:**
```c
#include <unistd.h>
#include <stdio.h>

int main() {
    for (int i = 0; i < 1000; i++) {
        printf("Doing work %d...\n", i);
        yield();  // Let other processes run
    }
    
    return 0;
}
```

---

### SYS_BRK (13)

Gets or sets the process break (end of heap for dynamic memory).

**Signature:**
```c
int brk(void* addr);
void* sbrk(intptr_t increment);  // From stdlib.h
```

**Parameters:**
- **EAX**: 13 (SYS_BRK)
- **EBX**: New break address (0 to query current break)

**Returns:** New break address on success, -1 on failure

**Purpose:** Manage the heap. The malloc and free functions use this syscall internally.

**Memory Layout:**
```
+-------------------+
| Program Stack     | (grows downward)
+-------------------+
|                   |
|   Heap (grows up) |
+-------------------+ <- Break (managed by SYS_BRK)
| BSS/Data Segment  |
| Text Segment      |
+-------------------+
```

**Example:**
```c
#include <stdlib.h>
#include <stdio.h>

int main() {
    // Allocate 1KB on heap
    void* old_break = sbrk(1024);
    if (old_break == (void*)-1) {
        printf("sbrk failed!\n");
        return 1;
    }
    
    printf("Old break: %p\n", old_break);
    printf("New break: %p\n", sbrk(0));  // Query current break
    
    // malloc() uses sbrk() internally
    int* array = (int*)malloc(100 * sizeof(int));
    
    free(array);
    return 0;
}
```

---

## References

- [OSDev: System Calls](https://wiki.osdev.org/System_Calls)
- [Linux Man Pages: System Calls](https://man7.org/linux/man-pages/man2/syscalls.2.html)
- [POSIX Specifications](https://pubs.opengroup.org/onlinepubs/9699919799/)

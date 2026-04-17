# Syscall Reference (Detailed)

This page documents kernel syscall signatures, semantics, and return conventions.

Supported syscalls:

- int sys_exit(int status);
  - Terminates current process with status.
  - Returns: does not return to caller.

- ssize_t sys_write(int fd, const void* buf, size_t count);
  - Writes up to count bytes to file descriptor fd (0=stdin,1=stdout,2=stderr).
  - Returns: number of bytes written or negative errno.

- ssize_t sys_read(int fd, void* buf, size_t count);
  - Reads up to count bytes from fd into buf.
  - Returns: bytes read or negative errno.

- int sys_open(const char* path, int flags, int mode);
  - Opens a file and returns fd or negative errno.

- int sys_close(int fd);
  - Closes fd. Returns 0 or negative errno.

- int sys_brk(void* addr);
  - Set process break to addr. Returns 0 on success or negative errno.

- void* sys_sbrk(intptr_t inc);
  - Increment process break by inc, returns previous break or (void*)-1 on error.

- int sys_ioctl(int fd, unsigned long request, void* arg);
  - Device-specific control operations. Returns 0 or negative errno.

- int sys_kill(int pid, int sig);
  - Send signal to process. Returns 0 or negative errno.

- int sys_wait(int* status);
  - Wait for child process. Returns pid or negative errno.

Notes:
- Error returns use negative errno values (e.g., -ENOENT).
- See kernel/include headers for any arch-specific calling conventions.

Examples:

Reading from STDIN:

int n = sys_read(0, buf, sizeof(buf));

Writing to STDOUT:

sys_write(1, "Hello\n", 6);

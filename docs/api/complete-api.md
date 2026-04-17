# TilekarOS Complete API Reference

This document consolidates kernel syscalls, libC functions, VFS and driver APIs in one place for quick reference.

1) Kernel Syscalls
- SYS_EXIT(int code)
- SYS_WRITE(int fd, const void* buf, size_t count)
- SYS_READ(int fd, void* buf, size_t count)
- SYS_OPEN(const char* path, int flags, int mode)
- SYS_CLOSE(int fd)
- SYS_BRK(void* addr) / SYS_SBRK(intptr_t inc)
- SYS_FORK (if supported)
- SYS_EXEC (elf loader interface)
- SYS_IOCTL (device control)
- SYS_KILL / SYS_WAIT (process control)

Refer to docs/api/syscall-reference.md for detailed signatures and error codes.

2) LibC (exported functions)
- stdio: printf, puts, putchar, fopen (if available), fclose, fread, fwrite, vsnprintf
- stdlib: malloc, calloc, realloc, free, abort, atoi, itoa
- string: memcpy, memset, strcmp, strcpy, strlen, strstr, strtok, strncpy
- ctype: isalpha, isdigit, tolower, toupper
- errno: errno values and strerror

See libc/include headers for exact signatures and docs/libc-reference.md for examples.

3) VFS Kernel API
- vnode_t, file_t structures
- vfs_open(const char* path, int flags)
- vfs_read(int fd, void* buf, size_t n)
- vfs_write(int fd, const void* buf, size_t n)
- vfs_close(int fd)
- vfs_mkdir(const char* path)
- vfs_rmdir(const char* path)
- vfs_unlink(const char* path)
- vfs_readdir(int fd, int index, void* out)
- Mounting APIs: vfs_mount(path, fs_driver*)

4) Driver & Device API
- block_device_t: read(block, buf), write(block, buf), block_size
- ata_read/ata_write/ata_identify
- ramdisk_init(addr, size), ramdisk_read, ramdisk_write
- tty/console: tty_write(const char* buf, size_t n)
- keyboard: kbd_register_handler(callback)

5) Kernel Internal Helpers (for driver authors)
- memory_get_phys(void* vaddr)
- kmalloc(size), kfree(ptr)
- interrupt_save()/interrupt_restore()
- out_port_b(port, value), in_port_b(port)

6) Error Codes
- Standard errno codes used across kernel and libc: -ENOENT, -EEXIST, -EISDIR, -ENOTDIR, -EACCES, -EIO, -ENOMEM

7) Examples
- User-space: open, read, write example available in docs/kernel/filesystem.md
- Kernel: vfs usage and driver examples in docs/kernel/filesystem.md and docs/kernel/boot-and-init.md

For full, authoritative signatures and examples, consult these files:
- docs/api/syscall-reference.md
- docs/api/libc-reference.md
- docs/api/libc-internals.md
- docs/drivers/driver-reference.md
- kernel/include/ (headers)
- libc/include/ (headers)

If you want, generate expanded pages per API group (syscalls, libc, vfs, drivers) with full signatures and C examples; confirm and this tool will create them.
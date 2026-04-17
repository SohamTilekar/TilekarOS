# VFS API Reference

Kernel-facing VFS functions and structures.

Core types (from docs and kernel headers):

- typedef struct vnode { char name[256]; uint32_t size; uint8_t type; void* fs_node; struct fs_driver* fs; } vnode_t;
- typedef struct file { vnode_t* vnode; uint32_t offset; uint32_t flags; } file_t;

Functions:
- int vfs_open(const char* path, int flags);
- int vfs_read(int fd, void* buf, size_t count);
- int vfs_write(int fd, const void* buf, size_t count);
- int vfs_close(int fd);
- int vfs_mkdir(const char* path);
- int vfs_rmdir(const char* path);
- int vfs_unlink(const char* path);
- int vfs_readdir(int fd, int index, void* out);
- int vfs_create(const char* path, int mode);
- int vfs_mount(const char* path, struct fs_driver* driver);

Return conventions: 0 on success, negative errno on error, positive values for byte counts where appropriate.

Example (reading a file):

int fd = vfs_open("/README.TXT", 0);
char buf[128];
int n = vfs_read(fd, buf, sizeof(buf));
if (n > 0) { /* use buf */ }
vfs_close(fd);

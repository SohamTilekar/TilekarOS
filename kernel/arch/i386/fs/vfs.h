#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>
#include "devices.h"

typedef enum {
    VFS_TYPE_FILE,
    VFS_TYPE_DIRECTORY
} vnode_type_t;

typedef struct {
    char name[256];
    uint32_t size;
    vnode_type_t type;
} vfs_dirent_t;

struct vnode;
struct file;

typedef struct vfs_ops {
    int (*read)(struct file* file, void* buffer, uint32_t size);
    int (*write)(struct file* file, const void* buffer, uint32_t size);
    struct vnode* (*lookup)(struct vnode* parent, const char* name);
    int (*mkdir)(struct vnode* parent, const char* name);
    int (*rmdir)(struct vnode* parent, const char* name);
    int (*unlink)(struct vnode* parent, const char* name);
    int (*readdir)(struct vnode* dir, uint32_t index, vfs_dirent_t* out);
} vfs_ops_t;

typedef struct vnode {
    vnode_type_t type;
    uint32_t size;
    vfs_ops_t* ops;
    void* fs_data;
    device_t* dev;
    int refcount;
    struct vnode* mounted_vnode; // Redirection for mount points
} vnode_t;

typedef struct file {
    vnode_t* node;
    uint32_t position;
    uint32_t flags;
} file_t;

#define MAX_FILES_PER_PROCESS 16

// VFS API
void vfs_init();
vnode_t* vfs_mount(const char* path, device_t* dev, vnode_t* (*mount_fn)(device_t*));
vnode_t* vfs_device_node_create(device_t* dev);

int vfs_open(const char* path, int flags);
uint32_t vfs_get_size(int fd);
int vfs_read(int fd, void* buffer, uint32_t size);
int vfs_write(int fd, const void* buffer, uint32_t size);
void vfs_close(int fd);

int vfs_mkdir(const char* path);
int vfs_rmdir(const char* path);
int vfs_unlink(const char* path);
int vfs_readdir(int fd, uint32_t index, vfs_dirent_t* out);

#endif // VFS_H

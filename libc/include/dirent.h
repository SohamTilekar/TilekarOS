#ifndef _DIRENT_H
#define _DIRENT_H

#include <stdint.h>

typedef enum {
    VFS_TYPE_FILE,
    VFS_TYPE_DIRECTORY,
    VFS_TYPE_CHAR,
    VFS_TYPE_BLOCK
} vnode_type_t;

typedef struct {
    char name[256];
    uint32_t size;
    vnode_type_t type;
} vfs_dirent_t;

#endif

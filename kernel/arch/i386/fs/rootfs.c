#include "rootfs.h"
#include <string.h>
#include "kmalloc.h"
#include <stdbool.h>

typedef struct rootfs_entry {
    char name[256];
    vnode_t* node;
    struct rootfs_entry* next;
} rootfs_entry_t;

typedef struct {
    rootfs_entry_t* entries;
} rootfs_data_t;

static vfs_ops_t rootfs_ops;

static vnode_t* rootfs_create_vnode(vnode_type_t type) {
    vnode_t* node = kcalloc(1, sizeof(vnode_t));
    if (!node) return NULL;
    node->type = type;
    node->ops = &rootfs_ops;
    if (type == VFS_TYPE_DIRECTORY) {
        rootfs_data_t* data = kcalloc(1, sizeof(rootfs_data_t));
        node->fs_data = data;
    }
    return node;
}

static vnode_t* rootfs_lookup(vnode_t* parent, const char* name) {
    if (parent->type != VFS_TYPE_DIRECTORY || !parent->fs_data) return NULL;
    rootfs_data_t* data = (rootfs_data_t*)parent->fs_data;
    rootfs_entry_t* curr = data->entries;
    while (curr) {
        if (strcmp(curr->name, name) == 0) return curr->node;
        curr = curr->next;
    }
    return NULL;
}

static int rootfs_mkdir(vnode_t* parent, const char* name) {
    if (parent->type != VFS_TYPE_DIRECTORY || !parent->fs_data) return -1;
    if (rootfs_lookup(parent, name)) return -1; // Already exists

    vnode_t* new_node = rootfs_create_vnode(VFS_TYPE_DIRECTORY);
    if (!new_node) return -2;

    rootfs_entry_t* entry = kmalloc(sizeof(rootfs_entry_t));
    if (!entry) {
        // cleanup new_node
        return -2;
    }
    strncpy(entry->name, name, 255);
    entry->name[255] = '\0';
    entry->node = new_node;

    rootfs_data_t* data = (rootfs_data_t*)parent->fs_data;
    entry->next = data->entries;
    data->entries = entry;
    return 0;
}

static int rootfs_readdir(vnode_t* dir, uint32_t index, vfs_dirent_t* out) {
    if (dir->type != VFS_TYPE_DIRECTORY || !dir->fs_data) return -1;
    rootfs_data_t* data = (rootfs_data_t*)dir->fs_data;
    rootfs_entry_t* curr = data->entries;
    uint32_t i = 0;
    while (curr) {
        if (i == index) {
            strncpy(out->name, curr->name, sizeof(out->name) - 1);
            out->name[sizeof(out->name)-1] = '\0';
            out->type = curr->node->type;
            out->size = 0;
            return 0;
        }
        i++;
        curr = curr->next;
    }
    return -1;
}

static vfs_ops_t rootfs_ops = {
    .read = NULL,
    .write = NULL,
    .create = NULL,
    .lookup = rootfs_lookup,
    .mkdir = rootfs_mkdir,
    .rmdir = NULL,
    .unlink = NULL,
    .readdir = rootfs_readdir
};

vnode_t* rootfs_mount(Device_t* dev) {
    (void)dev;
    return rootfs_create_vnode(VFS_TYPE_DIRECTORY);
}

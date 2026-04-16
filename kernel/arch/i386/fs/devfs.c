#include "devfs.h"
#include <string.h>
#include "kmalloc.h"
#include "kernel/tty.h"

typedef struct devfs_node_cache {
    Device_t* dev;
    vnode_t* node;
    struct devfs_node_cache* next;
} devfs_node_cache_t;

static vnode_t* devfs_root = NULL;
static devfs_node_cache_t* devfs_nodes = NULL;
static uint32_t devfs_generation = 0;

static vnode_type_t devfs_device_type_to_vnode(DeviceType_t type) {
    if (type == DEVICE_TYPE_CHAR) return VFS_TYPE_CHAR;
    if (type == DEVICE_TYPE_BLOCK) return VFS_TYPE_BLOCK;
    return VFS_TYPE_FILE;
}

static int dev_vfs_read(file_t* file, void* buffer, uint32_t size) {
    if (file->node->dev && file->node->dev->read) {
        return file->node->dev->read(file->node->dev, buffer, size);
    }
    return -1;
}

static int dev_vfs_write(file_t* file, const void* buffer, uint32_t size) {
    if (file->node->dev && file->node->dev->write) {
        return file->node->dev->write(file->node->dev, buffer, size);
    }

    // Preserve early boot behavior where tty fallback may be used.
    const char* data = (const char*)buffer;
    for (uint32_t i = 0; i < size; i++) {
        terminal_putchar(data[i]);
    }
    return size;
}

static vfs_ops_t device_vfs_ops = {
    .read = dev_vfs_read,
    .write = dev_vfs_write,
    .lookup = NULL,
    .mkdir = NULL,
    .rmdir = NULL,
    .unlink = NULL,
    .readdir = NULL
};

static vnode_t* devfs_create_device_node(Device_t* dev) {
    if (!dev) return NULL;
    vnode_t* node = kmalloc(sizeof(vnode_t));
    if (!node) return NULL;
    node->type = devfs_device_type_to_vnode(dev->type);
    node->size = 0;
    node->ops = &device_vfs_ops;
    node->fs_data = NULL;
    node->dev = dev;
    node->refcount = 1;
    node->mounted_vnode = NULL;
    return node;
}

static vnode_t* devfs_get_cached_node(Device_t* dev) {
    devfs_node_cache_t* curr = devfs_nodes;
    while (curr) {
        if (curr->dev == dev) return curr->node;
        curr = curr->next;
    }

    vnode_t* node = devfs_create_device_node(dev);
    if (!node) return NULL;

    devfs_node_cache_t* entry = kmalloc(sizeof(devfs_node_cache_t));
    if (!entry) return node;
    entry->dev = dev;
    entry->node = node;
    entry->next = devfs_nodes;
    devfs_nodes = entry;
    return node;
}

static vnode_t* devfs_lookup(vnode_t* parent, const char* name) {
    (void)parent;
    Device_t* dev = device_get(name);
    if (!dev) return NULL;
    return devfs_get_cached_node(dev);
}

static int devfs_readdir(vnode_t* dir, uint32_t index, vfs_dirent_t* out) {
    (void)dir;
    if (!out) return -1;

    Device_t* dev = device_get_next(NULL);
    uint32_t i = 0;
    while (dev) {
        if (i == index) {
            strncpy(out->name, dev->name, sizeof(out->name) - 1);
            out->name[sizeof(out->name) - 1] = '\0';
            out->size = 0;
            out->type = devfs_device_type_to_vnode(dev->type);
            return 0;
        }
        i++;
        dev = device_get_next(dev);
    }
    return -1;
}

static vfs_ops_t devfs_dir_ops = {
    .read = NULL,
    .write = NULL,
    .lookup = devfs_lookup,
    .mkdir = NULL,
    .rmdir = NULL,
    .unlink = NULL,
    .readdir = devfs_readdir
};

void devfs_on_device_registered(Device_t* dev) {
    if (!dev) return;
    devfs_generation++;
    (void)devfs_generation;
}

void devfs_init(void) {
    if (devfs_root) return;
    devfs_root = kmalloc(sizeof(vnode_t));
    if (!devfs_root) return;
    devfs_root->type = VFS_TYPE_DIRECTORY;
    devfs_root->size = 0;
    devfs_root->ops = &devfs_dir_ops;
    devfs_root->fs_data = NULL;
    devfs_root->dev = NULL;
    devfs_root->refcount = 1;
    devfs_root->mounted_vnode = NULL;
}

vnode_t* devfs_get_root(void) {
    return devfs_root;
}

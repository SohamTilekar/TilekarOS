#include "vfs.h"
#include <stdbool.h>
#include <string.h>
#include "kmalloc.h"
#include <stdio.h>
#include "kernel/tty.h"

static vnode_t* root_vnode = NULL;
static file_t* global_file_table[MAX_FILES_PER_PROCESS];

// --- Mount Point Table ---

typedef struct vfs_mount_entry {
    char path[256];
    vnode_t* root;
    struct vfs_mount_entry* next;
} vfs_mount_entry_t;

static vfs_mount_entry_t* mount_list = NULL;

static vnode_t* vfs_get_mount_root(const char* path) {
    vfs_mount_entry_t* curr = mount_list;
    while (curr) {
        if (strcmp(curr->path, path) == 0) return curr->root;
        curr = curr->next;
    }
    return NULL;
}

static void vfs_add_mount(const char* path, vnode_t* root) {
    vfs_mount_entry_t* entry = kmalloc(sizeof(vfs_mount_entry_t));
    strncpy(entry->path, path, 255);
    entry->path[255] = '\0';
    entry->root = root;
    entry->next = mount_list;
    mount_list = entry;
}

// --- Device Node Support ---

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

    // Safety Fallback: If this is stdout/stderr and dev write fails, use terminal directly
    // This handles the case where printf is called before devices are fully registered.
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
    .readdir = NULL,
    .unlink = NULL,
    .rmdir = NULL
};

vnode_t* vfs_device_node_create(device_t* dev) {
    vnode_t* node = kmalloc(sizeof(vnode_t));
    node->type = VFS_TYPE_FILE;
    node->size = 0;
    node->ops = &device_vfs_ops;
    node->dev = dev;
    node->fs_data = NULL;
    node->refcount = 1;
    node->mounted_vnode = NULL;
    return node;
}

static void vfs_strcat(char* dest, const char* src) {
    while (*dest) dest++;
    while ((*dest++ = *src++));
}

static vnode_t* resolve_path_ext(const char* path, bool follow_last_mount) {
    if (!root_vnode || path[0] != '/') return NULL;
    vnode_t* curr = root_vnode;
    const char* p = path + 1;
    char current_full_path[512] = "/";

    while (*p) {
        char component[256];
        const char* end = strchr(p, '/');
        if (end) {
            size_t len = end - p;
            strncpy(component, p, len);
            component[len] = '\0';
            p = end + 1;
        } else {
            strcpy(component, p);
            p += strlen(p);
        }
        if (component[0] == '\0') continue;

        // Build path for mount point checking
        if (current_full_path[strlen(current_full_path) - 1] != '/') {
            vfs_strcat(current_full_path, "/");
        }
        vfs_strcat(current_full_path, component);

        if (!curr->ops->lookup) return NULL;
        vnode_t* next = curr->ops->lookup(curr, component);
        if (!next) return NULL;

        // Check mount table for redirection
        vnode_t* mounted = vfs_get_mount_root(current_full_path);

        if (mounted && (follow_last_mount || *p != '\0')) {
            curr = mounted;
        } else {
            curr = next;
        }

        // Follow any explicit mount redirections on the vnode itself
        while (curr->mounted_vnode && (follow_last_mount || *p != '\0')) {
            curr = curr->mounted_vnode;
        }

        if (*p && curr->type != VFS_TYPE_DIRECTORY) return NULL;
    }
    return curr;
}

static vnode_t* resolve_path(const char* path) {
    return resolve_path_ext(path, true);
}

static vnode_t* resolve_path_no_mount(const char* path) {
    return resolve_path_ext(path, false);
}

vnode_t* vfs_mount(const char* path, device_t* dev, vnode_t* (*mount_fn)(device_t*)) {
    if (strcmp(path, "/") == 0) {
        root_vnode = mount_fn(dev);
        vfs_add_mount("/", root_vnode);
        return root_vnode;
    }

    vnode_t* mount_point = resolve_path(path);
    if (!mount_point || mount_point->type != VFS_TYPE_DIRECTORY) return NULL;

    vnode_t* new_root = mount_fn(dev);
    mount_point->mounted_vnode = new_root; // For legacy/internal support
    vfs_add_mount(path, new_root);         // For stable path-based resolution

    return new_root;
}

void vfs_init() {
    for (int i = 0; i < MAX_FILES_PER_PROCESS; i++) {
        global_file_table[i] = NULL;
    }

    // Pre-populate 0, 1, 2 with devices
    device_t* kbd = device_get("kbd0");
    device_t* tty = device_get("tty0");

    if (kbd) {
        file_t* f = kmalloc(sizeof(file_t));
        f->node = vfs_device_node_create(kbd);
        f->position = 0;
        f->flags = 0;
        global_file_table[0] = f; // stdin
    }

    if (tty) {
        // Shared node for stdout/stderr
        vnode_t* tty_node = vfs_device_node_create(tty);

        file_t* f1 = kmalloc(sizeof(file_t));
        f1->node = tty_node;
        f1->position = 0;
        f1->flags = 0;
        global_file_table[1] = f1; // stdout

        file_t* f2 = kmalloc(sizeof(file_t));
        f2->node = tty_node;
        f2->position = 0;
        f2->flags = 0;
        global_file_table[2] = f2; // stderr
    }
}

static int split_path(const char* path, char* parent_path, char* child_name) {
    const char* last_slash = strrchr(path, '/');
    if (!last_slash) return -1;
    if (last_slash == path) {
        strcpy(parent_path, "/");
    } else {
        size_t len = last_slash - path;
        strncpy(parent_path, path, len);
        parent_path[len] = '\0';
    }
    strcpy(child_name, last_slash + 1);
    return 0;
}

int vfs_open(const char* path, int flags) {
    vnode_t* node = resolve_path(path);
    if (!node) return -1;
    int fd = -1;
    for (int i = 3; i < MAX_FILES_PER_PROCESS; i++) {
        if (!global_file_table[i]) { fd = i; break; }
    }
    if (fd == -1) return -2;
    file_t* file = kmalloc(sizeof(file_t));
    file->node = node;
    file->position = 0;
    file->flags = flags;
    global_file_table[fd] = file;
    return fd;
}

int vfs_read(int fd, void* buffer, uint32_t size) {
    if (fd < 0 || fd >= MAX_FILES_PER_PROCESS || !global_file_table[fd]) return -1;
    file_t* file = global_file_table[fd];
    if (!file->node->ops->read) return -2;
    return file->node->ops->read(file, buffer, size);
}

int vfs_write(int fd, const void* buffer, uint32_t size) {
    if (fd < 0 || fd >= MAX_FILES_PER_PROCESS || !global_file_table[fd]) {
        // Immediate fallback for stdout/stderr even if table entry is missing
        if (fd == 1 || fd == 2) {
            const char* data = (const char*)buffer;
            for (uint32_t i = 0; i < size; i++) terminal_putchar(data[i]);
            return size;
        }
        return -1;
    }
    file_t* file = global_file_table[fd];
    if (!file->node->ops->write) return -2;
    return file->node->ops->write(file, buffer, size);
}

void vfs_close(int fd) {
    if (fd < 0 || fd >= MAX_FILES_PER_PROCESS || !global_file_table[fd]) return;
    file_t* file = global_file_table[fd];
    kfree(file);
    global_file_table[fd] = NULL;
}

int vfs_mkdir(const char* path) {
    if (resolve_path_no_mount(path) != NULL) return 0; // Already exists

    char parent_p[256], name[256];
    if (split_path(path, parent_p, name) != 0) return -1;

    // Always resolve parent by following mounts to get to the right FS
    vnode_t* parent = resolve_path(parent_p);
    if (!parent || parent->type != VFS_TYPE_DIRECTORY || !parent->ops->mkdir) return -2;
    return parent->ops->mkdir(parent, name);
}

int vfs_rmdir(const char* path) {
    char parent_p[256], name[256];
    if (split_path(path, parent_p, name) != 0) return -1;
    vnode_t* parent = resolve_path(parent_p);
    if (!parent || parent->type != VFS_TYPE_DIRECTORY || !parent->ops->rmdir) return -2;
    return parent->ops->rmdir(parent, name);
}

int vfs_unlink(const char* path) {
    char parent_p[256], name[256];
    if (split_path(path, parent_p, name) != 0) return -1;
    vnode_t* parent = resolve_path(parent_p);
    if (!parent || parent->type != VFS_TYPE_DIRECTORY || !parent->ops->unlink) return -2;
    return parent->ops->unlink(parent, name);
}

int vfs_readdir(int fd, uint32_t index, vfs_dirent_t* out) {
    if (fd < 0 || fd >= MAX_FILES_PER_PROCESS || !global_file_table[fd]) return -1;
    file_t* file = global_file_table[fd];
    if (file->node->type != VFS_TYPE_DIRECTORY || !file->node->ops->readdir) return -2;
    return file->node->ops->readdir(file->node, index, out);
}

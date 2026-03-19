#include "syscall.h"
#include "task.h"
#include "vfs.h"
#include <kernel/tty.h>
#include <stdint.h>

uint32_t sys_write(uint32_t fd, uint32_t buf, uint32_t len, uint32_t a, uint32_t b) {
    (void)a; (void)b;
    return (uint32_t)vfs_write((int)fd, (const void*)buf, len);
}

uint32_t sys_exit(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    (void)a; (void)b; (void)c; (void)d; (void)e;
    task_exit();
    return 0;
}

uint32_t sys_getpid(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    (void)a; (void)b; (void)c; (void)d; (void)e;
    return current_task->id;
}

uint32_t sys_open(uint32_t path, uint32_t flags, uint32_t c, uint32_t d, uint32_t e) {
    (void)c; (void)d; (void)e;
    return (uint32_t)vfs_open((const char*)path, (int)flags);
}

uint32_t sys_read(uint32_t fd, uint32_t buf, uint32_t len, uint32_t d, uint32_t e) {
    (void)d; (void)e;
    return (uint32_t)vfs_read((int)fd, (void*)buf, len);
}

uint32_t sys_close(uint32_t fd, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    (void)b; (void)c; (void)d; (void)e;
    vfs_close((int)fd);
    return 0;
}

uint32_t sys_mkdir(uint32_t path, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    (void)b; (void)c; (void)d; (void)e;
    return (uint32_t)vfs_mkdir((const char*)path);
}

uint32_t sys_rmdir(uint32_t path, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    (void)b; (void)c; (void)d; (void)e;
    return (uint32_t)vfs_rmdir((const char*)path);
}

uint32_t sys_unlink(uint32_t path, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    (void)b; (void)c; (void)d; (void)e;
    return (uint32_t)vfs_unlink((const char*)path);
}

uint32_t sys_readdir(uint32_t fd, uint32_t index, uint32_t out, uint32_t d, uint32_t e) {
    (void)d; (void)e;
    return (uint32_t)vfs_readdir((int)fd, index, (vfs_dirent_t*)out);
}

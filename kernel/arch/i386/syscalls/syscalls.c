#include "syscall.h"
#include "task.h"
#include "vfs.h"
#include "memory.h"
#include <kernel/tty.h>
#include <stdint.h>

#define USER_STACK_BASE 0xB0000000U

static uint32_t align_up_page(uint32_t value) {
    return (value + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

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

uint32_t sys_brk(uint32_t addr, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    (void)b; (void)c; (void)d; (void)e;

    if (!current_task || current_task->privilege_level != 3) {
        return (uint32_t)-1;
    }

    if (current_task->heap_start == 0) {
        return (uint32_t)-1;
    }

    if (addr == 0) {
        return current_task->heap_break;
    }

    if (addr < current_task->heap_start || addr >= USER_STACK_BASE) {
        return (uint32_t)-1;
    }

    if (addr > current_task->heap_mapped_end) {
        uint32_t map_start = align_up_page(current_task->heap_mapped_end);
        uint32_t map_end = align_up_page(addr);

        for (uint32_t page = map_start; page < map_end; page += PAGE_SIZE) {
            if (memory_get_phys(page) != 0) {
                continue;
            }
            uint32_t phys = pmm_alloc_page_frame();
            if (!phys) {
                return (uint32_t)-1;
            }
            memory_map_page(page, phys, PAGE_FLAG_USER | PAGE_FLAG_WRITE | PAGE_FLAG_PRESENT);
        }
        current_task->heap_mapped_end = map_end;
    }

    current_task->heap_break = addr;
    return current_task->heap_break;
}

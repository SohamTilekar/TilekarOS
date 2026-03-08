#include "syscall.h"
#include "task.h"
#include <kernel/tty.h>
#include <stdint.h>

uint32_t sys_write(uint32_t fd, uint32_t buf, uint32_t len, uint32_t a, uint32_t b) {
    (void)fd;
    (void)a;
    (void)b;
    char* ptr = (char*)buf;

    for (uint32_t i = 0; i < len; i++)
        terminal_putchar(ptr[i]);

    return len;
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

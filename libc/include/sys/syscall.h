#ifndef _SYS_SYSCALL_H
#define _SYS_SYSCALL_H

#include <stdint.h>

enum  {
    SYS_EXIT = 0,
    SYS_WRITE,
    SYS_GET_PID,
    SYS_OPEN,
    SYS_READ,
    SYS_CLOSE,
    SYS_MKDIR,
    SYS_RMDIR,
    SYS_UNLINK,
    SYS_READDIR,
    SYS_FORK,
    SYS_EXECVE,
    SYS_YIELD,
    SYS_MAX
};

static inline uint32_t __syscall(uint32_t num, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    uint32_t ret;
    asm volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "b"(a), "c"(b), "d"(c), "S"(d), "D"(e)
    );
    return ret;
}

#endif

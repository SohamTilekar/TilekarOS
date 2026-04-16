#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "utils.h"

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
    SYS_BRK,
    SYS_MAX
};

uint32_t syscall(uint32_t num, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e);

uint32_t sys_exit(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e);
uint32_t sys_write(uint32_t fd, uint32_t buf, uint32_t len, uint32_t a, uint32_t b);
uint32_t sys_getpid(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e);
uint32_t sys_open(uint32_t path, uint32_t flags, uint32_t c, uint32_t d, uint32_t e);
uint32_t sys_read(uint32_t fd, uint32_t buf, uint32_t len, uint32_t d, uint32_t e);
uint32_t sys_close(uint32_t fd, uint32_t b, uint32_t c, uint32_t d, uint32_t e);
uint32_t sys_mkdir(uint32_t path, uint32_t b, uint32_t c, uint32_t d, uint32_t e);
uint32_t sys_rmdir(uint32_t path, uint32_t b, uint32_t c, uint32_t d, uint32_t e);
uint32_t sys_unlink(uint32_t path, uint32_t b, uint32_t c, uint32_t d, uint32_t e);
uint32_t sys_readdir(uint32_t fd, uint32_t index, uint32_t out, uint32_t d, uint32_t e);
uint32_t sys_brk(uint32_t addr, uint32_t b, uint32_t c, uint32_t d, uint32_t e);

uint32_t syscall_dispatch(InterruptReg_t* r);
void syscall_handler(InterruptReg_t* r);

#endif

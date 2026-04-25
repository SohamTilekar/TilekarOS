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
    SYS_KILL,
    SYS_SIGACTION,
    SYS_SIGRETURN,
    SYS_SIGPROCMASK,
    SYS_WAITPID,
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
uint32_t sys_kill(uint32_t pid, uint32_t sig, uint32_t c, uint32_t d, uint32_t e);
uint32_t sys_sigaction(uint32_t sig, uint32_t act, uint32_t oldact, uint32_t d, uint32_t e);
uint32_t sys_sigreturn(InterruptReg_t* r);
uint32_t sys_sigprocmask(uint32_t how, uint32_t set, uint32_t oldset, uint32_t d, uint32_t e);
uint32_t sys_waitpid(uint32_t pid, uint32_t status_ptr, uint32_t options, uint32_t d, uint32_t e);

uint32_t syscall_dispatch(InterruptReg_t* r);
void syscall_handler(InterruptReg_t* r);

#endif

#ifndef _UNISTD_H
#define _UNISTD_H

#include <sys/syscall.h>
#include <stdint.h>

static inline int fork() {
    return (int)__syscall(SYS_FORK, 0, 0, 0, 0, 0);
}

static inline int execve(const char *filename, char *const argv[], char *const envp[]) {
    return (int)__syscall(SYS_EXECVE, (uint32_t)filename, (uint32_t)argv, (uint32_t)envp, 0, 0);
}

static inline uint32_t getpid() {
    return __syscall(SYS_GET_PID, 0, 0, 0, 0, 0);
}

static inline void yield() {
    __syscall(SYS_YIELD, 0, 0, 0, 0, 0);
}

__attribute__((__noreturn__))
static inline void _exit(int status) {
    __syscall(SYS_EXIT, (uint32_t)status, 0, 0, 0, 0);
    while(1);
}

#endif

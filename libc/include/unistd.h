#ifndef _UNISTD_H
#define _UNISTD_H

#include <sys/syscall.h>
#include <stdint.h>
#include <stddef.h>

#define O_RDONLY 0x00
#define O_WRONLY 0x01
#define O_RDWR   0x02
#define O_CREAT  0x01

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

/* VFS syscall wrappers */
static inline int write(int fd, const void* buf, size_t count) {
    return (int)__syscall(SYS_WRITE, (uint32_t)fd, (uint32_t)buf, (uint32_t)count, 0, 0);
}

static inline int open(const char* path, int flags) {
    return (int)__syscall(SYS_OPEN, (uint32_t)path, (uint32_t)flags, 0, 0, 0);
}

static inline int read(int fd, void* buf, size_t count) {
    return (int)__syscall(SYS_READ, (uint32_t)fd, (uint32_t)buf, (uint32_t)count, 0, 0);
}

static inline int close(int fd) {
    return (int)__syscall(SYS_CLOSE, (uint32_t)fd, 0, 0, 0, 0);
}

static inline int mkdir(const char* path) {
    return (int)__syscall(SYS_MKDIR, (uint32_t)path, 0, 0, 0, 0);
}

static inline int rmdir(const char* path) {
    return (int)__syscall(SYS_RMDIR, (uint32_t)path, 0, 0, 0, 0);
}

static inline int unlink(const char* path) {
    return (int)__syscall(SYS_UNLINK, (uint32_t)path, 0, 0, 0, 0);
}

static inline int readdir(int fd, uint32_t index, void* out) {
    return (int)__syscall(SYS_READDIR, (uint32_t)fd, index, (uint32_t)out, 0, 0);
}

#endif

#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <sys/cdefs.h>
#include <unistd.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((__noreturn__))
void abort(void);

int brk(void* addr);
void* sbrk(intptr_t increment);

void* malloc(size_t size);
void free(void* ptr);

__attribute__((__noreturn__))
static inline void exit(int status) {
    _exit(status);
}

#ifdef __cplusplus
}
#endif

#endif

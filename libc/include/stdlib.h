#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <sys/cdefs.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((__noreturn__))
void abort(void);

__attribute__((__noreturn__))
static inline void exit(int status) {
    _exit(status);
}

#ifdef __cplusplus
}
#endif

#endif

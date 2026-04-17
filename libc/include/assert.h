#ifndef _ASSERT_H
#define _ASSERT_H 1

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NDEBUG
#define assert(ignore) ((void)0)
#else
__attribute__((__noreturn__))
void __assert_fail(const char* assertion, const char* file, unsigned int line, const char* function);
#define assert(expr) ((expr) ? (void)0 : __assert_fail(#expr, __FILE__, __LINE__, __func__))
#endif

#ifdef __cplusplus
}
#endif

#endif

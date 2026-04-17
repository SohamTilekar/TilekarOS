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
void* realloc(void* ptr, size_t size);
void* calloc(size_t nmemb, size_t size);
void free(void* ptr);

int abs(int j);
long labs(long j);
long long llabs(long long j);

int atoi(const char* nptr);
long atol(const char* nptr);
long long atoll(const char* nptr);

char* itoa(int value, char* str, int base);

void qsort(void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*));
void* bsearch(const void* key, const void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*));

__attribute__((__noreturn__))
static inline void exit(int status) {
    _exit(status);
}

#ifdef __cplusplus
}
#endif

#endif

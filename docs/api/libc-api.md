# LibC API Reference

This page lists exported libc functions and exact signatures (from libc/include headers).

stdio
- int printf(const char* fmt, ...)
- int fprintf(FILE* f, const char* fmt, ...)
- int sprintf(char* out, const char* fmt, ...)
- int snprintf(char* out, size_t n, const char* fmt, ...)
- int vsprintf(char* out, const char* fmt, va_list ap)
- int vsnprintf(char* out, size_t n, const char* fmt, va_list ap)
- FILE* fopen(const char* path, const char* mode)
- int fclose(FILE*)
- int putchar(int)
- int puts(const char*)

stdlib
- void abort(void)
- int brk(void* addr)
- void* sbrk(intptr_t increment)
- void* malloc(size_t size)
- void* realloc(void* ptr, size_t size)
- void* calloc(size_t nmemb, size_t size)
- void free(void* ptr)
- int atoi(const char* nptr)
- char* itoa(int value, char* str, int base)
- qsort, bsearch

string
- void* memcpy(void* dst, const void* src, size_t n)
- void* memmove(void* dst, const void* src, size_t n)
- void* memset(void* s, int c, size_t n)
- int strcmp(const char* a, const char* b)
- size_t strlen(const char* s)
- char* strchr(const char* s, int c)
- char* strstr(const char* hay, const char* needle)

ctype
- int isalpha(int c)
- int isdigit(int c)
- int tolower(int c)
- int toupper(int c)

Errors:
- errno values available via errno.h and strerror(int)

Usage examples and libc internals: docs/api/libc-internals-expanded.md, docs/api/malloc-examples.md and docs/api/libc-reference.md

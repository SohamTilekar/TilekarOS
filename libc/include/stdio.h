#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>
#include <stddef.h>
#include <stdarg.h>

#define EOF (-1)

typedef struct {
    int fd;
} FILE;

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

#define stdin  stdin
#define stdout stdout
#define stderr stderr

#ifdef __cplusplus
extern "C" {
#endif

int printf(const char* __restrict, ...);
int fprintf(FILE* __restrict, const char* __restrict, ...);
int sprintf(char* __restrict, const char* __restrict, ...);
int snprintf(char* __restrict, size_t, const char* __restrict, ...);
int vsprintf(char* __restrict, const char* __restrict, va_list);
int vsnprintf(char* __restrict, size_t, const char* __restrict, va_list);
int vfprintf(FILE* __restrict, const char* __restrict, va_list);

int vprintf(const char* __restrict, va_list);

FILE* fopen(const char* __restrict, const char* __restrict);
int fclose(FILE*);

int putchar(int);
int puts(const char*);

#ifdef __cplusplus
}
#endif

#endif

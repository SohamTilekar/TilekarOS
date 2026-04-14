#include <stdio.h>

int puts(const char* s) {
    while (*s) {
        if (putchar(*s++) == EOF) return EOF;
    }
    return putchar('\n') == EOF ? EOF : 0;
}

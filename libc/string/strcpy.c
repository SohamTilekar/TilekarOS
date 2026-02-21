#include <string.h>

char* strcpy(char* __restrict dst, const char* __restrict src) {
    char* ret = dst;
    while ((*dst++ = *src++));
    return ret;
}

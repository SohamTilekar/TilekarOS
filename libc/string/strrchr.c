#include <string.h>

char* strrchr(const char* s, int c) {
    const char* last = (char*)0;
    while (*s) {
        if (*s == (char)c) last = s;
        s++;
    }
    return (char*)last;
}

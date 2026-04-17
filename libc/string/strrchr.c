#include <string.h>

char* strrchr(const char* s, int c) {
    char ch = (char)c;
    const char* last = (char*)0;
    while (*s) {
        if (*s == ch) last = s;
        s++;
    }
    if (ch == '\0') return (char*)s;
    return (char*)last;
}

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>

static char* itoa_internal(unsigned long long value, char* str, int base) {
    char* rc;
    char* ptr;
    char* low;
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }
    rc = ptr = str;
    low = ptr;
    do {
        *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[value % base];
        value /= base;
    } while (value);
    *ptr = '\0';
    char* start = low;
    char* end = ptr - 1;
    while (start < end) {
        char tmp = *start;
        *start++ = *end;
        *end-- = tmp;
    }
    return rc;
}

int sprintf(char* str, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    int count = 0;
    while (*format) {
        if (*format == '%') {
            format++;
            if (*format == 's') {
                char* s = va_arg(args, char*);
                while (*s) {
                    *str++ = *s++;
                    count++;
                }
            } else if (*format == 'd' || *format == 'i') {
                int i = va_arg(args, int);
                char buffer[32];
                if (i < 0) {
                    *str++ = '-';
                    count++;
                    i = -i;
                }
                itoa_internal((unsigned long long)i, buffer, 10);
                char* b = buffer;
                while (*b) {
                    *str++ = *b++;
                    count++;
                }
            } else if (*format == 'u') {
                unsigned int u = va_arg(args, unsigned int);
                char buffer[32];
                itoa_internal((unsigned long long)u, buffer, 10);
                char* b = buffer;
                while (*b) {
                    *str++ = *b++;
                    count++;
                }
            } else if (*format == 'x' || *format == 'X') {
                unsigned int x = va_arg(args, unsigned int);
                char buffer[32];
                itoa_internal((unsigned long long)x, buffer, 16);
                if (*format == 'X') {
                    for(int k=0; buffer[k]; k++) {
                        if (buffer[k] >= 'a' && buffer[k] <= 'z')
                            buffer[k] -= 32;
                    }
                }
                char* b = buffer;
                while (*b) {
                    *str++ = *b++;
                    count++;
                }
            } else if (*format == 'c') {
                char c = (char)va_arg(args, int);
                *str++ = c;
                count++;
            } else if (*format == '%') {
                *str++ = '%';
                count++;
            } else {
                // Unknown format, just skip or copy literally?
                *str++ = '%';
                *str++ = *format;
                count += 2;
            }
            format++;
        } else {
            *str++ = *format++;
            count++;
        }
    }
    *str = '\0';
    
    va_end(args);
    return count;
}

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// Extremely minimal sprintf for our needs
int sprintf(char* str, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    int count = 0;
    while (*format) {
        if (*format == '%' && *(format + 1) == 's') {
            char* s = va_arg(args, char*);
            while (*s) {
                *str++ = *s++;
                count++;
            }
            format += 2;
        } else if (*format == '%' && *(format + 1) == '.') {
            // Special handling for %s.%s if needed, but let's just do basic
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

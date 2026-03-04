#include "kernel/tty.h"
#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#if !defined(__is_libk)
#include <sys/syscall.h>
#endif

static bool print(const char* data, size_t length) {
#if defined(__is_libk)
	terminal_write(data, length);
#else
	__syscall(SYS_WRITE, 0, (uint32_t)data, length, 0, 0);
#endif
	return true;
}

static char* itoa(unsigned long long value, char* str, int base) {
    char* rc;
    char* ptr;
    char* low;
    // Check for supported base.
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }
    rc = ptr = str;
    // Set '-' for negative decimals is handled in printf wrappers, 
    // we treat value as unsigned long long here for generic base conversion.
    
    // Remember where the numbers start.
    low = ptr;
    // The actual conversion.
    do {
        // Modulo is negative for negative numbers, not needed for unsigned.
        *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[value % base];
        value /= base;
    } while (value);
    // Terminate the string.
    *ptr = '\0';
    // Invert the numbers.
    char* start = low;
    char* end = ptr - 1;
    while (start < end) {
        char tmp = *start;
        *start++ = *end;
        *end-- = tmp;
    }
    return rc;
}

int printf(const char* restrict format, ...) {
	va_list parameters;
	va_start(parameters, format);

	int written = 0;

	while (*format != '\0') {
		size_t maxrem = INT_MAX - written;

		if (format[0] != '%' || format[1] == '%') {
			if (format[0] == '%')
				format++;
			size_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			if (maxrem < amount) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, amount))
				return -1;
			format += amount;
			written += amount;
			continue;
		}

		const char* format_begun_at = format++;

		if (*format == 'c') {
			format++;
			char c = (char) va_arg(parameters, int /* char promotes to int */);
			if (!maxrem) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(&c, sizeof(c)))
				return -1;
			written++;
		} else if (*format == 's') {
			format++;
			const char* str = va_arg(parameters, const char*);
			size_t len = strlen(str);
			if (maxrem < len) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(str, len))
				return -1;
			written += len;
		} else if (*format == 'd' || *format == 'i') {
			format++;
			int i = va_arg(parameters, int);
			char buffer[32];
			if (i < 0) {
				if (!print("-", 1)) return -1;
				written++;
				i = -i;
			}
			itoa((unsigned long long)i, buffer, 10);
			size_t len = strlen(buffer);
			if (maxrem < len) return -1;
			if (!print(buffer, len)) return -1;
			written += len;
		} else if (*format == 'u') {
			format++;
			unsigned int u = va_arg(parameters, unsigned int);
			char buffer[32];
			itoa(u, buffer, 10);
			size_t len = strlen(buffer);
			if (maxrem < len) return -1;
			if (!print(buffer, len)) return -1;
			written += len;
		} else if (*format == 'x') {
			format++;
			unsigned int x = va_arg(parameters, unsigned int);
			char buffer[32];
			itoa(x, buffer, 16);
			size_t len = strlen(buffer);
			if (maxrem < len) return -1;
			if (!print(buffer, len)) return -1;
			written += len;
		} else if (*format == 'X') {
			format++;
			unsigned int x = va_arg(parameters, unsigned int);
			char buffer[32];
			itoa(x, buffer, 16);
			// Convert to uppercase
			for(int k=0; buffer[k]; k++) {
				if (buffer[k] >= 'a' && buffer[k] <= 'z') 
					buffer[k] -= 32;
			}
			size_t len = strlen(buffer);
			if (maxrem < len) return -1;
			if (!print(buffer, len)) return -1;
			written += len;
		} else if (*format == 'p') {
			format++;
			void* ptr = va_arg(parameters, void*);
			char buffer[32];
			if (!print("0x", 2)) return -1;
			written += 2;
			itoa((unsigned long long)(uintptr_t)ptr, buffer, 16);
			size_t len = strlen(buffer);
			if (maxrem < len) return -1;
			if (!print(buffer, len)) return -1;
			written += len;
		} else {
			format = format_begun_at;
			size_t len = strlen(format);
			if (maxrem < len) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, len))
				return -1;
			written += len;
			format += len;
		}
	}

	va_end(parameters);
	return written;
}

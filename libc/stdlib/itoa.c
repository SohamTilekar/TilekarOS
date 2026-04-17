#include <stdlib.h>
#include <string.h>

char* itoa(int value, char* str, int base) {
	char* rc;
	char* ptr;
	char* low;
	// Check for supported base.
	if (base < 2 || base > 36) {
		*str = '\0';
		return str;
	}
	rc = ptr = str;
	// Set '-' for negative numbers and convert to absolute value.
	if (value < 0 && base == 10) {
		*ptr++ = '-';
	}
	low = ptr;
	if (value < 0) {
		unsigned int v = (unsigned int)-value;
		do {
			*ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[v % base];
			v /= base;
		} while (v);
	} else {
		unsigned int v = (unsigned int)value;
		do {
			*ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[v % base];
			v /= base;
		} while (v);
	}
	// Terminate the string.
	*ptr-- = '\0';
	// Invert the digits.
	while (low < ptr) {
		char tmp = *low;
		*low++ = *ptr;
		*ptr-- = tmp;
	}
	return rc;
}

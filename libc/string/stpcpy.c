#include <string.h>

char* stpcpy(char* restrict dest, const char* restrict src) {
	while ((*dest = *src)) {
		dest++;
		src++;
	}
	return dest;
}

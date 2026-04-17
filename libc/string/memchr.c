#include <string.h>

void* memchr(const void* ptr, int ch, size_t n) {
	const unsigned char* p = (const unsigned char*)ptr;
	unsigned char c = (unsigned char)ch;
	while (n--) {
		if (*p == c)
			return (void*)p;
		p++;
	}
	return NULL;
}

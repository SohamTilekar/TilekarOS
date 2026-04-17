#include <string.h>

size_t strspn(const char* s, const char* accept) {
	size_t count = 0;
	while (*s && strchr(accept, *s++))
		count++;
	return count;
}

#include <string.h>

size_t strcspn(const char* s, const char* reject) {
	size_t count = 0;
	while (*s && !strchr(reject, *s++))
		count++;
	return count;
}

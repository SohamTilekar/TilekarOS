#include <string.h>

char* strpbrk(const char* s, const char* accept) {
	while (*s) {
		if (strchr(accept, *s))
			return (char*)s;
		s++;
	}
	return NULL;
}

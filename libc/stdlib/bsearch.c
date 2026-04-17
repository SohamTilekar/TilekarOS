#include <stdlib.h>

void* bsearch(const void* key, const void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*)) {
	const unsigned char* p = base;
	while (nmemb > 0) {
		size_t i = nmemb / 2;
		const void* mid = p + i * size;
		int cmp = compar(key, mid);
		if (cmp == 0) return (void*)mid;
		if (cmp > 0) {
			p = (const unsigned char*)mid + size;
			nmemb -= i + 1;
		} else {
			nmemb = i;
		}
	}
	return NULL;
}

#include <stdlib.h>
#include <string.h>

static void swap(void* a, void* b, size_t size) {
	unsigned char* p = a, * q = b;
	while (size--) {
		unsigned char tmp = *p;
		*p++ = *q;
		*q++ = tmp;
	}
}

void qsort(void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*)) {
	if (nmemb < 2) return;
	unsigned char* pivot = (unsigned char*)base + (nmemb / 2) * size;
	unsigned char* i = base;
	unsigned char* j = (unsigned char*)base + (nmemb - 1) * size;

	while (i <= j) {
		while (compar(i, pivot) < 0) i += size;
		while (compar(j, pivot) > 0) j -= size;
		if (i <= j) {
			if (pivot == i) pivot = j;
			else if (pivot == j) pivot = i;
			swap(i, j, size);
			i += size;
			j -= size;
		}
	}
	if ((unsigned char*)base < j) qsort(base, (j - (unsigned char*)base) / size + 1, size, compar);
	if (i < (unsigned char*)base + nmemb * size) qsort(i, ((unsigned char*)base + nmemb * size - i) / size, size, compar);
}

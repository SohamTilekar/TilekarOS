#include <string.h>

char* strtok_r(char* restrict s, const char* restrict delim, char** restrict saveptr) {
	char* token;
	if (s == NULL) s = *saveptr;

	// Scan leading delimiters
	s += strspn(s, delim);
	if (*s == '\0') {
		*saveptr = s;
		return NULL;
	}

	// Token starts here
	token = s;
	// Scan for next delimiter
	s = strpbrk(token, delim);
	if (s == NULL) {
		// No more delimiters
		*saveptr = strchr(token, '\0');
	} else {
		// Terminate token and set saveptr to next char
		*s = '\0';
		*saveptr = s + 1;
	}
	return token;
}

static char* __strtok_saveptr;
char* strtok(char* restrict s, const char* restrict delim) {
	return strtok_r(s, delim, &__strtok_saveptr);
}

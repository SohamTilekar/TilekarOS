#include <ctype.h>

int isalnum(int c) {
	return isalpha(c) || isdigit(c);
}

int isalpha(int c) {
    return islower(c) || isupper(c);
}

int isblank(int c) {
    return c == ' ' || c == '\t';
}

int iscntrl(int c) {

	return (unsigned int)c < 32 || c == 127;
}

int isdigit(int c) {
	return (unsigned int)c - '0' < 10;
}

int isgraph(int c) {
	return (unsigned int)c - '!' < 94;
}

int islower(int c) {
	return (unsigned int)c - 'a' < 26;
}

int isprint(int c) {
	return (unsigned int)c - ' ' < 95;
}

int ispunct(int c) {
	return isprint(c) && !isalnum(c) && !isspace(c);
}

int isspace(int c) {
	return c == ' ' || (unsigned int)c - '\t' < 5;
}

int isupper(int c) {
	return (unsigned int)c - 'A' < 26;
}

int isxdigit(int c) {
	return isdigit(c) || (unsigned int)(c | 32) - 'a' < 6;
}

int tolower(int c) {
	if (isupper(c)) return c | 32;
	return c;
}

int toupper(int c) {
	if (islower(c)) return c & ~32;
	return c;
}

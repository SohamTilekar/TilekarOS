#include <stdlib.h>
#include <ctype.h>

int atoi(const char* nptr) {
	int res = 0;
	int sign = 1;
	while (isspace(*nptr)) nptr++;
	if (*nptr == '-') {
		sign = -1;
		nptr++;
	} else if (*nptr == '+') {
		nptr++;
	}
	while (isdigit(*nptr)) {
		res = res * 10 + (*nptr - '0');
		nptr++;
	}
	return res * sign;
}

long atol(const char* nptr) {
	long res = 0;
	long sign = 1;
	while (isspace(*nptr)) nptr++;
	if (*nptr == '-') {
		sign = -1;
		nptr++;
	} else if (*nptr == '+') {
		nptr++;
	}
	while (isdigit(*nptr)) {
		res = res * 10 + (*nptr - '0');
		nptr++;
	}
	return res * sign;
}

long long atoll(const char* nptr) {
	long long res = 0;
	long long sign = 1;
	while (isspace(*nptr)) nptr++;
	if (*nptr == '-') {
		sign = -1;
		nptr++;
	} else if (*nptr == '+') {
		nptr++;
	}
	while (isdigit(*nptr)) {
		res = res * 10 + (*nptr - '0');
		nptr++;
	}
	return res * sign;
}

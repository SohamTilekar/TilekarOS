#include <stdio.h>

static FILE _stdin = {0};
static FILE _stdout = {1};
static FILE _stderr = {2};

FILE* stdin = &_stdin;
FILE* stdout = &_stdout;
FILE* stderr = &_stderr;

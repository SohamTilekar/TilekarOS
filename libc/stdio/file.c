#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

FILE* fopen(const char* __restrict filename, const char* __restrict mode) {
    int flags = 0;
    
    if (strcmp(mode, "r") == 0) {
        flags = O_RDONLY;
    } else if (strcmp(mode, "w") == 0) {
        flags = O_WRONLY | O_CREAT;
    } else if (strcmp(mode, "a") == 0) {
        flags = O_WRONLY | O_CREAT | O_APPEND;
    } else {
        return NULL;
    }

    int fd = open(filename, flags);
    if (fd < 0) {
        return NULL;
    }

    FILE* f = (FILE*)malloc(sizeof(FILE));
    if (!f) {
        close(fd);
        return NULL;
    }

    f->fd = fd;
    return f;
}

int fclose(FILE* f) {
    if (!f) return EOF;
    int fd = f->fd;
    free(f);
    return close(fd);
}

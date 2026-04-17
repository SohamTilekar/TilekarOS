#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("cat: missing operand\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        const char* path = argv[i];
        
        // Normalize path (convert \ to /)
        char normalized[512];
        strncpy(normalized, path, 511);
        normalized[511] = '\0';
        for (int j = 0; normalized[j]; j++) {
            if (normalized[j] == '\\') normalized[j] = '/';
        }

        int fd = open(normalized, O_RDONLY);
        if (fd < 0) {
            printf("cat: %s: No such file or directory\n", normalized);
            continue;
        }

        char buffer[1024];
        int bytes_read;
        while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
            write(1, buffer, bytes_read);
        }
        close(fd);
    }
    printf("\n");
    return 0;
}

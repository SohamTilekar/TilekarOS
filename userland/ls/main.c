#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

void list_dir(const char* path) {
    // Normalize path (convert \ to /)
    char normalized[512];
    strncpy(normalized, path, 511);
    normalized[511] = '\0';
    for (int i = 0; normalized[i]; i++) {
        if (normalized[i] == '\\') normalized[i] = '/';
    }

    int fd = open(normalized, O_RDONLY);
    if (fd < 0) {
        printf("ls: cannot open '%s'\n", normalized);
        return;
    }

    vfs_dirent_t dirent;
    int index = 0;
    while (readdir(fd, index++, &dirent) == 0) {
        if (dirent.name[0] == '\0') break;
        printf("%s%s  ", dirent.name, (dirent.type == VFS_TYPE_DIRECTORY) ? "/" : "");
    }
    printf("\n");
    close(fd);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        list_dir("/");
    } else {
        for (int i = 1; i < argc; i++) {
            if (argc > 2) printf("%s:\n", argv[i]);
            list_dir(argv[i]);
            if (argc > 2 && i < argc - 1) printf("\n");
        }
    }
    return 0;
}

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

void log_checkpoint(const char* id) {
    printf("[CHECKPOINT: %s]\n", id);
    int fd = open("/tmp/PROCESS.LOG", 1); // VFS_O_CREAT
    if (fd >= 0) {
        char junk[256];
        int bytes;
        while ((bytes = read(fd, junk, sizeof(junk))) > 0);
        write(fd, id, strlen(id));
        write(fd, "\n", 1);
        close(fd);
    }
}

int main() {
    printf("  [PASS] Execve: Replaced image with /BIN/HELLO\n");
    printf("  HELLO: My PID is %d\n", getpid());
    log_checkpoint("HELLO_ALIVE");

    mkdir("/tmp");
    int fd = open("/tmp/PROCESS.RES", 1); // 1 = VFS_O_CREAT
    if (fd >= 0) {
        write(fd, "PASS", 4);
        close(fd);
    }

    return 0;
}

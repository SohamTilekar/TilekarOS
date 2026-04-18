#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

int main() {
    uint32_t initial_pid = getpid(); // initial_pid will always be 0
    printf("Init process (PID %u) started.\n", initial_pid);

    fork();

    uint32_t current_pid = getpid();
    if (current_pid != initial_pid) {
        // Child process
        printf("Init Child: Launching /bin/sh...\n");
        char* argv[] = {"/bin/sh", NULL};
        execve("/bin/sh", argv, NULL);

        // If execve fails
        printf("Init Child: Failed to launch /bin/sh\n");
        _exit(1);
    } else {
        // Parent process
        printf("Init Process Entering Infinite Yield loop.\n", current_pid);
        while (1) {
            yield();
        }
    }

}

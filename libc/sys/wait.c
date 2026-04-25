#include <sys/wait.h>
#include <sys/syscall.h>

int wait(int *status) {
    return waitpid(-1, status, 0);
}

int waitpid(int pid, int *status, int options) {
    return (int)__syscall(SYS_WAITPID, (uint32_t)pid, (uint32_t)status, (uint32_t)options, 0, 0);
}

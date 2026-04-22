#include <signal.h>
#include <sys/syscall.h>

int kill(int pid, int sig) {
    return __syscall(SYS_KILL, (uint32_t)pid, (uint32_t)sig, 0, 0, 0);
}

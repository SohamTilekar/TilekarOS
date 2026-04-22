#include <signal.h>
#include <sys/syscall.h>

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
    return __syscall(SYS_SIGPROCMASK, (uint32_t)how, (uint32_t)set, (uint32_t)oldset, 0, 0);
}

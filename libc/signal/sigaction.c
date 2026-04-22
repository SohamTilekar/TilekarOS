#include <signal.h>
#include <sys/syscall.h>

extern void __sig_restorer(void);

int sigaction(int sig, const struct sigaction *act, struct sigaction *oldact) {
    struct sigaction kact;
    if (act) {
        kact = *act;
        kact.sa_restorer = __sig_restorer;
    }
    return __syscall(SYS_SIGACTION, (uint32_t)sig, act ? (uint32_t)&kact : 0, (uint32_t)oldact, 0, 0);
}

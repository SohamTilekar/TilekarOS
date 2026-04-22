#include <signal.h>
#include <sys/syscall.h>

void (*signal(int sig, void (*func)(int)))(int) {
    struct sigaction act, oact;
    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (sig == SIGALRM) {
#ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
#endif
    } else {
        act.sa_flags |= SA_RESTART;
    }
    if (sigaction(sig, &act, &oact) < 0)
        return SIG_ERR;
    return oact.sa_handler;
}

int sigemptyset(sigset_t *set) {
    *set = 0;
    return 0;
}

int sigfillset(sigset_t *set) {
    *set = ~0ULL;
    return 0;
}

int sigaddset(sigset_t *set, int signo) {
    if (signo <= 0 || signo >= NSIG) return -1;
    *set |= (1ULL << signo);
    return 0;
}

int sigdelset(sigset_t *set, int signo) {
    if (signo <= 0 || signo >= NSIG) return -1;
    *set &= ~(1ULL << signo);
    return 0;
}

int sigismember(const sigset_t *set, int signo) {
    if (signo <= 0 || signo >= NSIG) return -1;
    return (*set & (1ULL << signo)) != 0;
}

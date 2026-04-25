#include "syscall.h"
#include "task.h"
#include <stdint.h>
#include <stddef.h>

typedef uint32_t (*syscall_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

uint32_t syscall(uint32_t num, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    int ret;
    asm volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "b"(a), "c"(b), "d"(c), "S"(d), "D"(e)
    );
    return ret;
}

syscall_t syscall_table[] = {
    sys_exit,
    sys_write,
    sys_getpid,
    sys_open,
    sys_read,
    sys_close,
    sys_mkdir,
    sys_rmdir,
    sys_unlink,
    sys_readdir,
    NULL, // SYS_FORK
    NULL, // SYS_EXECVE
    NULL, // SYS_YIELD
    sys_brk,
    sys_kill,
    sys_sigaction,
    NULL, // SYS_SIGRETURN (needs InterruptReg_t)
    sys_sigprocmask,
    sys_waitpid,
    NULL  // SYS_MAX
};

uint32_t syscall_dispatch(InterruptReg_t* r) {
    uint32_t num = r->eax;
    if (num >= SYS_MAX) return -1;

    if (num == SYS_FORK) {
        uint32_t parent_pid = current_task->id;
        task_t* child = task_fork(r);
        if (!child) return -1;

        /* If task_fork() returned a task whose pid matches the parent's pid,
         * we're running in the child, so return 0. Otherwise we're in the
         * parent and should return the child's pid. */
        if (child->id == parent_pid) {
            return 0;
        }

        return child->id;
    }

    if (num == SYS_EXECVE) {
        return (uint32_t)task_execve((const char*)r->ebx, (char *const*)r->ecx, (char *const*)r->edx, r);
    }

    if (num == SYS_YIELD) {
        task_yield(r);
        return 0;
    }

    if (num == SYS_SIGRETURN) {
        return sys_sigreturn(r);
    }

    if (syscall_table[num] == NULL) return -1;

    return syscall_table[num](r->ebx, r->ecx, r->edx, r->esi, r->edi);
}

extern void check_signals(InterruptReg_t* r);

void syscall_handler(InterruptReg_t* r) {
    task_preempt_disable();
    r->eax = syscall_dispatch(r);
    task_preempt_enable();
    check_signals(r);
}

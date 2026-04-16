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
    NULL  // SYS_MAX
};

uint32_t syscall_dispatch(InterruptReg_t* r) {
    uint32_t num = r->eax;
    if (num >= SYS_MAX) return -1;

    if (num == SYS_FORK) {
        task_t* child = task_fork(r);
        return child ? child->id : -1;
    }

    if (num == SYS_EXECVE) {
        return (uint32_t)task_execve((const char*)r->ebx, r);
    }

    if (num == SYS_YIELD) {
        task_yield(r);
        return 0;
    }

    if (syscall_table[num] == NULL) return -1;

    return syscall_table[num](r->ebx, r->ecx, r->edx, r->esi, r->edi);
}

void syscall_handler(InterruptReg_t* r) {
    task_preempt_disable();
    r->eax = syscall_dispatch(r);
    task_preempt_enable();
}

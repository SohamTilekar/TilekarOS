#include "syscall.h"
#include <stdint.h>

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
    sys_readdir
};

uint32_t syscall_dispatch(uint32_t num, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    if (num >= SYS_MAX)
        return -1;

    return syscall_table[num](a, b, c, d, e);}

void syscall_handler(InteruptReg* r) {
    uint32_t ret = syscall_dispatch(
        r->eax,
        r->ebx,
        r->ecx,
        r->edx,
        r->esi,
        r->edi
    );

    r->eax = ret;
}

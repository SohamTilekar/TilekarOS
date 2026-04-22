#include "syscall.h"
#include "task.h"
#include "vfs.h"
#include "memory.h"
#include <kernel/tty.h>
#include <stdint.h>

#define USER_STACK_BASE 0xB0000000U

static uint32_t align_up_page(uint32_t value) {
    return (value + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

uint32_t sys_write(uint32_t fd, uint32_t buf, uint32_t len, uint32_t a, uint32_t b) {
    (void)a; (void)b;
    return (uint32_t)vfs_write((int)fd, (const void*)buf, len);
}

uint32_t sys_exit(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    (void)a; (void)b; (void)c; (void)d; (void)e;
    task_exit();
    return 0;
}

uint32_t sys_getpid(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    (void)a; (void)b; (void)c; (void)d; (void)e;
    return current_task->id;
}

uint32_t sys_open(uint32_t path, uint32_t flags, uint32_t c, uint32_t d, uint32_t e) {
    (void)c; (void)d; (void)e;
    return (uint32_t)vfs_open((const char*)path, (int)flags);
}

uint32_t sys_read(uint32_t fd, uint32_t buf, uint32_t len, uint32_t d, uint32_t e) {
    (void)d; (void)e;
    return (uint32_t)vfs_read((int)fd, (void*)buf, len);
}

uint32_t sys_close(uint32_t fd, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    (void)b; (void)c; (void)d; (void)e;
    vfs_close((int)fd);
    return 0;
}

uint32_t sys_mkdir(uint32_t path, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    (void)b; (void)c; (void)d; (void)e;
    return (uint32_t)vfs_mkdir((const char*)path);
}

uint32_t sys_rmdir(uint32_t path, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    (void)b; (void)c; (void)d; (void)e;
    return (uint32_t)vfs_rmdir((const char*)path);
}

uint32_t sys_unlink(uint32_t path, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    (void)b; (void)c; (void)d; (void)e;
    return (uint32_t)vfs_unlink((const char*)path);
}

uint32_t sys_readdir(uint32_t fd, uint32_t index, uint32_t out, uint32_t d, uint32_t e) {
    (void)d; (void)e;
    return (uint32_t)vfs_readdir((int)fd, index, (vfs_dirent_t*)out);
}

uint32_t sys_brk(uint32_t addr, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    (void)b; (void)c; (void)d; (void)e;

    if (!current_task || current_task->privilege_level != 3) {
        return (uint32_t)-1;
    }

    if (current_task->heap_start == 0) {
        return (uint32_t)-1;
    }

    if (addr == 0) {
        return current_task->heap_break;
    }

    if (addr < current_task->heap_start || addr >= USER_STACK_BASE) {
        return (uint32_t)-1;
    }

    if (addr > current_task->heap_mapped_end) {
        uint32_t map_start = align_up_page(current_task->heap_mapped_end);
        uint32_t map_end = align_up_page(addr);

        for (uint32_t page = map_start; page < map_end; page += PAGE_SIZE) {
            if (memory_get_phys(page) != 0) {
                continue;
            }
            uint32_t phys = pmm_alloc_page_frame();
            if (!phys) {
                return (uint32_t)-1;
            }
            memory_map_page(page, phys, PAGE_FLAG_USER | PAGE_FLAG_WRITE | PAGE_FLAG_PRESENT);
        }
        current_task->heap_mapped_end = map_end;
    }

    current_task->heap_break = addr;
    return current_task->heap_break;
}

uint32_t sys_kill(uint32_t pid, uint32_t sig, uint32_t c, uint32_t d, uint32_t e) {
    (void)c; (void)d; (void)e;
    if (sig >= 64) return (uint32_t)-1;
    task_t* target = task_get_by_pid(pid);
    if (!target) return (uint32_t)-1;
    if (sig > 0) {
        siginfo_t info;
        info.si_signo = sig;
        info.si_code = 0; // SI_USER
        info.si_pid = current_task ? current_task->id : 0;
        info.si_uid = 0;
        info.si_addr = NULL;
        info.si_status = 0;
        task_signal_enqueue(target, info);
    }
    return 0;
}

uint32_t sys_sigaction(uint32_t sig, uint32_t act_ptr, uint32_t oldact_ptr, uint32_t d, uint32_t e) {
    (void)d; (void)e;
    if (sig == 0 || sig >= 64 || sig == 9 /* SIGKILL */ || sig == 19 /* SIGSTOP */) return (uint32_t)-1;
    if (!current_task) return (uint32_t)-1;

    struct sigaction* act = (struct sigaction*)act_ptr;
    struct sigaction* oldact = (struct sigaction*)oldact_ptr;

    uint32_t flags = interrupt_save();
    if (oldact) {
        *oldact = current_task->sigactions[sig];
    }
    if (act) {
        current_task->sigactions[sig] = *act;
    }
    interrupt_restore(flags);
    return 0;
}

uint32_t sys_sigprocmask(uint32_t how, uint32_t set_ptr, uint32_t oldset_ptr, uint32_t d, uint32_t e) {
    (void)d; (void)e;
    if (!current_task) return (uint32_t)-1;

    sigset_t* set = (sigset_t*)set_ptr;
    sigset_t* oldset = (sigset_t*)oldset_ptr;

    uint32_t flags = interrupt_save();
    if (oldset) {
        *oldset = current_task->blocked_signals;
    }
    if (set) {
        sigset_t val = *set & ~((1ULL<<9) | (1ULL<<19)); // Cannot block SIGKILL or SIGSTOP
        if (how == 0) /* SIG_BLOCK */ current_task->blocked_signals |= val;
        else if (how == 1) /* SIG_UNBLOCK */ current_task->blocked_signals &= ~val;
        else if (how == 2) /* SIG_SETMASK */ current_task->blocked_signals = val;
    }
    interrupt_restore(flags);
    return 0;
}

uint32_t sys_sigreturn(InterruptReg_t* r) {
    if (!current_task || current_task->privilege_level != 3) return (uint32_t)-1;

    uint32_t* ustack = (uint32_t*)r->useresp;
    
    uint64_t old_blocked = *(uint64_t*)&ustack[3];
    InterruptReg_t* saved_regs = (InterruptReg_t*)ustack[2];

    // Security check: validate the context pointer
    if ((uint32_t)saved_regs >= KERNEL_START) {
        // Attempt to restore from kernel space, terminate task to prevent privilege escalation
        task_exit();
        return -1;
    }

    // Additional checks could be added to ensure saved_regs->cs is a user segment

    uint32_t flags = interrupt_save();
    current_task->blocked_signals = old_blocked;

    // Restore the full register state
    *r = *saved_regs;
    
    // Ensure segment registers are user mode segments to prevent ring 0 escalation
    r->cs = 0x1B;
    r->ds = 0x23;
    r->es = 0x23;
    r->fs = 0x23;
    r->gs = 0x23;
    r->ss = 0x23;
    
    interrupt_restore(flags);
    return r->eax; // The restored EAX
}

#include "task.h"
#include "idt.h"
#include "local_config.h"
#include "timer.h"
#include "kmalloc.h"
#include "utils.h"
#include "gdt.h"
#include "elf.h"
#include "../fs/vfs.h"
#include <stdio.h>
#include <string.h>
#include "memory.h"

extern void context_switch(uint32_t** current_esp, uint32_t* next_esp, uint32_t next_cr3, uint32_t intr_num);

extern uint32_t get_cr3(void);



static uint32_t next_tid = 0;
task_t* current_task = NULL;
static task_t* zombie_list = NULL;
static int32_t scheduler_trigger_index = -1;

static uint32_t align_up_page(uint32_t value) {
    return (value + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

static int task_init_user_heap(task_t* task, uint32_t heap_start, bool map_initial_page) {
    if (!task) return -1;

    uint32_t aligned_heap_start = align_up_page(heap_start);
    if (aligned_heap_start >= 0xB0000000) {
        return -1;
    }

    task->heap_start = aligned_heap_start;
    task->heap_break = aligned_heap_start;
    task->heap_mapped_end = aligned_heap_start;

    if (!map_initial_page) {
        return 0;
    }

    uint32_t heap_page_phys = pmm_alloc_page_frame();
    if (!heap_page_phys) {
        return -1;
    }
    memory_map_page(aligned_heap_start, heap_page_phys, PAGE_FLAG_USER | PAGE_FLAG_WRITE | PAGE_FLAG_PRESENT);
    task->heap_mapped_end = aligned_heap_start + PAGE_SIZE;
    return 0;
}

static int task_init_file_table(task_t* task) {
    return vfs_task_file_table_init(task->file_table);
}

/**
 * cleanup_zombies - Frees memory of tasks that have exited.
 */
static void cleanup_zombies() {
    while (zombie_list) {
        task_t* z = zombie_list;
        zombie_list = z->next;

        if (z->stack_limit) {
            kfree(z->stack_limit);
        }
        vfs_task_file_table_destroy(z->file_table);
        // printf("[Scheduler] Cleaning up Task %d\n", z->id);
        kfree(z);
    }
}

/**
 * task_wrapper - A wrapper function that calls the task's entry point
 * and ensures task_exit is called when the entry point returns.
 */
static void task_wrapper(void (*entry)(void)) {
    uint32_t status = 0;
    if (current_task && current_task->privilege_level == 3) {
        // Switch to user mode via iret
        // User stack is mapped at 0xB0000000 (1 page = 4KB)
        uint32_t user_esp = 0xB0000000 + 4096;

        // Push argc=0, argv=NULL onto user stack for crt0
        user_esp -= 8;
        uint32_t* ustack = (uint32_t*)user_esp;
        ustack[0] = 0; // argc
        ustack[1] = 0; // argv

        asm volatile(
            "cli \n\t"
            "mov $0x23, %%ax \n\t" // User data segment
            "mov %%ax, %%ds \n\t"
            "mov %%ax, %%es \n\t"
            "mov %%ax, %%fs \n\t"
            "mov %%ax, %%gs \n\t"
            "pushl $0x23 \n\t"     // SS
            "pushl %0 \n\t"        // ESP
            "pushl $0x202 \n\t"    // EFLAGS (Interrupts enabled)
            "pushl $0x1B \n\t"     // CS (User code segment)
            "pushl %1 \n\t"        // EIP (Entry point)
            "iret"
            : : "r"(user_esp), "r"(entry) : "memory", "eax"
        );
        // We will never return here from user mode.
        // The exit status will be passed via SYS_EXIT.
    } else {
        entry();
    }
    task_exit();
}

void task_init_scheduler() {
    task_t* main_task = kmalloc(sizeof(task_t));
    main_task->id = -1; // -1 cz the Init Task should get PID 0
    main_task->stack_limit = NULL; // Main kernel stack is already allocated
    main_task->state = TASK_RUNNING;
    main_task->preempt_count = 0;
    main_task->regs = NULL;
    main_task->page_directory = get_cr3();
    main_task->next = main_task;
    main_task->privilege_level = 0;
    main_task->heap_start = 0;
    main_task->heap_break = 0;
    main_task->heap_mapped_end = 0;
    if (task_init_file_table(main_task) < 0) {
        kfree(main_task);
        return;
    }
    current_task = main_task;
    scheduler_trigger_index = insert_trigger(100, &task_yield, 0); // Use local ticks

    // printf("Scheduler initialized. Main task TID: %d\n", main_task->id);
}

task_t* task_create(void (*entry)(void), uint8_t privilege_level) {
    // Disable interrupts to prevent scheduler from ticking while we modify the ready queue
    uint32_t flags = interrupt_save();
    cleanup_zombies(); // Opportunity to clean up

    task_t* task = kmalloc(sizeof(task_t));
    if (!task) {
        interrupt_restore(flags);
        return NULL;
    }

    // Allocate 4KB stack for kernel-mode execution of the task (even for user tasks, they need a kernel stack for interrupts)
    uint32_t stack_size = 4096;
    void* stack = kmalloc(stack_size);
    if (!stack) {
        kfree(task);
        interrupt_restore(flags);
        return NULL;
    }

    task->stack_limit = stack;
    task->id = next_tid++;
    task->state = TASK_READY;
    task->preempt_count = 0;
    task->privilege_level = privilege_level;
    task->page_directory = get_cr3(); // Inherit current page directory
    task->heap_start = 0;
    task->heap_break = 0;
    task->heap_mapped_end = 0;
    if (task_init_file_table(task) < 0) {
        kfree(stack);
        kfree(task);
        interrupt_restore(flags);
        return NULL;
    }

    // Set up the initial stack frame
    // The stack grows downwards, so start at the top
    uint32_t* stack_ptr = (uint32_t*)((uint32_t)stack + stack_size);

    // Prepare the stack so that context_switch (which now returns via iret)
    // properly "returns" to task_wrapper and task_wrapper receives 'entry'.

    // task_wrapper(entry)
    *(--stack_ptr) = (uint32_t)entry;      // Argument to task_wrapper
    *(--stack_ptr) = 0;                    // Dummy return address for task_wrapper

    // Hardware interrupt frame (popped by iret inside context_switch)
    *(--stack_ptr) = 0x202; // EFLAGS (Interrupts enabled)
    *(--stack_ptr) = GDT_KERNEL_CS_OFFSET;  // CS (Kernel Code Segment)
    *(--stack_ptr) = (uint32_t)task_wrapper; // EIP (Return address)

    // pushad registers
    *(--stack_ptr) = 0; // EAX
    *(--stack_ptr) = 0; // ECX
    *(--stack_ptr) = 0; // EDX
    *(--stack_ptr) = 0; // EBX
    *(--stack_ptr) = 0; // ESP (dummy)
    *(--stack_ptr) = 0; // EBP
    *(--stack_ptr) = 0; // ESI
    *(--stack_ptr) = 0; // EDI

    // Segment registers
    *(--stack_ptr) = 0x10; // DS
    *(--stack_ptr) = 0x10; // ES
    *(--stack_ptr) = 0x10; // FS
    *(--stack_ptr) = 0x10; // GS

    task->kernel_stack = stack_ptr;
    task->regs = (registers_t*)stack_ptr;

    // Add to the circular ready queue
    task->next = current_task->next;
    current_task->next = task;

    // printf("Task created. TID: %d, Privilege: %d, ESP: %x\n", task->id, task->privilege_level, (uint32_t)task->kernel_stack);

    // We restore interrupts for THIS currently running task that just created a new one
    interrupt_restore(flags);
    return task;
}

task_t* task_create_user(void* start_addr, void* end_addr) {
    uint32_t size = (uint32_t)end_addr - (uint32_t)start_addr;

    uint32_t flags = interrupt_save();
    cleanup_zombies();

    task_t* task = kmalloc(sizeof(task_t));
    if (!task) {
        interrupt_restore(flags);
        return NULL;
    }

    uint32_t stack_size = 4096;
    void* stack = kmalloc(stack_size);
    if (!stack) {
        kfree(task);
        interrupt_restore(flags);
        return NULL;
    }

    task->stack_limit = stack;
    task->id = next_tid++;
    task->state = TASK_READY;
    task->preempt_count = 0;
    task->privilege_level = 3;
    if (task_init_file_table(task) < 0) {
        kfree(stack);
        kfree(task);
        interrupt_restore(flags);
        return NULL;
    }

    uint32_t* pd_virt = memory_create_user_pagedir();
    uint32_t pd_phys = (uint32_t)pd_virt - KERNEL_START;
    task->page_directory = pd_phys;

    uint32_t current_pd = get_cr3();
    memory_set_pagedir(pd_virt);

    uint32_t num_code_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    if (num_code_pages == 0) num_code_pages = 1;

    for (uint32_t i = 0; i < num_code_pages; i++) {
        uint32_t code_phys = pmm_alloc_page_frame();
        memory_map_page(0x08048000 + i * PAGE_SIZE, code_phys, PAGE_FLAG_USER | PAGE_FLAG_WRITE | PAGE_FLAG_PRESENT);
    }

    uint32_t ustack_phys = pmm_alloc_page_frame();
    memory_map_page(0xB0000000, ustack_phys, PAGE_FLAG_USER | PAGE_FLAG_WRITE | PAGE_FLAG_PRESENT);

    memcpy((void*)0x08048000, start_addr, size);

    uint32_t heap_start = align_up_page(0x08048000 + size);
    if (task_init_user_heap(task, heap_start, true) < 0) {
        memory_set_pagedir((uint32_t*)(current_pd + KERNEL_START));
        kfree(stack);
        kfree(task);
        interrupt_restore(flags);
        return NULL;
    }

    memory_set_pagedir((uint32_t*)(current_pd + KERNEL_START));

    uint32_t* stack_ptr = (uint32_t*)((uint32_t)stack + stack_size);

    *(--stack_ptr) = 0x08048000;      // Argument to task_wrapper
    *(--stack_ptr) = 0;               // Dummy return address for task_wrapper

    *(--stack_ptr) = 0x202; // EFLAGS
    *(--stack_ptr) = GDT_KERNEL_CS_OFFSET;  // CS
    *(--stack_ptr) = (uint32_t)task_wrapper; // EIP

    // pushad registers
    *(--stack_ptr) = 0; // EAX
    *(--stack_ptr) = 0; // ECX
    *(--stack_ptr) = 0; // EDX
    *(--stack_ptr) = 0; // EBX
    *(--stack_ptr) = 0; // ESP
    *(--stack_ptr) = 0; // EBP
    *(--stack_ptr) = 0; // ESI
    *(--stack_ptr) = 0; // EDI

    // Segment registers
    *(--stack_ptr) = 0x10; // DS
    *(--stack_ptr) = 0x10; // ES
    *(--stack_ptr) = 0x10; // FS
    *(--stack_ptr) = 0x10; // GS

    task->kernel_stack = stack_ptr;
    task->regs = (registers_t*)stack_ptr;

    task->next = current_task->next;
    current_task->next = task;

    // printf("User Task created. TID: %d, Size: %d, ESP: %x\n", task->id, size, (uint32_t)task->kernel_stack);

    interrupt_restore(flags);
    return task;
}

task_t* task_create_elf(void* elf_data, uint8_t privilege_level) {
    Elf32_Ehdr *hdr = (Elf32_Ehdr *)elf_data;
    if (!elf_check_supported(hdr)) {
        printf("ELF: Unsupported or invalid ELF file\n");
        return NULL;
    }

    uint32_t flags = interrupt_save();
    cleanup_zombies();

    task_t* task = kmalloc(sizeof(task_t));
    if (!task) {
        interrupt_restore(flags);
        return NULL;
    }

    uint32_t stack_size = 4096;
    void* stack = kmalloc(stack_size);
    if (!stack) {
        kfree(task);
        interrupt_restore(flags);
        return NULL;
    }

    task->stack_limit = stack;
    task->id = next_tid++;
    task->state = TASK_READY;
    task->preempt_count = 0;
    task->privilege_level = privilege_level;
    if (task_init_file_table(task) < 0) {
        kfree(stack);
        kfree(task);
        interrupt_restore(flags);
        return NULL;
    }

    uint32_t* pd_virt = memory_create_user_pagedir();
    uint32_t pd_phys = (uint32_t)pd_virt - KERNEL_START;
    task->page_directory = pd_phys;

    uint32_t current_pd = get_cr3();
    memory_set_pagedir(pd_virt);

    if (privilege_level == 3) {
        // Map a 4KB page for the user stack at 0xB0000000
        uint32_t ustack_phys = pmm_alloc_page_frame();
        memory_map_page(0xB0000000, ustack_phys, PAGE_FLAG_USER | PAGE_FLAG_WRITE | PAGE_FLAG_PRESENT);
    }

    // Load ELF segments
    uint32_t heap_start = 0;
    void* entry = elf_load_segments(hdr, elf_data, privilege_level, &heap_start);
    if (!entry) {
        // Switch back before failing
        memory_set_pagedir((uint32_t*)(current_pd + KERNEL_START));
        kfree(stack);
        kfree(task);
        interrupt_restore(flags);
        return NULL;
    }

    if (privilege_level == 3) {
        if (task_init_user_heap(task, heap_start, true) < 0) {
            memory_set_pagedir((uint32_t*)(current_pd + KERNEL_START));
            kfree(stack);
            kfree(task);
            interrupt_restore(flags);
            return NULL;
        }
    } else {
        task->heap_start = 0;
        task->heap_break = 0;
        task->heap_mapped_end = 0;
    }

    memory_set_pagedir((uint32_t*)(current_pd + KERNEL_START));

    uint32_t* stack_ptr = (uint32_t*)((uint32_t)stack + stack_size);

    *(--stack_ptr) = (uint32_t)entry;      // Argument to task_wrapper
    *(--stack_ptr) = 0;               // Dummy return address for task_wrapper

    *(--stack_ptr) = 0x202; // EFLAGS
    *(--stack_ptr) = GDT_KERNEL_CS_OFFSET;  // CS
    *(--stack_ptr) = (uint32_t)task_wrapper; // EIP

    // pushad registers
    *(--stack_ptr) = 0; // EAX
    *(--stack_ptr) = 0; // ECX
    *(--stack_ptr) = 0; // EDX
    *(--stack_ptr) = 0; // EBX
    *(--stack_ptr) = 0; // ESP
    *(--stack_ptr) = 0; // EBP
    *(--stack_ptr) = 0; // ESI
    *(--stack_ptr) = 0; // EDI

    // Segment registers
    *(--stack_ptr) = 0x10; // DS
    *(--stack_ptr) = 0x10; // ES
    *(--stack_ptr) = 0x10; // FS
    *(--stack_ptr) = 0x10; // GS

    task->kernel_stack = stack_ptr;
    task->regs = (registers_t*)stack_ptr;

    task->next = current_task->next;
    current_task->next = task;

    // printf("ELF Task created. TID: %d, Entry: %p, Privilege: %d, ESP: %x\n",
    //        task->id, entry, task->privilege_level, (uint32_t)task->kernel_stack);

    interrupt_restore(flags);
    return task;
}

task_t* task_create_elf_from_file(const char* path, uint8_t privilege_level) {
    int fd = vfs_open(path, 0);
    if (fd < 0) {
        printf("ELF: Failed to open file %s\n", path);
        return NULL;
    }

    uint32_t size = vfs_get_size(fd);
    if (size == 0) {
        printf("ELF: File %s is empty or size unknown\n", path);
        vfs_close(fd);
        return NULL;
    }

    void* elf_data = kmalloc(size);
    if (!elf_data) {
        printf("ELF: Failed to allocate memory for ELF data\n");
        vfs_close(fd);
        return NULL;
    }

    int bytes_read = vfs_read(fd, elf_data, size);
    vfs_close(fd);

    if (bytes_read != (int)size) {
        printf("ELF: Failed to read full file %s (read %d of %d)\n", path, bytes_read, size);
        kfree(elf_data);
        return NULL;
    }

    task_t* task = task_create_elf(elf_data, privilege_level);

    // We can free elf_data because task_create_elf copies data into the new process pages
    kfree(elf_data);

    return task;
}

int task_file_table_copy(task_t* dst, const task_t* src) {
    if (!dst || !src) return -1;
    return vfs_task_file_table_copy(dst->file_table, src->file_table);
}

int task_file_table_set(task_t* task, int fd, const file_t* src_file) {
    if (!task) return -1;
    return vfs_task_file_table_set(task->file_table, fd, src_file);
}

void task_preempt_disable(void) {
    uint32_t flags = interrupt_save();
    if (current_task) {
        current_task->preempt_count++;
    }
    interrupt_restore(flags);
}

void task_preempt_enable(void) {
    uint32_t flags = interrupt_save();
    if (current_task && current_task->preempt_count > 0) {
        current_task->preempt_count--;
    }
    interrupt_restore(flags);
}

task_t* task_fork(InterruptReg_t *regs) {
    uint32_t flags = interrupt_save();
    cleanup_zombies();

    task_t* parent = current_task;
    task_t* child = kmalloc(sizeof(task_t));
    if (!child) {
        interrupt_restore(flags);
        return NULL;
    }

    child->id = next_tid++;
    child->state = TASK_READY;
    child->preempt_count = 0;
    child->privilege_level = parent->privilege_level;
    child->heap_start = parent->heap_start;
    child->heap_break = parent->heap_break;
    child->heap_mapped_end = parent->heap_mapped_end;

    // Clone the page directory
    uint32_t* child_pd_virt = memory_clone_pagedir();
    if (!child_pd_virt) {
        kfree(child);
        interrupt_restore(flags);
        return NULL;
    }
    child->page_directory = (uint32_t)child_pd_virt - KERNEL_START;

    // Allocate kernel stack for the child
    uint32_t stack_size = 4096;
    void* stack = kmalloc(stack_size);
    if (!stack) {
        // memory_destroy_pagedir(child_pd_virt); // Not implemented yet
        kfree(child);
        interrupt_restore(flags);
        return NULL;
    }
    child->stack_limit = stack;

    // Initialize file table and copy from parent
    if (task_init_file_table(child) < 0) {
        kfree(stack);
        kfree(child);
        interrupt_restore(flags);
        return NULL;
    }
    task_file_table_copy(child, parent);

    // Set up the child's kernel stack to return to the same point as the parent
    uint32_t* stack_ptr = (uint32_t*)((uint32_t)stack + stack_size);

    // iret frame for user mode if necessary
    if (child->privilege_level == 3) {
        *(--stack_ptr) = regs->ss;
        *(--stack_ptr) = regs->useresp;
    }

    *(--stack_ptr) = regs->eflags;
    *(--stack_ptr) = regs->cs; // CS
    *(--stack_ptr) = regs->eip;

    // pushad frame
    *(--stack_ptr) = 0; // EAX: Return value for the child is 0
    *(--stack_ptr) = regs->ecx;
    *(--stack_ptr) = regs->edx;
    *(--stack_ptr) = regs->ebx;
    *(--stack_ptr) = (uint32_t)stack_ptr; // ESP (dummy)
    *(--stack_ptr) = regs->ebp;
    *(--stack_ptr) = regs->esi;
    *(--stack_ptr) = regs->edi;

    // segment registers
    *(--stack_ptr) = regs->ds;
    *(--stack_ptr) = regs->es;
    *(--stack_ptr) = regs->fs;
    *(--stack_ptr) = regs->gs;

    child->kernel_stack = stack_ptr;
    child->regs = (registers_t*)stack_ptr;

    // Add to the ready queue
    child->next = current_task->next;
    current_task->next = child;

    interrupt_restore(flags);
    return child;
}

int task_execve(const char* path, char *const argv[], char *const envp[], InterruptReg_t *regs) {
    (void)envp; // envp not supported yet
    int fd = vfs_open(path, 0);
    if (fd < 0) return -1;

    uint32_t size = vfs_get_size(fd);
    void* elf_data = kmalloc(size);
    if (!elf_data) {
        vfs_close(fd);
        return -1;
    }

    vfs_read(fd, elf_data, size);
    vfs_close(fd);

    Elf32_Ehdr *hdr = (Elf32_Ehdr *)elf_data;
    if (!elf_check_supported(hdr)) {
        kfree(elf_data);
        return -1;
    }

    // Capture arguments in kernel memory before we switch page directory
    int argc = 0;
    char** kargv = NULL;
    if (argv) {
        while (argv[argc]) argc++;
        kargv = kmalloc(sizeof(char*) * (argc + 1));
        for (int i = 0; i < argc; i++) {
            kargv[i] = kmalloc(strlen(argv[i]) + 1);
            strcpy(kargv[i], argv[i]);
        }
        kargv[argc] = NULL;
    }

    uint32_t flags = interrupt_save();

    // Create a new page directory for the new process image
    uint32_t* new_pd_virt = memory_create_user_pagedir();
    if (!new_pd_virt) {
        if (kargv) {
            for (int i = 0; i < argc; i++) kfree(kargv[i]);
            kfree(kargv);
        }
        kfree(elf_data);
        interrupt_restore(flags);
        return -1;
    }
    uint32_t new_pd_phys = (uint32_t)new_pd_virt - KERNEL_START;

    // Switch to the new page directory temporarily to load segments
    uint32_t old_pd_phys = current_task->page_directory;
    memory_set_pagedir(new_pd_virt);

    if (current_task->privilege_level == 3) {
        // Map user stack at 0xB0000000
        uint32_t ustack_phys = pmm_alloc_page_frame();
        memory_map_page(0xB0000000, ustack_phys, PAGE_FLAG_USER | PAGE_FLAG_WRITE | PAGE_FLAG_PRESENT);
    }

    // Load segments into the new page directory
    uint32_t heap_start = 0;
    void* entry = elf_load_segments(hdr, elf_data, current_task->privilege_level, &heap_start);
    kfree(elf_data);

    if (!entry) {
        // Restore old page directory before returning error
        memory_set_pagedir((uint32_t*)(old_pd_phys + KERNEL_START));
        if (kargv) {
            for (int i = 0; i < argc; i++) kfree(kargv[i]);
            kfree(kargv);
        }
        interrupt_restore(flags);
        return -1;
    }

    // Copy arguments to the user stack
    uint32_t user_esp = 0xB0000000 + 4096;
    uint32_t* uargv_ptrs = NULL;

    if (argc > 0) {
        // Allocate space for the pointers to strings on the stack
        // We'll place strings first, then the pointers
        uint32_t* uargv = kmalloc(sizeof(uint32_t) * (argc + 1));

        for (int i = argc - 1; i >= 0; i--) {
            size_t slen = strlen(kargv[i]) + 1;
            user_esp -= slen;
            memcpy((void*)user_esp, kargv[i], slen);
            uargv[i] = user_esp;
        }
        uargv[argc] = 0;

        // Align ESP
        user_esp &= ~3;

        // Push uargv array (the pointers)
        for (int i = argc; i >= 0; i--) {
            user_esp -= 4;
            *(uint32_t*)user_esp = uargv[i];
        }
        uargv_ptrs = (uint32_t*)user_esp;
        kfree(uargv);

        // Free kernel buffers
        for (int i = 0; i < argc; i++) kfree(kargv[i]);
        kfree(kargv);
    }

    // Update the task's page directory address
    current_task->page_directory = new_pd_phys;

    if (current_task->privilege_level == 3) {
        if (task_init_user_heap(current_task, heap_start, true) < 0) {
            memory_set_pagedir((uint32_t*)(old_pd_phys + KERNEL_START));
            interrupt_restore(flags);
            return -1;
        }
    } else {
        current_task->heap_start = 0;
        current_task->heap_break = 0;
        current_task->heap_mapped_end = 0;
    }

    // Final stack setup for crt0: argc, argv
    user_esp -= 4;
    *(uint32_t*)user_esp = (uint32_t)uargv_ptrs;
    user_esp -= 4;
    *(uint32_t*)user_esp = (uint32_t)argc;

    // Modify the interrupt frame to return to the new entry point
    regs->eip = (uint32_t)entry;
    regs->ebx = 0;
    regs->ecx = 0;
    regs->edx = 0;
    regs->esi = 0;
    regs->edi = 0;
    regs->ebp = 0;

    if (current_task->privilege_level == 3) {
        regs->useresp = user_esp;
    }

    interrupt_restore(flags);
    return 0;
}

void task_yield(InterruptReg_t *regs) {
    uint32_t flags = interrupt_save();
    cleanup_zombies();

    if (regs && regs->intr_num == 32 && current_task && current_task->preempt_count > 0) {
        pic_send_eoi(regs->intr_num);
        regs->intr_num = 0;
        interrupt_restore(flags);
        return;
    }

    task_t* last = current_task;
    task_t* next = last->next;
    while (next != last && next->state != TASK_READY) {
        next = next->next;
    }

    if (next == last && next->state != TASK_READY) {
        if (regs && regs->intr_num >= 32) {
            pic_send_eoi(regs->intr_num);
            regs->intr_num = 0;
        }
        interrupt_restore(flags);
        return;
    }

    if (next == last) {
        current_task->state = TASK_RUNNING;
        if (regs && regs->intr_num >= 32) {
            pic_send_eoi(regs->intr_num);
            regs->intr_num = 0;
        }
        interrupt_restore(flags);
        return;
    }

    current_task = next;

    if (scheduler_trigger_index != -1) {
        set_trigger_ticks(scheduler_trigger_index, 0);
    }

    if (last->state == TASK_RUNNING) {
        last->state = TASK_READY;
    }
    current_task->state = TASK_RUNNING;

    uint32_t intr_num = 0;
    if (regs && regs->intr_num >= 32) {
        intr_num = regs->intr_num;
        // Prevent calling handler from sending EOI again
        regs->intr_num = 0;
    }

    if (current_task->stack_limit) {
        tss_set_kernel_stack((uint32_t)current_task->stack_limit + 4096); // 4096 is stack_size
    }

    context_switch(&last->kernel_stack, current_task->kernel_stack, current_task->page_directory, intr_num);

    interrupt_restore(flags);
}

void task_block_current() {
    if (!current_task) return;

    uint32_t flags = interrupt_save();
    current_task->state = TASK_BLOCKED;
    interrupt_restore(flags);

    task_yield(NULL);
}

void task_unblock(task_t* task) {
    if (!task) return;

    uint32_t flags = interrupt_save();
    if (task->state == TASK_BLOCKED) {
        task->state = TASK_READY;
    }
    interrupt_restore(flags);
}

void task_exit() {
    uint32_t flags = interrupt_save();
    // printf("Task %d exiting...\n", current_task->id);

    // Remove current_task from the circular list
    task_t* prev = current_task;
    uint32_t guard = 0;
    while (prev->next != current_task) {
        prev = prev->next;
        if (++guard > next_tid) {
            printf("Task list corruption detected during exit of PID %d.\n", current_task ? current_task->id : 0);
            interrupt_restore(flags);
            for (;;) {
                __asm__ volatile("sti; hlt");
            }
        }
    }

    if (prev == current_task) {
        // Last task exiting
        printf("No more tasks. Halting system.\n");
        interrupt_restore(flags);
        while(1) {
            __asm__ volatile("sti; hlt");
        }
    }

    prev->next = current_task->next;
    task_t* exiting_task = current_task;
    current_task = current_task->next;

    // Add to zombie list to be cleaned up by the next task that runs
    exiting_task->next = zombie_list;
    zombie_list = exiting_task;

    current_task->state = TASK_RUNNING;

    if (current_task->stack_limit) {
        tss_set_kernel_stack((uint32_t)current_task->stack_limit + 4096);
    }

    uint32_t* dummy_esp = 0;

    context_switch(&dummy_esp, current_task->kernel_stack, current_task->page_directory, 0);

    // We should never reach here, but in case we do somehow
    interrupt_restore(flags);
}

void task_stop_scheduler() {
    if (scheduler_trigger_index != -1) {
        set_trigger_flags(scheduler_trigger_index, get_trigger_flags(scheduler_trigger_index) & ~TIMER_TRIGGER_ACTIVE);
    }
}

void task_start_scheduler() {
    if (scheduler_trigger_index != -1) {
        set_trigger_ticks(scheduler_trigger_index, 0);
        set_trigger_flags(scheduler_trigger_index, get_trigger_flags(scheduler_trigger_index) | TIMER_TRIGGER_ACTIVE);
    }
}

void task_switch_to(task_t* next) {
    if (!next || next == current_task) return;

    uint32_t flags = interrupt_save();

    task_t* last = current_task;
    current_task = next;

    if (last->state == TASK_RUNNING) {
        last->state = TASK_READY;
    }
    current_task->state = TASK_RUNNING;

    if (current_task->stack_limit) {
        tss_set_kernel_stack((uint32_t)current_task->stack_limit + 4096);
    }

    // Reset scheduler trigger so it doesn't fire immediately after re-enable
    if (scheduler_trigger_index != -1) {
        set_trigger_ticks(scheduler_trigger_index, 0);
    }

    context_switch(&last->kernel_stack, current_task->kernel_stack, current_task->page_directory, 0);

    interrupt_restore(flags);
}

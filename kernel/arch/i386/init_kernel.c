#include "kernel/tty.h"
#include "gdt.h"
#include "idt.h"
#include "ktask.h"
#include "timer.h"
#include "keyboard.h"
#include <stdint.h>
#include <stdio.h>
#include "memory.h"
#include "kmalloc.h"

extern uint32_t _kernel_end;

/*
 * init_kernel
 * The main entry point for C kernel initialization.
 * Called from boot.asm after stack setup.
 */
void init_kernel(uint32_t magic, MultiBootInfo* boot_info) {
    (void)magic; // Suppress unused parameter warning

    // Convert boot_info to virtual address (higher half)
    // GRUB passes a physical address, which is currently identity mapped.
    // We must use the virtual address once identity mapping is removed.
    boot_info = (MultiBootInfo*)((uint32_t)boot_info + KERNEL_START);

    // Initialize core kernel subsystems
    init_terminal();
    init_gdt();
    init_idt();
    init_keyboard();

    {
        /*
        * Memory Management Initialization
        *
        * 1. Calculate the end of the last loaded module (e.g., initrd).
        * If no modules are present, we use the end of the kernel image.
        */
        uint32_t module_end = (uint32_t)&_kernel_end;

        // Convert to physical address if it's a virtual address from the linker
        if (module_end >= KERNEL_START) {
            module_end -= KERNEL_START;
        }

        if (boot_info->flags & (1 << 3) && boot_info->mods_count > 0) {
            // mods_addr is a physical address
            uint32_t* mods = (uint32_t*)(boot_info->mods_addr + KERNEL_START);
            uint32_t first_mod_end = mods[1]; // end address of the first module
            if (first_mod_end > module_end) {
                module_end = first_mod_end;
            }
        }

        /*
        * 2. Align the start of free physical memory to the next 4KB page boundary.
        */
        uint32_t phys_alloc_start = (module_end + 0xFFF) & ~0xFFF;

        // 3. Initialize the physical memory manager and paging system.
        // boot_info->mem_upper is KB above 1MB. Total memory = 1024 + mem_upper.
        init_memory(1024 + boot_info->mem_upper, phys_alloc_start);
    }

    // 4. Initialize the kernel heap (1MB initial size).
    kmalloc_init(1024 * 1024);


    // 5. init Kernel Task Scheduler
    printf("\nInitializing Multitasking...\n");
    ktask_init_scheduler();

    printf("Memory allocation initialized.\n");
}

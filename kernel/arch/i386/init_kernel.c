#include "kernel/tty.h"
#include "gdt.h"
#include "idt.h"
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

    // Initialize core kernel subsystems
    init_terminal();
    init_gdt();
    init_idt();
    init_timer();
    init_keyboard();

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
        // mods_addr + 4 is the end address of the first module
        uint32_t first_mod_end = *(uint32_t*)(boot_info->mods_addr + 4);
        if (first_mod_end > module_end) {
            module_end = first_mod_end;
        }
    }

    /*
     * 2. Align the start of free physical memory to the next 4KB page boundary.
     * This ensures we don't overwrite the module data and start on a clean page.
     */
    uint32_t phys_alloc_start = (module_end + 0xFFF) & ~0xFFF;

    // 3. Initialize the physical memory manager and paging system.
    // We pass the size of upper memory (in bytes) and the starting physical address.
    init_memory(boot_info->mem_upper * 1024, phys_alloc_start);


    // 4. Initialize the kernel heap (1MB initial size).
    kmalloc_init(1024 * 1024);

    printf("Memory allocation initialized.\n");
}

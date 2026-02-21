#include "kernel/tty.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "keyboard.h"
#include <stdint.h>
#include <stdio.h>
#include "memory.h"

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
     * The multiboot header provides the address of the modules structure.
     * We access the `mod_end` field of the first module to find where free memory starts.
     */
    uint32_t module_end = *(uint32_t*)(boot_info->mods_addr + 4);

    /*
     * 2. Align the start of free physical memory to the next 4KB page boundary.
     * This ensures we don't overwrite the module data and start on a clean page.
     */
    uint32_t phys_alloc_start = (module_end + 0xFFF) & ~0xFFF;

    // 3. Initialize the physical memory manager and paging system.
    // We pass the size of upper memory (in bytes) and the starting physical address.
    init_memory(boot_info->mem_upper * 1024, phys_alloc_start);
}
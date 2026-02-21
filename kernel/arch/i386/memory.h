#ifndef ARCH_I386_MEMORY_H
#define ARCH_I386_MEMORY_H

#include "multiboot.h"
#include <stdint.h>
#include <stddef.h>

/*
 * Memory Management Constants
 */

// Virtual address where the kernel is mapped (Higher Half Kernel)
#define KERNEL_START 0xC0000000

// Page Table/Directory Entry Flags
#define PAGE_FLAG_PRESENT (1 << 0) // Page is present in memory
#define PAGE_FLAG_WRITE   (1 << 1) // Page is writable

/*
 * External Variables
 */

// The initial page directory defined in boot.asm
extern uint32_t initial_page_dir[1024];

/*
 * Memory Management Functions
 */

/**
 * init_memory - Initialize the memory management subsystem.
 * @mem_upper_kb: The amount of upper memory (above 1MB) in Kilobytes.
 * @physical_alloc_start: The physical address where free memory allocation begins.
 */
void init_memory(uint32_t mem_upper_kb, uint32_t physical_alloc_start);

/**
 * flush_tlb_entry - Invalidate a Translation Lookaside Buffer (TLB) entry.
 * @vaddr: The virtual address to invalidate.
 * 
 * Flushes the TLB entry for the specified virtual address using the `invlpg`
 * instruction.
 */
void flush_tlb_entry(uint32_t vaddr);

#endif // ARCH_I386_MEMORY_H

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

// Virtual address for the kernel heap (3.25GB)
#define KERNEL_MALLOC 0xD0000000

// Page Size (4KB)
#define PAGE_SIZE 4096

// Recursive Page Directory/Table Access
#define RECURSIVE_PAGE_DIR   ((uint32_t*)0xFFFFF000)
#define RECURSIVE_PAGE_TABLE(i) ((uint32_t*)(0xFFC00000 + ((i) << 12)))

// Page Table/Directory Entry Flags
#define PAGE_FLAG_PRESENT (1 << 0) // Page is present in memory
#define PAGE_FLAG_WRITE   (1 << 1) // Page is writable
#define PAGE_FLAG_OWNER   (1 << 9) // Software flag: Page owned by current ktask/kernel

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

/**
 * pmm_alloc_page_frame - Allocate a single physical page frame.
 * 
 * Scans the physical memory bitmap for a free page frame, marks it as used,
 * and returns its physical address.
 * 
 * Return: The physical address of the allocated frame, or 0 if out of memory.
 */
uint32_t pmm_alloc_page_frame(void);

/**
 * memory_map_page - Map a virtual page to a physical frame.
 * @virtual_addr: The virtual address to map.
 * @phys_addr: The physical address to map to.
 * @flags: Page table flags (e.g., PAGE_FLAG_WRITE).
 * 
 * Updates the page tables to map the specified virtual address to the physical
 * address. Allocates new page tables if necessary.
 */
void memory_map_page(uint32_t virtual_addr, uint32_t phys_addr, uint32_t flags);

/**
 * memory_get_current_pagedir - Get the current page directory physical address.
 * 
 * Return: The linear address of the current page directory.
 */
uint32_t* memory_get_current_pagedir(void);

/**
 * memory_set_pagedir - Switch to a new page directory.
 * @pd: The physical address of the new page directory.
 */
void memory_set_pagedir(uint32_t* pd);

#endif // ARCH_I386_MEMORY_H

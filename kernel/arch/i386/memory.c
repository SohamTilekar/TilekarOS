#include "memory.h"
#include <stdbool.h>
#include <string.h>
#include "utils.h"
#include <stddef.h>

/*
 * Physical Memory Manager (PMM) Configuration
 *
 * We use a bitmap to track free/allocated physical frames.
 * Each bit corresponds to one 4KB page frame.
 *
 * Max RAM = 4GB (0x100000000)
 * Total Frames = 4GB / 4KB = 1,048,576 frames
 * Bitmap Size = 1,048,576 / 8 bits per byte = 131,072 bytes (128KB)
 */
#define TOTAL_PAGE_FRAMES (0x100000000 / PAGE_SIZE)
#define BITMAP_SIZE_BYTES (TOTAL_PAGE_FRAMES / 8)

static uint8_t physical_memory_bitmap[BITMAP_SIZE_BYTES];

static uint32_t page_frame_min; // Lowest frame index available for allocation
static uint32_t page_frame_max; // Highest frame index available (based on RAM size)
static uint32_t total_allocated; // Counter for allocated frames

static uint32_t total_mapped_pages; // Counter for mapped virtual pages

/*
 * Page Table Management
 *
 * We reserve a pool of page tables to be used by the kernel.
 * Each Page Directory Entry (PDE) points to a Page Table.
 * A Page Table contains 1024 Page Table Entries (PTEs).
 */
#define NUM_PAGE_DIRS 256
static uint32_t page_tables[NUM_PAGE_DIRS][1024] __attribute__((aligned(4096)));
static uint8_t page_tables_used[NUM_PAGE_DIRS];

/**
 * init_pmm - Initialize the Physical Memory Manager (PMM).
 * @mem_low: The starting physical address of free memory.
 * @mem_high: The ending physical address of available memory.
 *
 * Marks the range of available memory based on the boot information.
 */
static void init_pmm(uint32_t mem_low, uint32_t mem_high) {
    page_frame_min = CEIL_DIV(mem_low, PAGE_SIZE);
    page_frame_max = mem_high / PAGE_SIZE;
    total_allocated = 0;

    // Clear the bitmap (all frames free by default)
    // In a real PMM, we should mark kernel space as used first.
    memset(physical_memory_bitmap, 0, sizeof(physical_memory_bitmap));
}

/*
 * flush_tlb_entry - Invalidate a TLB entry
 * @vaddr: Virtual address to invalidate
 */
void flush_tlb_entry(uint32_t vaddr) {
    asm volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
}

uint32_t* memory_get_current_pagedir(void) {
    uint32_t pd_phys;
    asm volatile("mov %%cr3, %0": "=r"(pd_phys));
    
    // Convert physical address to virtual address
    // This assumes the kernel is mapped at KERNEL_START
    return (uint32_t*)(pd_phys + KERNEL_START);
}

void memory_set_pagedir(uint32_t* pd) {
    // Convert virtual address to physical address
    uint32_t pd_phys = (uint32_t)pd - KERNEL_START;
    asm volatile("mov %0, %%cr3" :: "r"(pd_phys));
}

#define MAX_ACTIVE_PAGEDIRS 256
static uint32_t active_pagedirs[MAX_ACTIVE_PAGEDIRS];
static int active_pagedir_count = 0;

uint32_t* memory_create_user_pagedir(void) {
    uint32_t pd_phys = pmm_alloc_page_frame();
    if (!pd_phys) return NULL;

    // We temporarily map the new page directory to 0xE0000000 so we can initialize it.
    memory_map_page(0xE0000000, pd_phys, PAGE_FLAG_WRITE | PAGE_FLAG_PRESENT);
    uint32_t* pd = (uint32_t*)0xE0000000;

    // Clear the lower 3GB (user space PDEs)
    memset(pd, 0, 768 * sizeof(uint32_t));

    // Copy the upper 1GB (kernel space PDEs) from initial_page_dir
    for (int i = 768; i < 1024; i++) {
        pd[i] = initial_page_dir[i];
    }

    // Set up recursive mapping for the new directory
    pd[1023] = pd_phys | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;

    // Unmap the temporary mapping
    uint32_t pd_index = 0xE0000000 >> 22;
    uint32_t pt_index = (0xE0000000 >> 12) & 0x3FF;
    uint32_t* pt = RECURSIVE_PAGE_TABLE(pd_index);
    pt[pt_index] = 0;
    flush_tlb_entry(0xE0000000);

    if (active_pagedir_count < MAX_ACTIVE_PAGEDIRS) {
        active_pagedirs[active_pagedir_count++] = pd_phys;
    }

    return (uint32_t*)(pd_phys + KERNEL_START);
}

static void memory_sync_pagedirs(void) {
    uint32_t flags = interrupt_save();
    
    // We use PDE 1022 of initial_page_dir to temporarily map other page directories
    // PDE 1022 corresponds to virtual address 0xFF800000.
    // When used as a Page Table via recursive mapping, its contents are visible at 0xFFFFE000.
    for (int i = 0; i < active_pagedir_count; i++) {
        uint32_t pd_phys = active_pagedirs[i];
        
        initial_page_dir[1022] = pd_phys | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
        flush_tlb_entry(0xFFFFE000); 
        
        uint32_t* target_pd = (uint32_t*)0xFFFFE000;
        
        // Copy kernel PDEs (768 to 1021).
        // 1022 is our temporary mapping, skip it.
        // 1023 is the recursive mapping, keep it untouched.
        for (int j = 768; j < 1022; j++) {
            target_pd[j] = initial_page_dir[j];
        }
    }
    
    // Clear the temporary mapping
    initial_page_dir[1022] = 0;
    flush_tlb_entry(0xFFFFE000);
    
    interrupt_restore(flags);
}

uint32_t pmm_alloc_page_frame(void) {
    uint32_t flags = interrupt_save();

    // Calculate start/end bytes in the bitmap
    uint32_t start_byte = page_frame_min / 8;
    uint32_t end_byte = (page_frame_max + 7) / 8;

    for (uint32_t b = start_byte; b < end_byte; b++) {
        uint8_t byte = physical_memory_bitmap[b];
        
        // Skip if all bits set (full)
        if (byte == 0xFF) {
            continue;
        }

        for (uint32_t i = 0; i < 8; i++) {
            // Check if bit 'i' is used (1) or free (0)
            bool used = (byte >> i) & 1;

            if (!used) {
                uint32_t frame_index = b * 8 + i;
                
                // Ensure we don't go past the max or below the min
                if (frame_index < page_frame_min) continue;
                if (frame_index >= page_frame_max) break;

                // Mark as used
                physical_memory_bitmap[b] |= (1 << i);
                total_allocated++;

                interrupt_restore(flags);
                return frame_index * PAGE_SIZE;
            }
        }
    }

    interrupt_restore(flags);
    return 0; // Out of memory
}

void memory_map_page(uint32_t virtual_addr, uint32_t phys_addr, uint32_t flags) {
    uint32_t intr_flags = interrupt_save();

    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

    uint32_t* page_dir = RECURSIVE_PAGE_DIR;
    uint32_t* pt = RECURSIVE_PAGE_TABLE(pd_index);

    // If mapping kernel space, ensure we are using the initial page directory
    // to keep global kernel mappings consistent.
    uint32_t* prev_pagedir = NULL;
    if (virtual_addr >= KERNEL_START) {
        prev_pagedir = memory_get_current_pagedir();
        if (prev_pagedir != initial_page_dir) {
            memory_set_pagedir(initial_page_dir);
        }
    }

    // Check if the page table is present in the directory
    if (!(page_dir[pd_index] & PAGE_FLAG_PRESENT)) {
        // Allocate a new page table
        uint32_t pt_phys = pmm_alloc_page_frame();
        if (!pt_phys) {
            interrupt_restore(intr_flags);
            return; // OOM
        }

        // Map the new page table into the directory
        page_dir[pd_index] = pt_phys | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE | (flags & PAGE_FLAG_OWNER) | (flags & PAGE_FLAG_USER);
        
        flush_tlb_entry((uint32_t)pt);

        // Clear the new page table
        memset(pt, 0, PAGE_SIZE);
    } else if (page_dir[pd_index] & (1 << 7)) {
        if (prev_pagedir != NULL && prev_pagedir != initial_page_dir) {
            memory_set_pagedir(prev_pagedir);
        }
        interrupt_restore(intr_flags);
        return;
    }

    // Map the page
    pt[pt_index] = phys_addr | PAGE_FLAG_PRESENT | flags;
    total_mapped_pages++;
    flush_tlb_entry(virtual_addr);

    // Restore previous page directory if we switched
    if (prev_pagedir != NULL && prev_pagedir != initial_page_dir) {
        memory_sync_pagedirs();
        memory_set_pagedir(prev_pagedir);
    }

    interrupt_restore(intr_flags);
}


/*
 * init_memory - Initialize the memory system (Paging + PMM)
 * @mem_total_kb: Total available RAM in KB
 * @physical_alloc_start: Physical address where free allocation starts
 */
void init_memory(uint32_t mem_total_kb, uint32_t physical_alloc_start) {
    // 1. Unmap the identity mapping of the first 4MB (0-4MB).
    total_mapped_pages = 0;
    initial_page_dir[0] = 0;
    flush_tlb_entry(0);

    // 2. Set up recursive page directory mapping.
    uint32_t pd_phys_addr = ((uint32_t)initial_page_dir - KERNEL_START);
    initial_page_dir[1023] = pd_phys_addr | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
    flush_tlb_entry(0xFFFFF000);

    // 3. Initialize Physical Memory Manager.
    // mem_total_kb is total memory including first 1MB.
    init_pmm(physical_alloc_start, mem_total_kb * 1024);

    // 4. Clear the pre-allocated page table pool.
    memset(page_tables, 0, sizeof(page_tables));
    memset(page_tables_used, 0, sizeof(page_tables_used));
}


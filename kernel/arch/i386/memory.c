#include "memory.h"
#include <string.h>
#include "utils.h"
#include <stddef.h>

/*
 * Paging Constants
 */
#define PAGE_SIZE 4096 // 4KB

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
    asm volatile("invlpg %0" :: "m"(vaddr));
}

/*
 * init_memory - Initialize the memory system (Paging + PMM)
 * @mem_upper_kb: Size of upper memory in KB (from Multiboot)
 * @physical_alloc_start: Physical address where free allocation starts
 */
void init_memory(uint32_t mem_upper_kb, uint32_t physical_alloc_start) {
    // 1. Unmap the identity mapping of the first 4MB (0-4MB).
    // The bootloader/assembly code identity maps the first 4MB so the kernel can run
    // before paging is fully set up. We remove this to catch NULL pointer dereferences.
    initial_page_dir[0] = 0;
    flush_tlb_entry(0);

    // 2. Set up recursive page directory mapping.
    // The last entry (1023) points to the page directory itself.
    // This allows accessing page tables via virtual addresses at the end of memory.
    // KERNEL_START is 0xC0000000 (3GB).
    uint32_t pd_phys_addr = ((uint32_t)initial_page_dir - KERNEL_START);
    initial_page_dir[1023] = pd_phys_addr | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
    
    // Invalidate the TLB for the recursive mapping address.
    // The recursive mapping is at virtual address 0xFFFFF000.
    flush_tlb_entry(0xFFFFF000);

    // 3. Initialize Physical Memory Manager.
    // mem_upper_kb is in KB. Total memory is roughly mem_upper + 1MB.
    // We pass the upper memory size directly as the high mark for now.
    init_pmm(physical_alloc_start, mem_upper_kb);

    // 4. Clear the pre-allocated page table pool.
    memset(page_tables, 0, sizeof(page_tables));
    memset(page_tables_used, 0, sizeof(page_tables_used));
}
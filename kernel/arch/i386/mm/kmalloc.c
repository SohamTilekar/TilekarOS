#include "kmalloc.h"
#include "utils.h"
#include "memory.h"
#include <string.h>
#include <stdbool.h>

#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define HEADER_SIZE (sizeof(block_header_t))

typedef struct block_header {
    size_t size;            // Payload size (excluding header)
    bool is_free;
    struct block_header *next;
    struct block_header *prev;
} block_header_t;

static block_header_t *head = NULL;
static void *heap_end = NULL;
static uintptr_t heap_mapped_top = 0;

static void* heap_sbrk(intptr_t increment);

void kmalloc_init(size_t initial_size) {
    heap_end = (void*)KERNEL_MALLOC;
    heap_mapped_top = KERNEL_MALLOC;
    head = NULL;

    if (initial_size > 0) {
        initial_size = ALIGN(initial_size, 4096);
        void* initial_block = heap_sbrk(initial_size);
        if (initial_block != (void*)-1) {
            head = (block_header_t*)initial_block;
            head->size = initial_size - HEADER_SIZE;
            head->is_free = true;
            head->next = NULL;
            head->prev = NULL;
        }
    }
}

static void* heap_sbrk(intptr_t increment) {
    if (increment == 0) return heap_end;

    uint32_t flags = interrupt_save();

    uintptr_t old_break = (uintptr_t)heap_end;
    uintptr_t new_break = old_break + increment;

    if (new_break > heap_mapped_top) {
        uintptr_t new_mapped_top = ALIGN(new_break, PAGE_SIZE);

        for (uintptr_t addr = heap_mapped_top; addr < new_mapped_top; addr += PAGE_SIZE) {
            uint32_t phys = pmm_alloc_page_frame();
            if (!phys) {
                interrupt_restore(flags);
                return (void*)-1; // Out of memory
            }
            memory_map_page(addr, phys, PAGE_FLAG_WRITE | PAGE_FLAG_PRESENT | PAGE_FLAG_OWNER);
        }
        heap_mapped_top = new_mapped_top;
    }

    heap_end = (void*)new_break;
    interrupt_restore(flags);
    return (void*)old_break;
}

void* kmalloc(size_t size) {
    if (size == 0) return NULL;

    size = ALIGN(size, 8);

    uint32_t flags = interrupt_save();

    block_header_t *current = head;
    block_header_t *last = NULL;

    // First fit search
    while (current) {
        if (current->is_free && current->size >= size) {
            // Found a free block. Can we split it?
            if (current->size >= size + HEADER_SIZE + 8) {
                block_header_t *new_block = (block_header_t*)((uint8_t*)current + HEADER_SIZE + size);
                new_block->size = current->size - size - HEADER_SIZE;
                new_block->is_free = true;
                new_block->next = current->next;
                new_block->prev = current;

                if (new_block->next) {
                    new_block->next->prev = new_block;
                }

                current->size = size;
                current->next = new_block;
            }
            current->is_free = false;
            interrupt_restore(flags);
            return (void*)(current + 1);
        }
        last = current;
        current = current->next;
    }

    // No free block found, extend heap.
    if (last && last->is_free) {
        size_t needed = size - last->size;
        void* res = heap_sbrk(needed);
        if (res == (void*)-1) {
            interrupt_restore(flags);
            return NULL;
        }
        last->size = size;
        last->is_free = false;
        interrupt_restore(flags);
        return (void*)(last + 1);
    }

    // Otherwise, allocate a completely new block
    size_t total_size = size + HEADER_SIZE;
    block_header_t *new_block = (block_header_t*)heap_sbrk(total_size);

    if (new_block == (void*)-1) {
        interrupt_restore(flags);
        return NULL; // OOM
    }

    new_block->size = size;
    new_block->is_free = false;
    new_block->next = NULL;
    new_block->prev = last;

    if (last) {
        last->next = new_block;
    } else {
        head = new_block;
    }

    interrupt_restore(flags);
    return (void*)(new_block + 1);
}

void kfree(void* ptr) {
    if (!ptr) return;

    uint32_t flags = interrupt_save();

    block_header_t *block = (block_header_t*)ptr - 1;
    block->is_free = true;

    // Coalesce with next block
    if (block->next && block->next->is_free) {
        block->size += HEADER_SIZE + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    // Coalesce with previous block
    if (block->prev && block->prev->is_free) {
        block->prev->size += HEADER_SIZE + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }

    interrupt_restore(flags);
}


void* kcalloc(size_t nmemb, size_t size) {
    size_t total_size = nmemb * size;
    // Check for overflow
    if (nmemb != 0 && total_size / nmemb != size) return NULL;

    void *ptr = kmalloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

void* krealloc(void* ptr, size_t size) {
    if (!ptr) return kmalloc(size);
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }

    block_header_t *block = (block_header_t*)ptr - 1;
    if (block->size >= size) {
        // We could split here if the new size is much smaller, but for now just return same ptr
        return ptr;
    }

    void *new_ptr = kmalloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, block->size);
        kfree(ptr);
    }
    return new_ptr;
}

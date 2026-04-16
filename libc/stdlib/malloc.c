#include <stdlib.h>
#include <sys/syscall.h>
#include "malloc_internal.h"

#define MALLOC_ALIGNMENT 8U
#define MALLOC_SPLIT_MIN 8U

typedef struct block_header {
    uint32_t size;
    uint32_t free;
} block_header_t;

static block_header_t* heap_base = NULL;

static uint32_t align_up(uint32_t value) {
    return (value + (MALLOC_ALIGNMENT - 1U)) & ~(MALLOC_ALIGNMENT - 1U);
}

int brk(void* addr) {
    uint32_t requested = (uint32_t)(uintptr_t)addr;
    uint32_t ret = __syscall(SYS_BRK, requested, 0, 0, 0, 0);
    if (ret == (uint32_t)-1 || ret != requested) {
        return -1;
    }
    return 0;
}

void* sbrk(intptr_t increment) {
    uint32_t old_break = __syscall(SYS_BRK, 0, 0, 0, 0, 0);
    if (old_break == (uint32_t)-1) {
        return (void*)-1;
    }

    uint32_t new_break = old_break;
    if (increment > 0) {
        uint32_t inc = (uint32_t)increment;
        if (new_break > UINT32_MAX - inc) {
            return (void*)-1;
        }
        new_break += inc;
    } else if (increment < 0) {
        uint32_t dec = (uint32_t)(-increment);
        if (dec > new_break) {
            return (void*)-1;
        }
        new_break -= dec;
    }

    uint32_t ret = __syscall(SYS_BRK, new_break, 0, 0, 0, 0);
    if (ret == (uint32_t)-1 || ret != new_break) {
        return (void*)-1;
    }
    return (void*)(uintptr_t)old_break;
}

static block_header_t* next_block(block_header_t* block) {
    return (block_header_t*)((uint8_t*)(block + 1) + block->size);
}

static block_header_t* find_fit(uint32_t aligned_size) {
    if (!heap_base) return NULL;

    uintptr_t heap_end = (uintptr_t)sbrk(0);
    if (heap_end == (uintptr_t)-1) return NULL;

    block_header_t* curr = heap_base;
    while ((uintptr_t)curr + sizeof(block_header_t) <= heap_end) {
        uintptr_t block_end = (uintptr_t)curr + sizeof(block_header_t) + curr->size;
        if (block_end > heap_end) {
            return NULL;
        }
        if (curr->free && curr->size >= aligned_size) {
            return curr;
        }
        curr = next_block(curr);
    }
    return NULL;
}

static void split_block(block_header_t* block, uint32_t aligned_size) {
    if (!block) return;
    if (block->size <= aligned_size + sizeof(block_header_t) + MALLOC_SPLIT_MIN) return;

    block_header_t* split = (block_header_t*)((uint8_t*)(block + 1) + aligned_size);
    split->size = block->size - aligned_size - sizeof(block_header_t);
    split->free = 1;
    block->size = aligned_size;
}

void* malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    if (size > UINT32_MAX - sizeof(block_header_t) - MALLOC_ALIGNMENT) {
        return NULL;
    }

    uint32_t aligned_size = align_up((uint32_t)size);
    if (!heap_base) {
        heap_base = (block_header_t*)sbrk(0);
        if ((uintptr_t)heap_base == (uintptr_t)-1) {
            return NULL;
        }
    }

    block_header_t* block = find_fit(aligned_size);
    if (block) {
        split_block(block, aligned_size);
        block->free = 0;
        return (void*)(block + 1);
    }

    uint32_t needed = sizeof(block_header_t) + aligned_size;
    void* old_break = sbrk((intptr_t)needed);
    if (old_break == (void*)-1) {
        return NULL;
    }

    block = (block_header_t*)old_break;
    block->size = aligned_size;
    block->free = 0;
    return (void*)(block + 1);
}

void __libc_free_impl(void* ptr) {
    if (!ptr) {
        return;
    }

    block_header_t* block = ((block_header_t*)ptr) - 1;
    block->free = 1;

    if (!heap_base) {
        return;
    }

    uintptr_t heap_end = (uintptr_t)sbrk(0);
    if (heap_end == (uintptr_t)-1) {
        return;
    }

    block_header_t* curr = heap_base;
    while ((uintptr_t)curr + sizeof(block_header_t) <= heap_end) {
        uintptr_t curr_end = (uintptr_t)curr + sizeof(block_header_t) + curr->size;
        if (curr_end > heap_end) {
            break;
        }

        block_header_t* next = next_block(curr);
        if (curr_end < heap_end &&
            (uintptr_t)next + sizeof(block_header_t) <= heap_end &&
            curr->free && next->free) {
            curr->size += sizeof(block_header_t) + next->size;
            continue;
        }
        curr = next;
    }

    block_header_t* last = NULL;
    curr = heap_base;
    while ((uintptr_t)curr + sizeof(block_header_t) <= heap_end) {
        uintptr_t curr_end = (uintptr_t)curr + sizeof(block_header_t) + curr->size;
        if (curr_end > heap_end) {
            break;
        }
        last = curr;
        if (curr_end == heap_end) {
            break;
        }
        curr = next_block(curr);
    }

    if (last && last->free) {
        intptr_t shrink = -((intptr_t)(sizeof(block_header_t) + last->size));
        (void)sbrk(shrink);
    }
}

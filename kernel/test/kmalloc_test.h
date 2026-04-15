#ifndef KERNEL_TEST_KMALLOC_TEST_H
#define KERNEL_TEST_KMALLOC_TEST_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "kmalloc.h"
#include "test_utils.h"

static inline void run_kmalloc_tests(test_stats_t* stats) {
    test_print_category(stats, "KMALLOC & ALIGNMENT");

    void* p1 = kmalloc_aligned(128, 4096);
    test_record(stats, p1 != NULL, "kmalloc_aligned(128, 4096) returns non-NULL");
    test_record(stats, ((uintptr_t)p1 & 0xFFF) == 0, "kmalloc_aligned returns 4K aligned address");
    
    if (p1) {
        memset(p1, 0xAA, 128);
        test_record(stats, ((uint8_t*)p1)[0] == 0xAA && ((uint8_t*)p1)[127] == 0xAA, "kmalloc_aligned memory is writable");
    }

    // Try kfree
    kfree(p1);
    test_record(stats, true, "kfree(aligned_ptr) completes without crashing");

    // Try krealloc on aligned pointer
    void* p2 = kmalloc_aligned(64, 4096);
    test_record(stats, p2 != NULL && ((uintptr_t)p2 & 0xFFF) == 0, "kmalloc_aligned(64, 4096) for realloc");
    if (p2) {
        memset(p2, 0xBB, 64);
        void* p3 = krealloc(p2, 256);
        test_record(stats, p3 != NULL, "krealloc on aligned pointer succeeds");
        if (p3) {
            test_record(stats, ((uint8_t*)p3)[0] == 0xBB && ((uint8_t*)p3)[63] == 0xBB, "krealloc preserves data from aligned ptr");
            kfree(p3);
        }
    }
}

#endif

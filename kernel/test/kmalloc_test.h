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
        kfree(p1);
    }

    test_print_category(stats, "KMALLOC STRESS TEST");
    void* ptrs[256];
    bool stress_pass = true;
    for (int i = 0; i < 256; i++) {
        ptrs[i] = kmalloc(16);
        if (!ptrs[i]) {
            stress_pass = false;
            break;
        }
        memset(ptrs[i], i, 16);
    }
    
    if (stress_pass) {
        for (int i = 0; i < 256; i++) {
            if (((uint8_t*)ptrs[i])[0] != (uint8_t)i) stress_pass = false;
            kfree(ptrs[i]);
        }
    }
    test_record(stats, stress_pass, "256 sequential kmalloc/kfree cycles");
}

#endif

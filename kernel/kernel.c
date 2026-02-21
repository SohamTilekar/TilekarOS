#include "stdio.h"
#include <kernel/tty.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "arch/i386/kmalloc.h"
#include "string.h"

void test_kmalloc() {
    printf("Testing kmalloc...\n");

    // Test 1: Simple Allocation & Free
    printf("1. Basic allocation: ");
    void* p1 = kmalloc(128);
    if (p1) {
        printf("PASSED (addr: %x)\n", (uint32_t)p1);
        // kfree(p1);
    } else {
        printf("FAILED\n");
    }

    // Test 2: Reuse
    printf("2. Block reuse: ");
    void* p2 = kmalloc(128);
    if (p2 == p1) {
        printf("PASSED (reused same address)\n");
    } else {
        printf("INFO (different address: %x, expected %x)\n", (uint32_t)p2, (uint32_t)p1);
    }
    // kfree(p2);

    // Test 3: Multiple allocations
    printf("3. Multiple allocations: ");
    void* arr[5];
    bool multi_ok = true;
    for (int i = 0; i < 5; i++) {
        arr[i] = kmalloc(64);
        if (!arr[i]) multi_ok = false;
        // Fill with data
        if (arr[i]) memset(arr[i], i + 1, 64);
    }

    // Verify data
    for (int i = 0; i < 5; i++) {
        if (arr[i]) {
            uint8_t* b = (uint8_t*)arr[i];
            for (int j = 0; j < 64; j++) {
                if (b[j] != i + 1) multi_ok = false;
            }
        }
    }

    if (multi_ok) printf("PASSED\n");
    else printf("FAILED\n");

    // for (int i = 0; i < 5; i++) kfree(arr[i]);

    // Test 4: Heap Expansion
    // Our initial heap is 1MB. Let's try to allocate 2MB total.
    printf("4. Heap expansion (allocating 1.5MB): ");
    void* large = kmalloc(1536 * 1024);
    if (large) {
        printf("PASSED (addr: %x)\n", (uint32_t)large);
        memset(large, 0xAA, 1536 * 1024);
        printf("   Verification: ");
        bool large_ok = true;
        uint8_t* b = (uint8_t*)large;
        if (b[0] != 0xAA || b[1536 * 1024 - 1] != 0xAA) large_ok = false;

        if (large_ok) printf("PASSED\n");
        else printf("FAILED\n");

        // kfree(large);
    } else {
        printf("FAILED (could not extend heap)\n");
    }

    printf("kmalloc test completed.\n");
}

void kernel_main() {
  printf("Hello World!\nPrint On TilekarOS by Soham Tilekar\n");

  test_kmalloc();

  for (;;);
}

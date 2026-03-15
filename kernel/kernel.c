#include "stdio.h"
#include <kernel/tty.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "arch/i386/kmalloc.h"
#include "arch/i386/task.h"
#include "string.h"

void test_kmalloc() {
    printf("Testing kmalloc...\n");

    // Test 1: Simple Allocation & Free
    printf("1. Basic allocation: ");
    void* p1 = kmalloc(128);
    if (p1) {
        printf("PASSED (addr: %x)\n", (uint32_t)p1);
        kfree(p1);
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
    kfree(p2);

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

    for (int i = 0; i < 5; i++) kfree(arr[i]);

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

        kfree(large);
    } else {
        printf("FAILED (could not extend heap)\n");
    }

    printf("kmalloc test completed.\n");
}

extern char _start_user_task;
extern char _end_user_task;

extern char _start_elf_user_task;
extern char _end_elf_user_task;

// Task B: Infinite loop, relies on preemption
void task_B() {
    while (true) {
        printf("B");
        for (volatile int i = 0; i < 50000; i++);
    }
}

// Task C: Finite loop, explicitly yields
void task_C() {
    for (int i = 0; i < 5; i++) {
        printf("C(%d)", i);
        task_yield(NULL);
    }
    printf("[C exits]");
    task_exit();
}

// Task D: Finite loop, relies on preemption, then exits
void task_D() {
    for (int i = 0; i < 3; i++) {
        printf("D(%d)", i);
        for (volatile int j = 0; j < 100000; j++);
    }
    printf("[D exits]");
    task_exit();
}

// Task E: Creates another task and then exits
void task_E_child() {
    printf("[E_child runs & exits]");
    task_exit();
}

void task_E() {
    printf("[E creates child]");
    task_create(task_E_child, 0);
    printf("[E exits]");
    task_exit();
}

void kernel_main(uint32_t magic, void* boot_info) {
  (void)magic; (void)boot_info;
  printf("Hello World!\nPrint On TilekarOS by Soham Tilekar\n");

  test_kmalloc();

  printf("\n--- Multitasking Stress Test ---\n");

  // task_create(task_B, 0);
  // task_create(task_C, 0);
  // task_create(task_D, 0);
  // task_create(task_E, 0);
  // task_create_user(&_start_user_task, &_end_user_task);

  // ELF Task Creation
  uint32_t elf_size = (uint32_t)(&_end_elf_user_task - &_start_elf_user_task);
  printf("Creating ELF task from user_task.elf (size: %u bytes)...\n", elf_size);
  task_create_elf(&_start_elf_user_task);

  printf("Main task (TID 0) entering infinite yield loop.\n");
  while (true) {
      task_yield(NULL);
  }
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

static int tests_run = 0;
static int tests_failed = 0;

void print_block_divider() {
    printf("----------------------------------------------------------------\n");
}

void print_category(const char* name) {
    printf("\n[ %s ]\n", name);
}

void assert_test(int condition, const char* name) {
    tests_run++;
    if (condition) {
        printf("  [PASS] %s\n", name);
    } else {
        printf("  [FAIL] %s\n", name);
        tests_failed++;
    }
}

void test_alignment() {
    print_category("Alignment Test");
    void* p1 = malloc(1);
    void* p2 = malloc(13);
    void* p3 = malloc(64);
    
    assert_test(((uintptr_t)p1 % 8) == 0, "malloc(1) alignment");
    assert_test(((uintptr_t)p2 % 8) == 0, "malloc(13) alignment");
    assert_test(((uintptr_t)p3 % 8) == 0, "malloc(64) alignment");

    free(p1);
    free(p2);
    free(p3);
}

void test_splitting_and_reuse() {
    print_category("Splitting and Reuse Test");
    void* large = malloc(1024);
    uintptr_t large_addr = (uintptr_t)large;
    assert_test(large != NULL, "Large allocation");
    
    free(large);
    
    void* s1 = malloc(128);
    void* s2 = malloc(128);
    
    assert_test(s1 != NULL && s2 != NULL, "Small allocations after large free");
    assert_test((uintptr_t)s1 == large_addr, "Reuse of freed block start");
    assert_test((uintptr_t)s2 > (uintptr_t)s1, "Second block follows first");
    
    free(s1);
    free(s2);
}

void test_coalescing() {
    print_category("Coalescing Test");
    void* p1 = malloc(128);
    void* p2 = malloc(128);
    void* p3 = malloc(128);
    
    uintptr_t base = (uintptr_t)p1;
    free(p1);
    free(p2);
    free(p3);
    
    void* big = malloc(350);
    assert_test(big != NULL, "Allocation in coalesced space");
    assert_test((uintptr_t)big == base, "Large block reused original base address");
    
    free(big);
}

void test_edge_cases() {
    print_category("Edge Cases");
    void* p0 = malloc(0);
    assert_test(p0 == NULL, "malloc(0) returns NULL");
    
    void* p_huge = malloc(0xFFFFFFFF);
    assert_test(p_huge == NULL, "malloc(huge) returns NULL");

    void* c1 = calloc(0, 10);
    assert_test(c1 == NULL, "calloc(0, 10) returns NULL");
    
    void* c2 = calloc(10, 0);
    assert_test(c2 == NULL, "calloc(10, 0) returns NULL");

    void* p1 = malloc(100);
    void* p2 = realloc(p1, 0);
    assert_test(p2 == NULL, "realloc(p, 0) returns NULL (frees)");
    // Note: p1 is already freed by realloc(p1, 0)
}

void test_realloc_expansion() {
    print_category("Realloc Expansion Test");
    void* p1 = malloc(128);
    memset(p1, 0xAA, 128);
    
    void* p2 = realloc(p1, 256);
    assert_test(p2 != NULL, "realloc success");
    
    int data_ok = 1;
    unsigned char* ptr = (unsigned char*)p2;
    for (int i = 0; i < 128; i++) {
        if (ptr[i] != 0xAA) {
            data_ok = 0;
            break;
        }
    }
    assert_test(data_ok, "realloc preserved data");
    
    free(p2);
}

void test_stress() {
    print_category("Stress Test");
    void* ptrs[64];
    int success = 1;
    
    for (int i = 0; i < 64; i++) {
        ptrs[i] = malloc(8 * (i + 1));
        if (!ptrs[i]) {
            success = 0;
            printf("  [FAIL] Failed at allocation %d\n", i);
        } else {
            memset(ptrs[i], i, 8 * (i + 1));
        }
    }
    assert_test(success, "64 sequential allocations");
    
    // Verify data integrity
    success = 1;
    for (int i = 0; i < 64; i++) {
        unsigned char* p = (unsigned char*)ptrs[i];
        for (int j = 0; j < 8 * (i + 1); j++) {
            if (p[j] != (unsigned char)i) {
                success = 0;
                break;
            }
        }
    }
    assert_test(success, "Data integrity check");
    
    // Fragmentation and re-allocation
    for (int i = 0; i < 64; i += 2) {
        free(ptrs[i]);
    }
    
    success = 1;
    for (int i = 0; i < 32; i++) {
        void* p = malloc(4);
        if (!p) success = 0;
        else free(p);
    }
    assert_test(success, "Re-allocation in fragmented heap");
    
    for (int i = 1; i < 64; i += 2) {
        free(ptrs[i]);
    }
}

void write_result(const char* result) {
    mkdir("/tmp");
    int fd = open("/tmp/MALLOC.RES", 0x42); // O_WRONLY | O_CREAT (assuming 0x42 from other parts of OS)
}

int main() {
    print_block_divider();
    printf("USERSpace Malloc Test Suite Starting...\n");

    test_alignment();
    test_splitting_and_reuse();
    test_coalescing();
    test_edge_cases();
    test_realloc_expansion();
    test_stress();

    printf("\nSummary: %d/%d tests passed\n", tests_run - tests_failed, tests_run);
    
    mkdir("/tmp");
    int fd = open("/tmp/MALLOC.RES", 1); // 1 = VFS_O_CREAT
    
    if (tests_failed == 0) {
        printf("RESULT: malloc_test PASS\n");
        if (fd >= 0) {
            write(fd, "PASS", 4);
            close(fd);
        }
        print_block_divider();
        return 0;
    } else {
        printf("RESULT: malloc_test FAIL\n");
        if (fd >= 0) {
            write(fd, "FAIL", 4);
            close(fd);
        }
        print_block_divider();
        return 1;
    }
}

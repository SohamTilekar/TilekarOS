#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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
}

void test_stress() {
    print_category("Stress Test");
    void* ptrs[32];
    int success = 1;
    
    for (int i = 0; i < 32; i++) {
        ptrs[i] = malloc(16 * (i + 1));
        if (!ptrs[i]) success = 0;
    }
    assert_test(success, "32 sequential allocations");
    
    for (int i = 0; i < 32; i += 2) {
        free(ptrs[i]);
    }
    
    for (int i = 0; i < 32; i += 2) {
        ptrs[i] = malloc(8);
        if (!ptrs[i]) success = 0;
    }
    assert_test(success, "Re-allocation of holes");
    
    for (int i = 0; i < 32; i++) {
        free(ptrs[i]);
    }
}

int main() {
    print_block_divider();
    printf("USERSpace Malloc Test Suite Starting...\n");

    test_alignment();
    test_splitting_and_reuse();
    test_coalescing();
    test_edge_cases();
    test_stress();

    printf("\nSummary: %d/%d tests passed\n", tests_run - tests_failed, tests_run);
    
    if (tests_failed == 0) {
        printf("RESULT: malloc_test PASS\n");
        print_block_divider();
        return 0;
    } else {
        printf("RESULT: malloc_test FAIL\n");
        print_block_divider();
        return 1;
    }
}

#ifndef KERNEL_TEST_TEST_RUNNER_H
#define KERNEL_TEST_TEST_RUNNER_H

#include <stdio.h>
#include "devices.h"
#include "fat_test.h"

static inline bool run_all_tests(device_t* primary_storage) {
    test_stats_t stats = {0, 0};

    printf("[TEST] Running kernel test suite...\n");
    run_fat_tests(primary_storage, &stats);
    printf("[TEST] Summary: %u passed, %u failed\n", stats.passed, stats.failed);
    printf("[TEST] Overall: %s\n", (stats.failed == 0) ? "PASS" : "FAIL");

    return stats.failed == 0;
}

#endif

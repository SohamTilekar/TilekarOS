#ifndef KERNEL_TEST_TEST_RUNNER_H
#define KERNEL_TEST_TEST_RUNNER_H

#include <stdio.h>
#include "devices.h"
#include "fat_test.h"
#include "fd_table_test.h"
#include "kmalloc_test.h"
#include "process_test.h"

static inline bool run_all_tests(Device_t* primary_storage) {
    test_stats_t stats = {0, 0, NULL};

    test_print_header("TILEKAR OS KERNEL TEST SUITE");
    
    run_kmalloc_tests(&stats);
    run_fat_tests(primary_storage, &stats);
    run_fd_table_tests(primary_storage, &stats);
    run_process_tests(&stats);

    printf("\n/============================================================\\\n");
    printf("| FINAL TEST SUMMARY                                         |\n");
    printf("|------------------------------------------------------------|\n");
    printf("|  PASSED: %-49u |\n", stats.passed);
    printf("|  FAILED: %-49u |\n", stats.failed);
    printf("|  STATUS: %-49s |\n", (stats.failed == 0) ? "PASS" : "FAIL");
    printf("\\============================================================/\n\n");

    return stats.failed == 0;
}

#endif

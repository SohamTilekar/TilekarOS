#ifndef KERNEL_TEST_TEST_RUNNER_H
#define KERNEL_TEST_TEST_RUNNER_H

#include <stdio.h>
#include "devices.h"
#include "fat_test.h"
#include "fd_table_test.h"
#include "kmalloc_test.h"
#include "malloc_test.h"
#include "process_test.h"
#include "libc_test.h"
#include "signal_test.h"

#include "drivers/timer.h"

static inline bool run_all_tests(Device_t* primary_storage) {
    test_stats_t stats = {0, 0, NULL};
    uint32_t total_start = get_ticks();

    // Ensure /tmp exists for tests that use it
    vfs_mkdir("/tmp");

    test_print_header("TILEKAR OS KERNEL TEST SUITE");
    
    run_kmalloc_tests(&stats);
    run_malloc_tests(&stats);
    run_fat_tests(primary_storage, &stats);
    run_fd_table_tests(primary_storage, &stats);
    run_process_tests(&stats);
    run_signal_tests(&stats);
    run_libc_userspace_tests(&stats);

    uint32_t total_duration = get_ticks() - total_start;

    printf("\n/============================================================\\\n");
    printf("| FINAL TEST SUMMARY                                         |\n");
    printf("|------------------------------------------------------------|\n");
    printf("|  PASSED:   %-47u |\n", stats.passed);
    printf("|  FAILED:   %-47u |\n", stats.failed);
    printf("|  DURATION: %-42u ticks |\n", total_duration);
    printf("|  STATUS:   %-47s |\n", (stats.failed == 0) ? "PASS" : "FAIL");
    printf("\\============================================================/\n\n");

    return stats.failed == 0;
}

#endif

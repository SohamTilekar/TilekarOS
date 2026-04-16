#ifndef KERNEL_TEST_MALLOC_TEST_H
#define KERNEL_TEST_MALLOC_TEST_H

#include "test_utils.h"
#include "../arch/i386/task/task.h"
#include "../arch/i386/fs/vfs.h"
#include "../arch/i386/drivers/timer.h"
#include "../arch/i386/drivers/keyboard.h"
#include <stdio.h>

/**
 * run_malloc_tests - Launches the usermode malloc test binary (/BIN/MALLOC_TEST)
 */
static inline void run_malloc_tests(test_stats_t* stats) {
    test_print_category(stats, "USERSPACE MALLOC TEST (sbrk/malloc/free)");

    int fd = vfs_open("/BIN/MALLOC_TEST", 0);
    bool exists = (fd >= 0);
    if (fd >= 0) vfs_close(fd);

    test_record(stats, exists, "Userspace binary /BIN/MALLOC_TEST found");

    if (exists) {
        const char* path = "/BIN/MALLOC_TEST";
        printf("| [>] Stopping scheduler for userspace malloc test...       |\n");
        task_stop_scheduler();

        printf("| [>] Launching %s...                         |\n", path);
        task_t* test_task = task_create_elf_from_file(path, 3);

        if (test_task) {
            test_record(stats, true, "Userspace malloc test task created");

            printf("| [>] Starting scheduler and switching to userspace test... |\n");
            test_print_line('-', '-', '-', 62);
            task_start_scheduler();
            task_switch_to(test_task);

            uint32_t start_ticks = get_ticks();
            while (get_ticks() < start_ticks + 4000) { // wait ~4s for output
                task_yield(NULL);
            }
            test_print_line('-', '-', '-', 62);

            printf("| [?] Does the userspace malloc output look correct? (y/n): ");
            char choice = 0;
            // Clear keyboard buffer and wait for input up to 10s
            keyboard_clear_buffer();
            uint32_t verify_start = get_ticks();
            while (choice == 0 && get_ticks() < verify_start + 10000) {
                choice = keyboard_getchar();
                if (choice == 0) task_yield(NULL);
            }
            printf("\n");

            bool matched = (choice == 'y' || choice == 'Y');
            if (choice == 0) {
                printf("| [!] Verification input timed out. Recording as FAIL.     |\n");
            }
            test_record(stats, matched, "Userspace malloc test manual verification");

            printf("| [>] Resume tests after verification.                      |\n");
        }
    } else {
        printf("| [!] INSTRUCTION: To fully verify userspace malloc:\n");
        printf("| [!] 1. Compile TestVM/exported_drives/boot/malloc_test.c as /BIN/MALLOC_TEST\n");
        printf("| [!] 2. Place it in /BIN/MALLOC_TEST on the boot disk.\n");
    }
}

#endif

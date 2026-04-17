#ifndef KERNEL_TEST_UTILS_H
#define KERNEL_TEST_UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint32_t passed;
    uint32_t failed;
    const char* current_category;
} test_stats_t;

static inline void test_print_chars(char c, int count) {
    for (int i = 0; i < count; i++) {
        putchar(c);
    }
}

static inline void test_print_line(char start, char mid, char end, int width) {
    putchar(start);
    test_print_chars(mid, width);
    putchar(end);
    putchar('\n');
}

static inline void test_print_header(const char* title) {
    printf("\n");
    test_print_line('/', '=', '\\', 60);
    printf("| %-58s |\n", title);
    test_print_line('\\', '=', '/', 60);
}

static inline void test_print_category(test_stats_t* stats, const char* category) {
    stats->current_category = category;
    printf("\n[ CATEGORY: %-46s ]\n", category);
    test_print_line('-', '-', '-', 62);
}

static inline void test_record(test_stats_t* stats, bool ok, const char* name) {
    if (ok) {
        stats->passed++;
        printf("| [+] PASS : %-48s |\n", name);
    } else {
        stats->failed++;
        printf("| [!] FAIL : %-48s |\n", name);
    }
}

static inline void test_print_divider(int width) {
    test_print_line('-', '-', '-', width);
}

/**
 * test_verify_res_file - Helper to check if a result file contains "PASS" or "FAIL"
 * Returns: 1 if PASS, 0 if FAIL, -1 if file missing/error
 */
static inline int test_verify_res_file(const char* path) {
    int res_fd = vfs_open(path, 0);
    if (res_fd < 0) return -1;

    char res_buf[8];
    memset(res_buf, 0, 8);
    int bytes = vfs_read(res_fd, res_buf, 4);
    vfs_close(res_fd);

    if (bytes == 4) {
        if (memcmp(res_buf, "PASS", 4) == 0) return 1;
        if (memcmp(res_buf, "FAIL", 4) == 0) return 0;
    }
    return -1;
}

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
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

void test_ctype() {
    print_category("Ctype Tests");
    assert_test(isdigit('0') && isdigit('9') && !isdigit('a'), "isdigit");
    assert_test(isalpha('a') && isalpha('Z') && !isalpha('1'), "isalpha");
    assert_test(isblank(' ') && isblank('\t') && !isblank('\n'), "isblank");
    assert_test(iscntrl('\n') && iscntrl('\0') && !iscntrl('A'), "iscntrl");
    assert_test(isgraph('!') && isgraph('z') && !isgraph(' '), "isgraph");
    assert_test(islower('a') && !islower('A'), "islower");
    assert_test(isupper('Z') && !isupper('z'), "isupper");
    assert_test(isprint(' ') && isprint('A') && !isprint('\t'), "isprint");
    assert_test(ispunct('.') && ispunct(',') && !ispunct('A'), "ispunct");
    assert_test(isspace(' ') && isspace('\n') && !isspace('x'), "isspace");
    assert_test(toupper('a') == 'A' && toupper('Z') == 'Z', "toupper");
    assert_test(tolower('A') == 'a' && tolower('z') == 'z', "tolower");
    assert_test(isxdigit('a') && isxdigit('F') && isxdigit('0') && !isxdigit('g'), "isxdigit");
}

void test_string() {
    print_category("String Tests");

    char buf[128];
    strcpy(buf, "hello");
    assert_test(strcmp(buf, "hello") == 0, "strcpy/strcmp");

    assert_test(strcmp(strerror(ENOENT), "No such file or directory") == 0, "strerror(ENOENT)");
    assert_test(strcmp(strerror(0), "Success") == 0, "strerror(0)");
    assert_test(strcmp(strerror(999), "Unknown error") == 0, "strerror(unknown)");

    strcat(buf, " world");
    assert_test(strcmp(buf, "hello world") == 0, "strcat");

    assert_test(strlen(buf) == 11, "strlen");
    assert_test(strnlen(buf, 5) == 5, "strnlen limit");
    assert_test(strnlen(buf, 20) == 11, "strnlen no limit");

    // Safety test: strncpy
    char small_buf[5];
    strncpy(small_buf, "toolong", 5);
    assert_test(memcmp(small_buf, "toolo", 5) == 0, "strncpy truncation");

    assert_test(strncmp(buf, "hello", 5) == 0, "strncmp match");
    assert_test(strncmp(buf, "hellx", 5) != 0, "strncmp mismatch");

    assert_test(strchr(buf, 'w') == buf + 6, "strchr");
    assert_test(strrchr(buf, 'l') == buf + 9, "strrchr");
    assert_test(strstr(buf, "world") == buf + 6, "strstr");

    char tok_buf[] = "a,b,c;d";
    char* t = strtok(tok_buf, ",;");
    assert_test(strcmp(t, "a") == 0, "strtok 1");
    t = strtok(NULL, ",;");
    assert_test(strcmp(t, "b") == 0, "strtok 2");
    t = strtok(NULL, ",;");
    assert_test(strcmp(t, "c") == 0, "strtok 3");
    t = strtok(NULL, ",;");
    assert_test(strcmp(t, "d") == 0, "strtok 4");
    t = strtok(NULL, ",;");
    assert_test(t == NULL, "strtok end");

    memset(buf, 'A', 5);
    buf[5] = '\0';
    assert_test(strcmp(buf, "AAAAA") == 0, "memset");

    char src[] = "copy me";
    memcpy(buf, src, 8);
    assert_test(memcmp(buf, src, 8) == 0, "memcpy/memcmp");

    // memmove overlapping
    char overlap[] = "1234567890";
    memmove(overlap + 2, overlap, 5); // "1212345890" ? No, "12345" copied to +2
    // 0123456789
    // 1234567890
    //   12345
    // 1212345890
    assert_test(memcmp(overlap, "1212345890", 10) == 0, "memmove overlapping (forward)");

    strcpy(buf, "hello world");
    assert_test(strspn("123abc456", "1234567890") == 3, "strspn");
    assert_test(strcspn("abc123def", "1234567890") == 3, "strcspn");
    assert_test(strpbrk(buf, "ow") == buf + 4, "strpbrk 'hello world'");

    char stp_buf[32];
    char* stp_ret = stpcpy(stp_buf, "test");
    assert_test(strcmp(stp_buf, "test") == 0 && *stp_ret == '\0' && stp_ret == stp_buf + 4, "stpcpy");
}

void test_stdlib() {
    print_category("Stdlib Tests");

    assert_test(atoi("123") == 123, "atoi positive");
    assert_test(atoi("-456") == -456, "atoi negative");
    assert_test(atol("2147483647") == 2147483647L, "atol");

    char buf[32];
    itoa(1234, buf, 10);
    assert_test(strcmp(buf, "1234") == 0, "itoa base 10");
    itoa(0xABC, buf, 16);
    assert_test(strcmp(buf, "abc") == 0, "itoa base 16");

    assert_test(abs(-5) == 5 && abs(5) == 5, "abs");
    assert_test(labs(-123456L) == 123456L, "labs");

    // Calloc
    int* arr = (int*)calloc(10, sizeof(int));
    int all_zero = 1;
    if (arr) {
        for(int i=0; i<10; i++) if(arr[i] != 0) all_zero = 0;
    } else {
        all_zero = 0;
    }
    assert_test(arr != NULL && all_zero, "calloc zeros memory");

    // Realloc
    if (arr) {
        for(int i=0; i<10; i++) arr[i] = i;
        arr = (int*)realloc(arr, 20 * sizeof(int));
        assert_test(arr != NULL, "realloc expansion");
        int data_ok = 1;
        if (arr) {
            for(int i=0; i<10; i++) if(arr[i] != i) data_ok = 0;
        } else {
            data_ok = 0;
        }
        assert_test(data_ok, "realloc preserved data");
    }

    // realloc(NULL, size) is malloc
    void* r_malloc = realloc(NULL, 100);
    assert_test(r_malloc != NULL, "realloc(NULL, size) acts as malloc");
    free(r_malloc);

    free(arr);
}

int cmp_int(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

void test_qsort_bsearch() {
    print_category("Sort & Search Tests");

    int data[100];
    for(int i=0; i<100; i++) data[i] = 100 - i;

    qsort(data, 100, sizeof(int), cmp_int);

    int sorted = 1;
    for(int i=0; i<99; i++) if(data[i] > data[i+1]) sorted = 0;
    assert_test(sorted, "qsort 100 elements");

    int key = 42;
    void* res = bsearch(&key, data, 100, sizeof(int), cmp_int);
    assert_test(res != NULL && *(int*)res == 42, "bsearch found 42");

    key = 101;
    res = bsearch(&key, data, 100, sizeof(int), cmp_int);
    assert_test(res == NULL, "bsearch not found 101");
}

void test_stdio() {
    print_category("Stdio Tests");

    char buf[128];
    int n = sprintf(buf, "score: %d, name: %s, hex: %x, uint: %u", 100, "player", 0xABC, 4000000000U);
    assert_test(strcmp(buf, "score: 100, name: player, hex: abc, uint: 4000000000") == 0, "sprintf complex (inc uint)");
    assert_test(n == (int)strlen(buf), "sprintf return value");

    n = snprintf(buf, 10, "123456789012345");
    assert_test(strlen(buf) == 9, "snprintf truncation length");
    assert_test(strcmp(buf, "123456789") == 0, "snprintf truncation content");
    assert_test(buf[9] == '\0', "snprintf null-termination");

    mkdir("/tmp");
    FILE* f = fopen("/tmp/LIBC_IO.TXT", "w");
    if (f) {
        fprintf(f, "Testing fprintf: %d %s\n", 123, "success");
        fclose(f);
        assert_test(1, "fprintf to file");
    } else {
        assert_test(0, "fopen for write failed");
    }
}

void test_errno_assert() {
    print_category("Errno & Assert Tests");

    errno = 0;
    assert_test(errno == 0, "errno initial");
    errno = ENOENT;
    assert_test(errno == ENOENT, "errno set/get");

    assert(1 == 1);
    assert_test(1, "assert(true) passed");
}

void test_stack_smash() {
#ifdef TEST_STACK_SMASH
    print_category("Stack Smash Test (Expect Crash)");
    char small[4];
    // Overwrite return address
    memset(small, 'A', 64);
#endif
}

int main() {
    print_block_divider();
    printf("LibC Comprehensive Test Suite Starting...\n");

    test_ctype();
    test_string();
    test_stdlib();
    test_qsort_bsearch();
    test_stdio();
    test_errno_assert();
    test_stack_smash();

    printf("\nSummary: %d/%d tests passed\n", tests_run - tests_failed, tests_run);

    mkdir("/tmp");
    int fd = open("/tmp/LIBC.RES", O_WRONLY | O_CREAT); // O_WRONLY | O_CREAT

    if (tests_failed == 0) {
        printf("RESULT: libc_test PASS\n");
        if (fd >= 0) {
            write(fd, "PASS", 4);
            close(fd);
        }
        print_block_divider();
        return 0;
    } else {
        printf("RESULT: libc_test FAIL (%d failures)\n", tests_failed);
        if (fd >= 0) {
            write(fd, "FAIL", 4);
            close(fd);
        }
        print_block_divider();
        return 1;
    }
}

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int global_var = 100;

void log_checkpoint(const char* id) {
    printf("[CHECKPOINT: %s]\n", id);
    int fd = open("/tmp/PROCESS.LOG", O_WRONLY | O_CREAT); // O_WRONLY | O_CREAT
    if (fd >= 0) {
        // Advance to end of file
        char junk[256];
        int bytes;
        while ((bytes = read(fd, junk, sizeof(junk))) > 0);

        write(fd, id, strlen(id));
        write(fd, "\n", 1);
        close(fd);
    }
}

void test_memory_isolation() {
    printf("\n--- Memory Isolation Test ---\n");
    log_checkpoint("MEM_ISO_START");
    int local_var = 50;

    uint32_t parent_pid = getpid();
    int fork_ret = fork();
    uint32_t current_pid = getpid();

    if (current_pid != parent_pid) {
        // Child process
        global_var = 200;
        local_var = 150;
        printf("  CHILD: PID %d (Parent %d). Modified variables.\n", current_pid, parent_pid);
        log_checkpoint("CHILD_MOD_VARS");
        exit(0);
    } else {
        // Parent process
        yield(); // Child Process should be run first
        printf("  PARENT: My PID %d. Child PID was %d.\n", current_pid, fork_ret);
        printf("  PARENT: global_var is %d (expected 100), local_var is %d(expected 50)\n", global_var, local_var);
        log_checkpoint("PARENT_CHECK_VARS");

        if (global_var == 100 && local_var == 50) {
            printf("  [PASS] Memory Isolation\n");
            log_checkpoint("MEM_ISO_PASS");
        } else {
            printf("  [FAIL] Memory Isolation\n");
        }
    }
}

void test_nested_fork() {
    printf("\n--- Nested Fork (Grandchild) Test ---\n");
    log_checkpoint("NESTED_FORK_START");

    uint32_t p_pid = getpid();
    int __attribute__((unused)) c_ret = fork();

    if (getpid() != p_pid) {
        // Child
        uint32_t c_pid = getpid();
        int g_ret = fork();

        if (getpid() != c_pid) {
            // Grandchild
            printf("  GRANDCHILD: PID %d, child of %d\n", getpid(), c_pid);
            log_checkpoint("GRANDCHILD_ALIVE");
            exit(0);
        } else {
            // Child
            printf("  CHILD: PID %d, created grandchild %d\n", c_pid, g_ret);
            log_checkpoint("CHILD_CREATED_GC");
            exit(0);
        }
    }
    // Parent
}

void test_execve_integration() {
    printf("\n--- Execve Integration Test ---\n");
    printf("  PROCESS: PID %d, attempting execve /BIN/HELLO...\n", getpid());
    log_checkpoint("EXEC_START");

    int rc = execve("/BIN/HELLO", NULL, NULL);

    // If we reach here, execve failed
    printf("  [FAIL] Execve failed (rc=%d)\n", rc);
    exit(1);
}

int main() {
    printf("ADVANCED PROCESS LIFECYCLE TESTS (USERSPACE)\n");
    printf("Base PID: %d\n", getpid());

    test_memory_isolation();
    test_nested_fork();

    printf("\nDispatching final tests...\n");
    test_execve_integration();
    return 0;
}

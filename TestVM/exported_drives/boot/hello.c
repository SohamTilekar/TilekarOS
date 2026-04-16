#include <stdio.h>
#include <unistd.h>

int main() {
    printf("  [PASS] Execve: Replaced image with /BIN/HELLO\n");
    printf("  HELLO: My PID is %d\n", getpid());
    return 0;
}

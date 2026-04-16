#include <stdio.h>
#include <unistd.h>

int main() {
    printf("HELLO: Successfully replaced process image via execve()!\n");
    printf("HELLO: My PID is %d. I am now running in place of the old task.\n", getpid());
    return 0;
}

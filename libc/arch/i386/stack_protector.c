#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

uintptr_t __stack_chk_guard = 0xDEADC0DE;

__attribute__((noreturn))
void __stack_chk_fail(void) {
    printf("\n*** STACK SMASHING DETECTED ***\n");
    _exit(1);
}

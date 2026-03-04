#include <stdint.h>

void init_gdt();
void tss_set_kernel_stack(uint32_t stack_base);

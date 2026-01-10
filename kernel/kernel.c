#include "include/kernel/tty.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel/tty.h>

void kernel_main(void)
{
    // gdt & idt have alrady been setuped in the boot asm
	terminal_writestring("Hello World!\nPrint On TilekarOS by Soham Tilekar\n");
	volatile int i = 0;
	volatile int x = 1/i;
}

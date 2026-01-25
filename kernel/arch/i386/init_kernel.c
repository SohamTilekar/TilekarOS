#include "kernel/tty.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "keyboard.h"
#include <stdio.h>
#include "memory.h"

void init_kernel(uint32_t magic, MultiBootInfo* boot_info) {
    init_terminal();
	init_memory(boot_info);
	init_gdt();
	init_idt();
	init_timer();
	init_keyboard();
}

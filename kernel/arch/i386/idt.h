#ifndef IDT_H
#define IDT_H

#include <stdint.h>
#include "utils.h"

// IDT Initialization
void init_idt();

// IRQ Handling
void irq_install_handler(int irq, void (*handler)(InteruptReg *r));
void irq_uninstall_handler(int irq);

#endif

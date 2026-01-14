#include <stdint.h>
#include "idt.h"

void init_timer();
void onIrq0(InteruptReg *regs);

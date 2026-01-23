#include "timer.h"
#include "utils.h"
#include "idt.h"

uint64_t ticks;
const uint32_t freq = 100;

void onIrq0(InteruptReg *regs){
    (void)regs;
    ticks += 1;
    // put code to run when timer ticks
}

void init_timer(){
    ticks = 0;
    irq_install_handler(0,&onIrq0);

    //119318.16666 Mhz
    uint32_t divisor = 1193180/freq;

    //0011 0110
    out_port_b(0x43,0x36);
    out_port_b(0x40,(uint8_t)(divisor & 0xFF));
    out_port_b(0x40,(uint8_t)((divisor >> 8) & 0xFF));
}

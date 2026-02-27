#include "timer.h"
#include "kernel/tty.h"
#include "kmalloc.h"
#include "utils.h"
#include "idt.h"
#include <stdint.h>
#include <stdio.h>

uint32_t ticks;
const uint32_t freq = 1000;


typedef struct {
    uint32_t tick_mod;
    void (*func)();
} Triger;

typedef struct {
    uint32_t len;
    Triger trigers[];
} Trigers;
Trigers* trigers = NULL;

void insert_triger(uint32_t tick_mod, void (*func)()) {
    if (tick_mod == 0) return;

    uint32_t flags = interrupt_save();

    if (trigers == NULL) {
        init_timer();
    }

    uint32_t new_len = trigers->len + 1;
    size_t new_size = sizeof(Trigers) + new_len * sizeof(Triger);

    Trigers* new_trigers = krealloc(trigers, new_size);
    if (new_trigers) {
        trigers = new_trigers;
        trigers->trigers[trigers->len].tick_mod = tick_mod;
        trigers->trigers[trigers->len].func = func;
        trigers->len = new_len;
    }

    interrupt_restore(flags);
};

void onIrq0(InteruptReg *regs){
    (void)regs;
    ticks += 1;

    if (trigers == NULL) return;

    for (uint32_t i = 0; i < trigers->len; i++) {
        if ((ticks % trigers->trigers[i].tick_mod) == 0) {
            trigers->trigers[i].func();
        }
    }
}


void init_timer(){
    trigers = kmalloc(sizeof(Trigers));
    trigers->len = 0;
    ticks = 0;

    irq_install_handler(0,&onIrq0);

    //119318.16666 Mhz
    uint32_t divisor = 1193180/freq;

    //0011 0110
    out_port_b(0x43,0x36);
    out_port_b(0x40,(uint8_t)(divisor & 0xFF));
    out_port_b(0x40,(uint8_t)((divisor >> 8) & 0xFF));
}

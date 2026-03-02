#include "timer.h"
#include "kernel/tty.h"
#include "kmalloc.h"
#include "utils.h"
#include "idt.h"
#include <stdint.h>
#include <stdio.h>

uint32_t ticks;
const uint32_t freq = 1000;

uint32_t get_ticks() {
    uint32_t flags = interrupt_save();
    uint32_t current_ticks = ticks;
    interrupt_restore(flags);
    return current_ticks;
}

void set_ticks(uint32_t new_ticks) {
    uint32_t flags = interrupt_save();
    ticks = new_ticks;
    interrupt_restore(flags);
}


typedef struct {
    uint32_t tick_mod;
    uint32_t current_ticks;
    void (*func)(InteruptReg *regs); // send EOI if going to change context
    uint8_t flags;
} Triger;

typedef struct {
    uint32_t len;
    Triger trigers[];
} Trigers;
Trigers* trigers = NULL;

int32_t insert_triger(uint32_t tick_mod, void (*func)(InteruptReg *regs), uint8_t flags) {
    if (tick_mod == 0) return -1;

    uint32_t int_flags = interrupt_save();

    if (trigers == NULL) {
        init_timer();
    }

    int32_t index = -1;
    // Search for available slot
    for (uint32_t i = 0; i < trigers->len; i++) {
        if (!(trigers->trigers[i].flags & TIMER_TRIGGER_ACTIVE)) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        uint32_t new_len = trigers->len + 1;
        size_t new_size = sizeof(Trigers) + new_len * sizeof(Triger);

        Trigers* new_trigers = krealloc(trigers, new_size);
        if (!new_trigers) {
            interrupt_restore(int_flags);
            return -1;
        }
        trigers = new_trigers;
        index = trigers->len;
        trigers->len = new_len;
    }

    trigers->trigers[index].tick_mod = tick_mod;
    trigers->trigers[index].current_ticks = 0;
    trigers->trigers[index].func = func;
    trigers->trigers[index].flags = flags | TIMER_TRIGGER_ACTIVE;

    interrupt_restore(int_flags);
    return index;
};

void remove_triger(int32_t index) {
    if (trigers == NULL || index < 0 || (uint32_t)index >= trigers->len) return;

    uint32_t int_flags = interrupt_save();
    trigers->trigers[index].tick_mod = 0;
    trigers->trigers[index].current_ticks = 0;
    trigers->trigers[index].func = NULL;
    trigers->trigers[index].flags = 0;
    interrupt_restore(int_flags);
}

void set_triger_mod(int32_t index, uint32_t tick_mod) {
    if (trigers == NULL || index < 0 || (uint32_t)index >= trigers->len) return;
    uint32_t int_flags = interrupt_save();
    trigers->trigers[index].tick_mod = tick_mod;
    interrupt_restore(int_flags);
}

void set_triger_ticks(int32_t index, uint32_t current_ticks) {
    if (trigers == NULL || index < 0 || (uint32_t)index >= trigers->len) return;
    uint32_t int_flags = interrupt_save();
    trigers->trigers[index].current_ticks = current_ticks;
    interrupt_restore(int_flags);
}

void onIrq0(InteruptReg *regs){
    ticks += 1;

    if (trigers == NULL) return;

    for (uint32_t i = 0; i < trigers->len; i++) {
        Triger* t = &trigers->trigers[i];
        if (!(t->flags & TIMER_TRIGGER_ACTIVE)) continue;

        if (t->flags & TIMER_TRIGGER_USE_GLOBAL) {
            if (ticks >= t->tick_mod) {
                t->func(regs);
            }
        } else {
            t->current_ticks++;
            if (t->current_ticks >= t->tick_mod) {
                t->current_ticks = 0;
                t->func(regs);
            }
        }
    }
}


void init_timer(){
    uint32_t flags = interrupt_save();
    if (trigers != NULL) {
        interrupt_restore(flags);
        return;
    }
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
    interrupt_restore(flags);
}

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
    void (*func)(InterruptReg_t *regs); // send EOI if going to change context
    uint8_t flags;
} Trigger;

typedef struct {
    uint32_t len;
    Trigger triggers[];
} Triggers;
Triggers* triggers = NULL;

int32_t insert_trigger(uint32_t tick_mod, void (*func)(InterruptReg_t *regs), uint8_t flags) {
    if (tick_mod == 0) return -1;

    uint32_t int_flags = interrupt_save();

    if (triggers == NULL) {
        init_timer();
    }

    int32_t index = -1;
    // Search for available slot
    for (uint32_t i = 0; i < triggers->len; i++) {
        if (!(triggers->triggers[i].flags & TIMER_TRIGGER_ACTIVE)) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        uint32_t new_len = triggers->len + 1;
        size_t new_size = sizeof(Triggers) + new_len * sizeof(Trigger);

        Triggers* new_triggers = krealloc(triggers, new_size);
        if (!new_triggers) {
            interrupt_restore(int_flags);
            return -1;
        }
        triggers = new_triggers;
        index = triggers->len;
        triggers->len = new_len;
    }

    triggers->triggers[index].tick_mod = tick_mod;
    triggers->triggers[index].current_ticks = 0;
    triggers->triggers[index].func = func;
    triggers->triggers[index].flags = flags | TIMER_TRIGGER_ACTIVE;

    interrupt_restore(int_flags);
    return index;
};

void remove_trigger(int32_t index) {
    if (triggers == NULL || index < 0 || (uint32_t)index >= triggers->len) return;

    uint32_t int_flags = interrupt_save();
    triggers->triggers[index].tick_mod = 0;
    triggers->triggers[index].current_ticks = 0;
    triggers->triggers[index].func = NULL;
    triggers->triggers[index].flags = 0;
    interrupt_restore(int_flags);
}

void set_trigger_mod(int32_t index, uint32_t tick_mod) {
    if (triggers == NULL || index < 0 || (uint32_t)index >= triggers->len) return;
    uint32_t int_flags = interrupt_save();
    triggers->triggers[index].tick_mod = tick_mod;
    interrupt_restore(int_flags);
}

void set_trigger_ticks(int32_t index, uint32_t current_ticks) {
    if (triggers == NULL || index < 0 || (uint32_t)index >= triggers->len) return;
    uint32_t int_flags = interrupt_save();
    triggers->triggers[index].current_ticks = current_ticks;
    interrupt_restore(int_flags);
}

void set_trigger_flags(int32_t index, uint8_t flags) {
    if (triggers == NULL || index < 0 || (uint32_t)index >= triggers->len) return;
    uint32_t int_flags = interrupt_save();
    triggers->triggers[index].flags = flags;
    interrupt_restore(int_flags);
}

uint8_t get_trigger_flags(int32_t index) {
    if (triggers == NULL || index < 0 || (uint32_t)index >= triggers->len) return 0;
    return triggers->triggers[index].flags;
}

void onIrq0(InterruptReg_t *regs){
    ticks += 1;

    if (triggers == NULL) return;

    for (uint32_t i = 0; i < triggers->len; i++) {
        Trigger* t = &triggers->triggers[i];
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
    if (triggers != NULL) {
        interrupt_restore(flags);
        return;
    }
    triggers = kmalloc(sizeof(Triggers));
    triggers->len = 0;
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

#include "utils.h"
#include <stdint.h>
#include <stdbool.h>

#define TIMER_TRIGGER_ACTIVE 1
#define TIMER_TRIGGER_USE_GLOBAL 2

void init_timer();
uint32_t get_ticks();
void set_ticks(uint32_t new_ticks);
int32_t insert_triger(uint32_t tick_mod, void (*func)(InteruptReg *regs), uint8_t flags);
void remove_triger(int32_t index);
void set_triger_mod(int32_t index, uint32_t tick_mod);
void set_triger_ticks(int32_t index, uint32_t ticks);

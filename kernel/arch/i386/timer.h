#include <stdint.h>

void init_timer();
void insert_triger(uint32_t tick_mod, void (*func)());

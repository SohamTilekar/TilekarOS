#include "stdio.h"
#include <kernel/tty.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void kernel_main(void) {
  printf("Hello World!\nPrint On TilekarOS by Soham Tilekar\n");
  for (;;);
}

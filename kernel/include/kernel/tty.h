#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <stddef.h>
#include <stdint.h>

void init_terminal(void);
void tty_register(void);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);

// Cursor Management
void terminal_enable_cursor(uint8_t cursor_start, uint8_t cursor_end);
void terminal_disable_cursor(void);
void terminal_update_cursor(void);
void terminal_cursor_up(void);
void terminal_cursor_down(void);
void terminal_cursor_left(void);
void terminal_cursor_right(void);
void terminal_delete_char(void);

#endif

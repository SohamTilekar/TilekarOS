#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <kernel/tty.h>

#include "vga.h"
#include "utils.h"

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer = (uint16_t*)VGA_MEMORY;

void terminal_enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
	out_port_b(0x3D4, 0x0A);
	out_port_b(0x3D5, (in_port_b(0x3D5) & 0xC0) | cursor_start);
 
	out_port_b(0x3D4, 0x0B);
	out_port_b(0x3D5, (in_port_b(0x3D5) & 0xE0) | cursor_end);
}

void terminal_disable_cursor(void)
{
	out_port_b(0x3D4, 0x0A);
	out_port_b(0x3D5, 0x20);
}

void terminal_update_cursor(void)
{
	uint16_t pos = terminal_row * VGA_WIDTH + terminal_column;
 
	out_port_b(0x3D4, 0x0F);
	out_port_b(0x3D5, (uint8_t) (pos & 0xFF));
	out_port_b(0x3D4, 0x0E);
	out_port_b(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

void init_terminal(void)
{
    // Initialize COM1 serial port for logging
    out_port_b(0x3F8 + 1, 0x00);    // Disable all interrupts
    out_port_b(0x3F8 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    out_port_b(0x3F8 + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    out_port_b(0x3F8 + 1, 0x00);    //                  (hi byte)
    out_port_b(0x3F8 + 3, 0x03);    // 8 bits, no parity, one stop bit
    out_port_b(0x3F8 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    out_port_b(0x3F8 + 4, 0x0B);    // IRQs enabled, RTS/DSR set

	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
    terminal_enable_cursor(0, 15);
    terminal_update_cursor();
}

void terminal_setcolor(uint8_t color)
{
	terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y)
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void terminal_scroll(void)
{
    memmove(terminal_buffer, terminal_buffer + VGA_WIDTH, (VGA_HEIGHT - 1) * VGA_WIDTH * sizeof(uint16_t));
    
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        const size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        terminal_buffer[index] = vga_entry(' ', terminal_color);
    }
}

void terminal_putchar(char c)
{
    // Write to serial port COM1 for headless logging
    out_port_b(0x3F8, c);

    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;
    } else if (c == '\b') {
        if (terminal_column > 0) {
            terminal_column--;
        } else if (terminal_row > 0) {
            terminal_row--;
            terminal_column = VGA_WIDTH - 1;
        }
        terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
    } else {
        terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
        if (++terminal_column == VGA_WIDTH) {
            terminal_column = 0;
            terminal_row++;
        }
    }

    if (terminal_row == VGA_HEIGHT) {
        terminal_scroll();
        terminal_row = VGA_HEIGHT - 1;
    }
    terminal_update_cursor();
}

void terminal_write(const char* data, size_t size)
{
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void terminal_writestring(const char* data)
{
	terminal_write(data, strlen(data));
}

void terminal_cursor_up(void) {
    if (terminal_row > 0) {
        terminal_row--;
        terminal_update_cursor();
    }
}

void terminal_cursor_down(void) {
    if (terminal_row < VGA_HEIGHT - 1) {
        terminal_row++;
        terminal_update_cursor();
    }
}

void terminal_cursor_left(void) {
    if (terminal_column > 0) {
        terminal_column--;
    } else if (terminal_row > 0) {
        terminal_row--;
        terminal_column = VGA_WIDTH - 1;
    }
    terminal_update_cursor();
}

void terminal_cursor_right(void) {
    if (terminal_column < VGA_WIDTH - 1) {
        terminal_column++;
    } else if (terminal_row < VGA_HEIGHT - 1) {
        terminal_row++;
        terminal_column = 0;
    }
    terminal_update_cursor();
}

void terminal_delete_char(void) {
    terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
    terminal_cursor_right();
}

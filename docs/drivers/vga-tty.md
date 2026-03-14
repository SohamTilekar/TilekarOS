# VGA & TTY Driver

TilekarOS uses the standard VGA Text Mode buffer (0xB8000) for output.

## 1. VGA Text Mode

- **Buffer Address**: 0xB8000
- **Dimensions**: 80 characters wide by 25 characters high.
- **Entry Format**: Each character is 2 bytes.
  - Byte 0: ASCII Character.
  - Byte 1: Color Attribute (Foreground/Background).

## 2. Terminal Logic (`tty.c`)

The terminal driver provides basic text output functions:

- `init_terminal()`: Clears the screen, sets the default color, and enables the VGA cursor.
- `terminal_putchar(char c)`: Prints a character, handling newlines (`\n`), backspace (`\b`), and scrolling.
- `terminal_writestring(const char* data)`: Prints a null-terminated string.
- `terminal_scroll()`: Moves all lines up by one when the bottom of the screen is reached.

---

## 3. Serial Port Logging (COM1)

In addition to the VGA buffer, `terminal_putchar` also writes every character to the **COM1 Serial Port** (`0x3F8`).

- **Baud Rate**: 38400.
- **Config**: 8 data bits, no parity, 1 stop bit.
- **Purpose**: Allows headless logging and debugging via an external serial monitor (e.g., QEMU `-serial stdio`).

---

## 4. Cursor Management

The hardware cursor is controlled using VGA I/O ports (`0x3D4`, `0x3D5`).

- `terminal_enable_cursor(start, end)`: Shows the cursor and sets its vertical shape.
- `terminal_disable_cursor()`: Hides the cursor.
- `terminal_update_cursor()`: Moves the cursor to the current `terminal_row` and `terminal_column`.

---

## 5. Movement and Editing

Extended functions are available for interactive text manipulation:

- `terminal_cursor_up/down/left/right()`: Move the cursor.
- `terminal_delete_char()`: Replaces the current character with a space and moves the cursor right.

---

## 6. Colors

Available colors are defined in `vga.h`:

- `VGA_COLOR_BLACK`, `VGA_COLOR_BLUE`, `VGA_COLOR_LIGHT_GREY`, etc.
- Attributes are combined using `(bg << 4) | fg`.

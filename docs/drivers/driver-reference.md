# Hardware Drivers Reference

This document provides a technical overview of the hardware drivers implemented in TilekarOS.

## 1. VGA & TTY Driver
**Source Files**: [tty.c](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/drivers/tty.c){: target="_blank" }, [vga.h](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/drivers/vga.h){: target="_blank" }

The VGA driver manages the text-mode buffer at `0xB8000`.

??? example "Code Preview: `tty.c`"
    ```c
    --8<-- "kernel/arch/i386/drivers/tty.c"
    ```

### Features:
- **Scrolling**: Moves the entire screen up when the bottom is reached.
- **Color Support**: Supports 16 foreground and 16 background colors.
- **Hardware Cursor**: Updates the blinking underscore using VGA I/O ports.
- **Serial Logging**: Mirrors all output to the **COM1 Serial Port** (`0x3F8`) for easy debugging in QEMU via `-serial stdio`.

Implementation details can be found in [tty.c](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/drivers/tty.c){: target="_blank" }.

!!! info "OSDev Reference"
    For VGA hardware details, see [OSDev: VGA Text Mode](https://wiki.osdev.org/VGA_Text_Mode).

---

## 2. PS/2 Keyboard Driver
**Source Files**: [keyboard.c](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/drivers/keyboard.c){: target="_blank" }, [keyboard.h](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/drivers/keyboard.h){: target="_blank" }

The keyboard driver handles input from a standard PS/2 keyboard using IRQ 1.

??? example "Code Preview: `keyboard.c`"
    ```c
    --8<-- "kernel/arch/i386/drivers/keyboard.c"
    ```

### Features:
- **Scancode Mapping**: Translates raw PS/2 Set 1 scancodes into ASCII characters.
- **Modifier Tracking**: Tracks the state of Shift, Ctrl, Alt, and Caps Lock.
- **Buffer API**: Provides `keyboard_getchar()` which yields or halts until a key is pressed.
- **Callback System**: Allows other kernel modules to register a function to be called on every key event.

Full key mapping is available in [keyboard.c](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/drivers/keyboard.c){: target="_blank" }.

!!! info "OSDev Reference"
    For keyboard scancode tables, see [OSDev: PS/2 Keyboard](https://wiki.osdev.org/PS/2_Keyboard).

---

## 3. Programmable Interval Timer (PIT)
**Source Files**: [timer.c](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/drivers/timer.c){: target="_blank" }, [timer.h](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/drivers/timer.h){: target="_blank" }

The PIT generates periodic interrupts at a fixed frequency (default: 1000Hz / 1ms).

??? example "Code Preview: `timer.c`"
    ```c
    --8<-- "kernel/arch/i386/drivers/timer.c"
    ```

### The Trigger System:
TilekarOS features a "Trigger" system (`insert_triger`) that allows functions to be scheduled to run every X ticks without needing a full task.
- **Preemption**: The scheduler uses a trigger to preempt tasks every 100ms.
- **Timekeeping**: The global `ticks` counter tracks time since boot.

Check the implementation in [timer.c](https://github.com/SohamTilekar/TilekarOS/blob/main/kernel/arch/i386/drivers/timer.c){: target="_blank" }.

!!! info "OSDev Reference"
    For PIT configuration details, see [OSDev: PIT](https://wiki.osdev.org/Programmable_Interval_Timer).

---

## 4. Test/Example: Custom Keyboard Handling

You can use the callback system to create interactive kernel features:

```c
void my_hotkey_handler(KeyEvent event) {
    if (event.pressed && event.key_code == KEY_F1) {
        printf("\n[DEBUG] Memory Usage: ...\n");
    }
}

void setup_hotkeys() {
    keyboard_set_callback(&my_hotkey_handler);
}
```

---

## References
- [OSDev: VGA Text Mode](https://wiki.osdev.org/VGA_Text_Mode)
- [OSDev: PS/2 Keyboard](https://wiki.osdev.org/PS/2_Keyboard)
- [OSDev: PIT](https://wiki.osdev.org/Programmable_Interval_Timer)
- [Wikipedia: Serial Port](https://en.wikipedia.org/wiki/Serial_port)
- [Wikipedia: PS/2 Port](https://en.wikipedia.org/wiki/PS/2_port)
- [Wikipedia: Programmable Interval Timer](https://en.wikipedia.org/wiki/Programmable_interval_timer)

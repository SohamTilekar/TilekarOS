# Keyboard Driver

The keyboard driver handles input from a standard PS/2 keyboard.

## 1. Hardware Interface

- **IRQ**: 1 (Interrupt Vector 33).
- **I/O Ports**:
  - `0x60`: Data Port (Read scancodes).
  - `0x64`: Command/Status Port.

## 2. Scancodes & Mapping

TilekarOS supports **PS/2 Scan Code Set 1**.

- **Make Code**: Sent when a key is pressed (e.g., 0x1E for 'A').
- **Break Code**: Sent when a key is released (Make Code OR 0x80).
- **Extended Scancodes**: Start with `0xE0` (e.g., arrows, `RALT`, `RCTRL`).

---

## 3. State Tracking

The driver tracks the state of modifier keys:

- **Shift** (Left/Right)
- **Ctrl** (Left/Right)
- **Alt** (Left/Right)
- **CapsLock** (Toggle)
- **NumLock** (Toggle)

---

## 4. Key Events

The driver generates a `KeyEvent` structure for every key press or release:
```c
typedef struct {
    enum KeyCode key_code;
    uint8_t scancode;
    bool pressed;
    bool shift_active;
    bool ctrl_active;
    bool alt_active;
    bool caps_lock;
    bool num_lock;
    char character;
} KeyEvent;
```

---

## 5. Usage

### Callback System
Register a custom callback function to handle keyboard events:
```c
void my_keyboard_handler(KeyEvent event) {
    if (event.pressed && event.key_code == KEY_ESC) {
        // Handle ESC...
    }
}

keyboard_set_callback(&my_keyboard_handler);
```

### Debug Mode (F1)
Pressing **F1** toggles Kernel Debug Mode.

- When enabled, the kernel logs unknown scancodes and special key presses.
- It also enables/disables the VGA text cursor.

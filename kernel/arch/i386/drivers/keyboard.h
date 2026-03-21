#ifndef ARCH_I386_KEYBOARD_H
#define ARCH_I386_KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>
#include "utils.h"

enum KeyCode {
    KEY_UNKNOWN      = 0x00,

    // Printable Characters
    KEY_A            = 0x01,
    KEY_B            = 0x02,
    KEY_C            = 0x03,
    KEY_D            = 0x04,
    KEY_E            = 0x05,
    KEY_F            = 0x06,
    KEY_G            = 0x07,
    KEY_H            = 0x08,
    KEY_I            = 0x09,
    KEY_J            = 0x0A,
    KEY_K            = 0x0B,
    KEY_L            = 0x0C,
    KEY_M            = 0x0D,
    KEY_N            = 0x0E,
    KEY_O            = 0x0F,
    KEY_P            = 0x10,
    KEY_Q            = 0x11,
    KEY_R            = 0x12,
    KEY_S            = 0x13,
    KEY_T            = 0x14,
    KEY_U            = 0x15,
    KEY_V            = 0x16,
    KEY_W            = 0x17,
    KEY_X            = 0x18,
    KEY_Y            = 0x19,
    KEY_Z            = 0x1A,

    KEY_0            = 0x1B,
    KEY_1            = 0x1C,
    KEY_2            = 0x1D,
    KEY_3            = 0x1E,
    KEY_4            = 0x1F,
    KEY_5            = 0x20,
    KEY_6            = 0x21,
    KEY_7            = 0x22,
    KEY_8            = 0x23,
    KEY_9            = 0x24,

    KEY_GRAVE        = 0x25,
    KEY_MINUS        = 0x26,
    KEY_EQUAL        = 0x27,
    KEY_LBRACKET     = 0x28,
    KEY_RBRACKET     = 0x29,
    KEY_BACKSLASH    = 0x2A,
    KEY_SEMICOLON    = 0x2B,
    KEY_APOSTROPHE   = 0x2C,
    KEY_COMMA        = 0x2D,
    KEY_DOT          = 0x2E,
    KEY_SLASH        = 0x2F,
    KEY_SPACE        = 0x30,

    // Function Keys
    KEY_F1           = 0x40,
    KEY_F2           = 0x41,
    KEY_F3           = 0x42,
    KEY_F4           = 0x43,
    KEY_F5           = 0x44,
    KEY_F6           = 0x45,
    KEY_F7           = 0x46,
    KEY_F8           = 0x47,
    KEY_F9           = 0x48,
    KEY_F10          = 0x49,
    KEY_F11          = 0x4A,
    KEY_F12          = 0x4B,

    // Control Keys
    KEY_ESC          = 0x50,
    KEY_ENTER        = 0x51,
    KEY_BACKSPACE    = 0x52,
    KEY_TAB          = 0x53,
    KEY_CAPSLOCK     = 0x54,
    KEY_LSHIFT       = 0x55,
    KEY_RSHIFT       = 0x56,
    KEY_LCTRL        = 0x57,
    KEY_RCTRL        = 0x58,
    KEY_LALT         = 0x59,
    KEY_RALT         = 0x5A,

    // Navigation / Editing
    KEY_INS          = 0x60,
    KEY_DEL          = 0x61,
    KEY_HOME         = 0x62,
    KEY_END          = 0x63,
    KEY_PGUP         = 0x64,
    KEY_PGDN         = 0x65,
    KEY_UP           = 0x66,
    KEY_DOWN         = 0x67,
    KEY_LEFT         = 0x68,
    KEY_RIGHT        = 0x69,

    // Lock Keys
    KEY_NUMLOCK      = 0x70,
    KEY_SCROLLLOCK   = 0x71,

    // Keypad
    KEY_KP_0         = 0x80,
    KEY_KP_1         = 0x81,
    KEY_KP_2         = 0x82,
    KEY_KP_3         = 0x83,
    KEY_KP_4         = 0x84,
    KEY_KP_5         = 0x85,
    KEY_KP_6         = 0x86,
    KEY_KP_7         = 0x87,
    KEY_KP_8         = 0x88,
    KEY_KP_9         = 0x89,
    KEY_KP_DECIMAL   = 0x8A,
    KEY_KP_DIV       = 0x8B,
    KEY_KP_MUL       = 0x8C,
    KEY_KP_SUB       = 0x8D,
    KEY_KP_ADD       = 0x8E,
    KEY_KP_ENTER     = 0x8F,

    // Additional keys
    KEY_PRINTSCREEN  = 0x90,
    KEY_PAUSE        = 0x91,
    KEY_LMETA        = 0x92, // Windows key
    KEY_RMETA        = 0x93,
    KEY_MENU         = 0x94
};

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

typedef void (*keyboard_callback_t)(KeyEvent event);

void init_keyboard();
void keyboard_register(void);
void keyboard_set_callback(keyboard_callback_t callback);
const char* keycode_to_string(enum KeyCode code);
char keycode_to_char(enum KeyCode code, bool shift, bool caps, bool numlock);

// New Buffer API
char keyboard_getchar();
int keyboard_read(void* buffer, uint32_t size);

#endif

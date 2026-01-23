#include "keyboard.h"
#include "idt.h"
#include "stdio.h"
#include <kernel/tty.h>

// State Variables
static bool is_extended = false;
static bool shift_l = false;
static bool shift_r = false;
static bool ctrl_l = false;
static bool ctrl_r = false;
static bool alt_l = false;
static bool alt_r = false;
static bool caps_lock = false;
static bool num_lock = true; // Enabled by default on most systems

// Debug Mode: Set to true to enable logging, false to disable.
static bool debug_mode = true;

static keyboard_callback_t active_callback = NULL;

const char* keycode_to_string(enum KeyCode code) {
    switch (code) {
        case KEY_UNKNOWN: return "UNKNOWN";
        case KEY_A: return "A"; case KEY_B: return "B"; case KEY_C: return "C";
        case KEY_D: return "D"; case KEY_E: return "E"; case KEY_F: return "F";
        case KEY_G: return "G"; case KEY_H: return "H"; case KEY_I: return "I";
        case KEY_J: return "J"; case KEY_K: return "K"; case KEY_L: return "L";
        case KEY_M: return "M"; case KEY_N: return "N"; case KEY_O: return "O";
        case KEY_P: return "P"; case KEY_Q: return "Q"; case KEY_R: return "R";
        case KEY_S: return "S"; case KEY_T: return "T"; case KEY_U: return "U";
        case KEY_V: return "V"; case KEY_W: return "W"; case KEY_X: return "X";
        case KEY_Y: return "Y"; case KEY_Z: return "Z";
        case KEY_0: return "0"; case KEY_1: return "1"; case KEY_2: return "2";
        case KEY_3: return "3"; case KEY_4: return "4"; case KEY_5: return "5";
        case KEY_6: return "6"; case KEY_7: return "7"; case KEY_8: return "8";
        case KEY_9: return "9";
        case KEY_GRAVE: return "`"; case KEY_MINUS: return "-"; case KEY_EQUAL: return "=";
        case KEY_LBRACKET: return "["; case KEY_RBRACKET: return "]"; case KEY_BACKSLASH: return "\\";
        case KEY_SEMICOLON: return ";"; case KEY_APOSTROPHE: return "'"; case KEY_COMMA: return ",";
        case KEY_DOT: return "."; case KEY_SLASH: return "/"; case KEY_SPACE: return "SPACE";
        case KEY_F1: return "F1"; case KEY_F2: return "F2"; case KEY_F3: return "F3";
        case KEY_F4: return "F4"; case KEY_F5: return "F5"; case KEY_F6: return "F6";
        case KEY_F7: return "F7"; case KEY_F8: return "F8"; case KEY_F9: return "F9";
        case KEY_F10: return "F10"; case KEY_F11: return "F11"; case KEY_F12: return "F12";
        case KEY_ESC: return "ESC"; case KEY_ENTER: return "ENTER"; case KEY_BACKSPACE: return "BACKSPACE";
        case KEY_TAB: return "TAB"; case KEY_CAPSLOCK: return "CAPSLOCK";
        case KEY_LSHIFT: return "LSHIFT"; case KEY_RSHIFT: return "RSHIFT";
        case KEY_LCTRL: return "LCTRL"; case KEY_RCTRL: return "RCTRL";
        case KEY_LALT: return "LALT"; case KEY_RALT: return "RALT";
        case KEY_INS: return "INS"; case KEY_DEL: return "DEL";
        case KEY_HOME: return "HOME"; case KEY_END: return "END";
        case KEY_PGUP: return "PGUP"; case KEY_PGDN: return "PGDN";
        case KEY_UP: return "UP"; case KEY_DOWN: return "DOWN";
        case KEY_LEFT: return "LEFT"; case KEY_RIGHT: return "RIGHT";
        case KEY_NUMLOCK: return "NUMLOCK"; case KEY_SCROLLLOCK: return "SCROLLLOCK";
        case KEY_KP_0: return "KP_0"; case KEY_KP_1: return "KP_1"; case KEY_KP_2: return "KP_2";
        case KEY_KP_3: return "KP_3"; case KEY_KP_4: return "KP_4"; case KEY_KP_5: return "KP_5";
        case KEY_KP_6: return "KP_6"; case KEY_KP_7: return "KP_7"; case KEY_KP_8: return "KP_8";
        case KEY_KP_9: return "KP_9";
        case KEY_KP_DECIMAL: return "KP_."; case KEY_KP_DIV: return "KP_/";
        case KEY_KP_MUL: return "KP_*"; case KEY_KP_SUB: return "KP_-";
        case KEY_KP_ADD: return "KP_+"; case KEY_KP_ENTER: return "KP_ENTER";
        case KEY_PRINTSCREEN: return "PRINTSCREEN"; case KEY_PAUSE: return "PAUSE";
        case KEY_LMETA: return "LMETA"; case KEY_RMETA: return "RMETA"; case KEY_MENU: return "MENU";
        default: return "UNKNOWN";
    }
}

// Scan Code Set 1 Map (0x00 - 0x58)
static enum KeyCode scan_code_map[128] = {
    KEY_UNKNOWN, KEY_ESC, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, // 0x00 - 0x09
    KEY_9, KEY_0, KEY_MINUS, KEY_EQUAL, KEY_BACKSPACE, KEY_TAB, KEY_Q, KEY_W, KEY_E, KEY_R, // 0x0A - 0x13
    KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P, KEY_LBRACKET, KEY_RBRACKET, KEY_ENTER, KEY_LCTRL, // 0x14 - 0x1D
    KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON, // 0x1E - 0x27
    KEY_APOSTROPHE, KEY_GRAVE, KEY_LSHIFT, KEY_BACKSLASH, KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, // 0x28 - 0x31
    KEY_M, KEY_COMMA, KEY_DOT, KEY_SLASH, KEY_RSHIFT, KEY_KP_MUL, KEY_LALT, KEY_SPACE, KEY_CAPSLOCK, KEY_F1, // 0x32 - 0x3B
    KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_NUMLOCK, // 0x3C - 0x45
    KEY_SCROLLLOCK, KEY_KP_7, KEY_KP_8, KEY_KP_9, KEY_KP_SUB, KEY_KP_4, KEY_KP_5, KEY_KP_6, KEY_KP_ADD, KEY_KP_1, // 0x46 - 0x4F
    KEY_KP_2, KEY_KP_3, KEY_KP_0, KEY_KP_DECIMAL, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_F11, KEY_F12, KEY_UNKNOWN, // 0x50 - 0x59
};

void keyboard_set_callback(keyboard_callback_t callback) {
    active_callback = callback;
}

static enum KeyCode resolve_keycode(uint8_t scancode, bool extended) {
    if (!extended) {
        if (scancode > 0x58) return KEY_UNKNOWN;
        return scan_code_map[scancode];
    } else {
        switch (scancode) {
            case 0x1C: return KEY_KP_ENTER;
            case 0x1D: return KEY_RCTRL;
            case 0x35: return KEY_KP_DIV;
            case 0x37: return KEY_PRINTSCREEN;
            case 0x38: return KEY_RALT;
            case 0x47: return KEY_HOME;
            case 0x48: return KEY_UP;
            case 0x49: return KEY_PGUP;
            case 0x4B: return KEY_LEFT;
            case 0x4D: return KEY_RIGHT;
            case 0x4F: return KEY_END;
            case 0x50: return KEY_DOWN;
            case 0x51: return KEY_PGDN;
            case 0x52: return KEY_INS;
            case 0x53: return KEY_DEL;
            case 0x5B: return KEY_LMETA;
            case 0x5C: return KEY_RMETA;
            case 0x5D: return KEY_MENU;
            default: return KEY_UNKNOWN;
        }
    }
}

char keycode_to_char(enum KeyCode code, bool shift, bool caps, bool numlock) {
    bool shift_state = shift ^ caps;
    bool num_shift = shift;

    if (code >= KEY_A && code <= KEY_Z) {
        return shift_state ? ('A' + (code - KEY_A)) : ('a' + (code - KEY_A));
    }

    // Keypad Logic
    if (code >= KEY_KP_0 && code <= KEY_KP_9) {
        if (numlock && !shift) { // Usually Shift + Keypad digits acts as navigation
            return '0' + (code - KEY_KP_0);
        }
        return 0; // Navigation (handled elsewhere via key_code)
    }

    switch (code) {
        case KEY_GRAVE: return num_shift ? '~' : '`';
        case KEY_MINUS: return num_shift ? '_' : '-';
        case KEY_EQUAL: return num_shift ? '+' : '=';
        case KEY_LBRACKET: return num_shift ? '{' : '[';
        case KEY_RBRACKET: return num_shift ? '}' : ']';
        case KEY_BACKSLASH: return num_shift ? '|' : '\\';
        case KEY_SEMICOLON: return num_shift ? ':' : ';';
        case KEY_APOSTROPHE: return num_shift ? '"' : '\'';
        case KEY_COMMA: return num_shift ? '<' : ',';
        case KEY_DOT: return num_shift ? '>' : '.';
        case KEY_SLASH: return num_shift ? '?' : '/';
        case KEY_SPACE: return ' ';

        case KEY_1: return num_shift ? '!' : '1';
        case KEY_2: return num_shift ? '@' : '2';
        case KEY_3: return num_shift ? '#' : '3';
        case KEY_4: return num_shift ? '$' : '4';
        case KEY_5: return num_shift ? '%' : '5';
        case KEY_6: return num_shift ? '^' : '6';
        case KEY_7: return num_shift ? '&' : '7';
        case KEY_8: return num_shift ? '*' : '8';
        case KEY_9: return num_shift ? '(' : '9';
        case KEY_0: return num_shift ? ')' : '0';

        case KEY_KP_DECIMAL: return (numlock && !shift) ? '.' : 0;
        case KEY_KP_DIV: return '/';
        case KEY_KP_MUL: return '*';
        case KEY_KP_SUB: return '-';
        case KEY_KP_ADD: return '+';

        default: return 0;
    }
}

void keyboard_handler(InteruptReg *r) {
    (void)r;
    uint8_t scancode = in_port_b(0x60);

    if (scancode == 0xE0) {
        is_extended = true;
        return;
    }

    bool released = scancode & 0x80;
    uint8_t make_code = scancode & 0x7F;

    enum KeyCode key = resolve_keycode(make_code, is_extended);

    // Update Modifiers
    if (key == KEY_LSHIFT) shift_l = !released;
    if (key == KEY_RSHIFT) shift_r = !released;
    if (key == KEY_LCTRL)  ctrl_l  = !released;
    if (key == KEY_RCTRL)  ctrl_r  = !released;
    if (key == KEY_LALT)   alt_l   = !released;
    if (key == KEY_RALT)   alt_r   = !released;
    if (key == KEY_CAPSLOCK && !released) caps_lock = !caps_lock;
    if (key == KEY_NUMLOCK && !released) num_lock = !num_lock;

    // Toggle Debug Mode and Cursor with F1 (Fn1)
    if (key == KEY_F1 && !released) {
        debug_mode = !debug_mode;
        if (debug_mode) {
            terminal_enable_cursor(0, 15);
        } else {
            terminal_disable_cursor(); // This uses 0x20
        }
    }

    KeyEvent event;
    event.key_code = key;
    event.scancode = scancode;
    event.pressed = !released;
    event.shift_active = shift_l | shift_r;
    event.ctrl_active = ctrl_l | ctrl_r;
    event.alt_active = alt_l | alt_r;
    event.caps_lock = caps_lock;
    event.num_lock = num_lock;
    event.character = keycode_to_char(key, event.shift_active, caps_lock, num_lock);

    is_extended = false;

    if (debug_mode) {
        if (key == KEY_UNKNOWN) {
            printf("Unknown Scancode: 0x%x (%s)\n", scancode, released ? "Released" : "Pressed");
        } else if (event.character == 0 && event.pressed) {
            // Log special keys (those without an ASCII char) only when pressed
            // Exclude arrow keys, shift, capslock, and numlock from debug logging as requested
            if (key != KEY_UP && key != KEY_DOWN && key != KEY_LEFT && key != KEY_RIGHT &&
                key != KEY_LSHIFT && key != KEY_RSHIFT && key != KEY_CAPSLOCK && key != KEY_NUMLOCK && key != KEY_DEL && key != KEY_BACKSPACE) {
                printf("Special Key: %s\n", keycode_to_string(key));
            }
        }
    }

    if (active_callback) {
        active_callback(event);
    } else {
        if (event.pressed) {
            if (event.character) {
                printf("%c", event.character);
            } else {
                // Determine if keypad should act as navigation
                enum KeyCode effective_key = event.key_code;

                // Keypad navigation fallback when NumLock is off or Shift is held
                if (event.key_code >= KEY_KP_0 && event.key_code <= KEY_KP_9) {
                    switch (event.key_code) {
                        case KEY_KP_8: effective_key = KEY_UP; break;
                        case KEY_KP_2: effective_key = KEY_DOWN; break;
                        case KEY_KP_4: effective_key = KEY_LEFT; break;
                        case KEY_KP_6: effective_key = KEY_RIGHT; break;
                        case KEY_KP_7: effective_key = KEY_HOME; break;
                        case KEY_KP_1: effective_key = KEY_END; break;
                        case KEY_KP_9: effective_key = KEY_PGUP; break;
                        case KEY_KP_3: effective_key = KEY_PGDN; break;
                        case KEY_KP_0: effective_key = KEY_INS; break;
                        case KEY_KP_DECIMAL: effective_key = KEY_DEL; break;
                        default: break;
                    }
                }

                switch (effective_key) {
                    case KEY_UP: if (debug_mode) terminal_cursor_up(); break;
                    case KEY_DOWN: if (debug_mode) terminal_cursor_down(); break;
                    case KEY_LEFT: if (debug_mode) terminal_cursor_left(); break;
                    case KEY_RIGHT: if (debug_mode) terminal_cursor_right(); break;
                    case KEY_ENTER:
                    case KEY_KP_ENTER: printf("\n"); break;
                    case KEY_BACKSPACE: printf("\b"); break;
                    case KEY_DEL: terminal_delete_char(); break;
                    default: break;
                }
            }
        }
    }
}

void init_keyboard() {
    shift_l = false; shift_r = false;
    ctrl_l = false; ctrl_r = false;
    alt_l = false; alt_r = false;
    caps_lock = false;
    num_lock = true;
    is_extended = false;
    active_callback = NULL;

    irq_install_handler(1, &keyboard_handler);
}

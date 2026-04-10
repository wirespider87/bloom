#ifndef BLOOM_CORE_RUNTIME_INPUT_H
#define BLOOM_CORE_RUNTIME_INPUT_H

#include "core/base/types/types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    /* Bloom key codes intentionally match Win32 VK_* values. */
    BLOOM_KEY_NONE = 0x00,
    BLOOM_KEY_BACKSPACE = 0x08,
    BLOOM_KEY_TAB = 0x09,
    BLOOM_KEY_CLEAR = 0x0C,
    BLOOM_KEY_ENTER = 0x0D,
    BLOOM_KEY_SHIFT = 0x10,
    BLOOM_KEY_CONTROL = 0x11,
    BLOOM_KEY_CTRL = BLOOM_KEY_CONTROL,
    BLOOM_KEY_ALT = 0x12,
    BLOOM_KEY_PAUSE = 0x13,
    BLOOM_KEY_CAPS_LOCK = 0x14,
    BLOOM_KEY_ESCAPE = 0x1B,
    BLOOM_KEY_SPACE = 0x20,
    BLOOM_KEY_PAGE_UP = 0x21,
    BLOOM_KEY_PAGE_DOWN = 0x22,
    BLOOM_KEY_END = 0x23,
    BLOOM_KEY_HOME = 0x24,
    BLOOM_KEY_LEFT = 0x25,
    BLOOM_KEY_UP = 0x26,
    BLOOM_KEY_RIGHT = 0x27,
    BLOOM_KEY_DOWN = 0x28,
    BLOOM_KEY_SELECT = 0x29,
    BLOOM_KEY_PRINT = 0x2A,
    BLOOM_KEY_EXECUTE = 0x2B,
    BLOOM_KEY_PRINT_SCREEN = 0x2C,
    BLOOM_KEY_INSERT = 0x2D,
    BLOOM_KEY_DELETE = 0x2E,
    BLOOM_KEY_HELP = 0x2F,
    BLOOM_KEY_0 = 0x30,
    BLOOM_KEY_1 = 0x31,
    BLOOM_KEY_2 = 0x32,
    BLOOM_KEY_3 = 0x33,
    BLOOM_KEY_4 = 0x34,
    BLOOM_KEY_5 = 0x35,
    BLOOM_KEY_6 = 0x36,
    BLOOM_KEY_7 = 0x37,
    BLOOM_KEY_8 = 0x38,
    BLOOM_KEY_9 = 0x39,
    BLOOM_KEY_A = 0x41,
    BLOOM_KEY_B = 0x42,
    BLOOM_KEY_C = 0x43,
    BLOOM_KEY_D = 0x44,
    BLOOM_KEY_E = 0x45,
    BLOOM_KEY_F = 0x46,
    BLOOM_KEY_G = 0x47,
    BLOOM_KEY_H = 0x48,
    BLOOM_KEY_I = 0x49,
    BLOOM_KEY_J = 0x4A,
    BLOOM_KEY_K = 0x4B,
    BLOOM_KEY_L = 0x4C,
    BLOOM_KEY_M = 0x4D,
    BLOOM_KEY_N = 0x4E,
    BLOOM_KEY_O = 0x4F,
    BLOOM_KEY_P = 0x50,
    BLOOM_KEY_Q = 0x51,
    BLOOM_KEY_R = 0x52,
    BLOOM_KEY_S = 0x53,
    BLOOM_KEY_T = 0x54,
    BLOOM_KEY_U = 0x55,
    BLOOM_KEY_V = 0x56,
    BLOOM_KEY_W = 0x57,
    BLOOM_KEY_X = 0x58,
    BLOOM_KEY_Y = 0x59,
    BLOOM_KEY_Z = 0x5A,
    BLOOM_KEY_LEFT_WINDOWS = 0x5B,
    BLOOM_KEY_RIGHT_WINDOWS = 0x5C,
    BLOOM_KEY_APPLICATION = 0x5D,
    BLOOM_KEY_SLEEP = 0x5F,
    BLOOM_KEY_NUMPAD_0 = 0x60,
    BLOOM_KEY_NUMPAD_1 = 0x61,
    BLOOM_KEY_NUMPAD_2 = 0x62,
    BLOOM_KEY_NUMPAD_3 = 0x63,
    BLOOM_KEY_NUMPAD_4 = 0x64,
    BLOOM_KEY_NUMPAD_5 = 0x65,
    BLOOM_KEY_NUMPAD_6 = 0x66,
    BLOOM_KEY_NUMPAD_7 = 0x67,
    BLOOM_KEY_NUMPAD_8 = 0x68,
    BLOOM_KEY_NUMPAD_9 = 0x69,
    BLOOM_KEY_NUMPAD_MULTIPLY = 0x6A,
    BLOOM_KEY_NUMPAD_ADD = 0x6B,
    BLOOM_KEY_NUMPAD_SEPARATOR = 0x6C,
    BLOOM_KEY_NUMPAD_SUBTRACT = 0x6D,
    BLOOM_KEY_NUMPAD_DECIMAL = 0x6E,
    BLOOM_KEY_NUMPAD_DIVIDE = 0x6F,
    BLOOM_KEY_F1 = 0x70,
    BLOOM_KEY_F2 = 0x71,
    BLOOM_KEY_F3 = 0x72,
    BLOOM_KEY_F4 = 0x73,
    BLOOM_KEY_F5 = 0x74,
    BLOOM_KEY_F6 = 0x75,
    BLOOM_KEY_F7 = 0x76,
    BLOOM_KEY_F8 = 0x77,
    BLOOM_KEY_F9 = 0x78,
    BLOOM_KEY_F10 = 0x79,
    BLOOM_KEY_F11 = 0x7A,
    BLOOM_KEY_F12 = 0x7B,
    BLOOM_KEY_F13 = 0x7C,
    BLOOM_KEY_F14 = 0x7D,
    BLOOM_KEY_F15 = 0x7E,
    BLOOM_KEY_F16 = 0x7F,
    BLOOM_KEY_F17 = 0x80,
    BLOOM_KEY_F18 = 0x81,
    BLOOM_KEY_F19 = 0x82,
    BLOOM_KEY_F20 = 0x83,
    BLOOM_KEY_F21 = 0x84,
    BLOOM_KEY_F22 = 0x85,
    BLOOM_KEY_F23 = 0x86,
    BLOOM_KEY_F24 = 0x87,
    BLOOM_KEY_NUM_LOCK = 0x90,
    BLOOM_KEY_SCROLL_LOCK = 0x91,
    BLOOM_KEY_LEFT_SHIFT = 0xA0,
    BLOOM_KEY_RIGHT_SHIFT = 0xA1,
    BLOOM_KEY_LEFT_CONTROL = 0xA2,
    BLOOM_KEY_RIGHT_CONTROL = 0xA3,
    BLOOM_KEY_LEFT_ALT = 0xA4,
    BLOOM_KEY_RIGHT_ALT = 0xA5,
    BLOOM_KEY_BROWSER_BACK = 0xA6,
    BLOOM_KEY_BROWSER_FORWARD = 0xA7,
    BLOOM_KEY_BROWSER_REFRESH = 0xA8,
    BLOOM_KEY_BROWSER_STOP = 0xA9,
    BLOOM_KEY_BROWSER_SEARCH = 0xAA,
    BLOOM_KEY_BROWSER_FAVORITES = 0xAB,
    BLOOM_KEY_BROWSER_HOME = 0xAC,
    BLOOM_KEY_VOLUME_MUTE = 0xAD,
    BLOOM_KEY_VOLUME_DOWN = 0xAE,
    BLOOM_KEY_VOLUME_UP = 0xAF,
    BLOOM_KEY_MEDIA_NEXT_TRACK = 0xB0,
    BLOOM_KEY_MEDIA_PREV_TRACK = 0xB1,
    BLOOM_KEY_MEDIA_STOP = 0xB2,
    BLOOM_KEY_MEDIA_PLAY_PAUSE = 0xB3,
    BLOOM_KEY_LAUNCH_MAIL = 0xB4,
    BLOOM_KEY_LAUNCH_MEDIA_SELECT = 0xB5,
    BLOOM_KEY_LAUNCH_APP_1 = 0xB6,
    BLOOM_KEY_LAUNCH_APP_2 = 0xB7,
    BLOOM_KEY_SEMICOLON = 0xBA,
    BLOOM_KEY_EQUALS = 0xBB,
    BLOOM_KEY_COMMA = 0xBC,
    BLOOM_KEY_MINUS = 0xBD,
    BLOOM_KEY_PERIOD = 0xBE,
    BLOOM_KEY_SLASH = 0xBF,
    BLOOM_KEY_GRAVE = 0xC0,
    BLOOM_KEY_LEFT_BRACKET = 0xDB,
    BLOOM_KEY_BACKSLASH = 0xDC,
    BLOOM_KEY_RIGHT_BRACKET = 0xDD,
    BLOOM_KEY_APOSTROPHE = 0xDE,
    BLOOM_KEY_OEM_8 = 0xDF,
    BLOOM_KEY_OEM_102 = 0xE2,
    BLOOM_KEY_PROCESS = 0xE5,
    BLOOM_KEY_PACKET = 0xE7,
    BLOOM_KEY_ATTN = 0xF6,
    BLOOM_KEY_CRSEL = 0xF7,
    BLOOM_KEY_EXSEL = 0xF8,
    BLOOM_KEY_EREOF = 0xF9,
    BLOOM_KEY_PLAY = 0xFA,
    BLOOM_KEY_ZOOM = 0xFB,
    BLOOM_KEY_NONAME = 0xFC,
    BLOOM_KEY_PA1 = 0xFD,
    BLOOM_KEY_OEM_CLEAR = 0xFE,
    BLOOM_KEY_COUNT = 0x100
};

enum
{
    BLOOM_MOUSE_LEFT   = 0,
    BLOOM_MOUSE_RIGHT  = 1,
    BLOOM_MOUSE_MIDDLE = 2,
    BLOOM_MOUSE_COUNT  = 3
};

typedef struct bloom_input
{
    bloom_vec2    mouse_pos;
    bloom_vec2    mouse_delta;
    float         mouse_wheel;
    bool          mouse_down[BLOOM_MOUSE_COUNT];
    bool          mouse_pressed[BLOOM_MOUSE_COUNT];
    bool          mouse_released[BLOOM_MOUSE_COUNT];

    bool          keys_down[BLOOM_KEY_COUNT];
    bool          keys_pressed[BLOOM_KEY_COUNT];
    bool          keys_released[BLOOM_KEY_COUNT];

    bool          ctrl_held;
    bool          shift_held;
    bool          alt_held;

    char          text_input[64];
    int           text_input_len;
} bloom_input;

#if defined(BLOOM_INTERNAL_BUILD) || defined(BLOOM_ENABLE_LEGACY_API)
void bloom_input_begin(bloom_input *input);
void bloom_input_set_mouse_pos(bloom_input *input, float x, float y);
void bloom_input_set_mouse_button(bloom_input *input, int button, bool down);
void bloom_input_set_mouse_wheel(bloom_input *input, float delta);
void bloom_input_set_key(bloom_input *input, int key, bool down);
void bloom_input_add_char(bloom_input *input, char c);
#endif

#ifdef __cplusplus
}
#endif

#endif

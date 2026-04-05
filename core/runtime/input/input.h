#ifndef BLOOM_CORE_RUNTIME_INPUT_H
#define BLOOM_CORE_RUNTIME_INPUT_H

#include "core/base/types/types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    BLOOM_KEY_NONE = 0,
    BLOOM_KEY_TAB,
    BLOOM_KEY_LEFT,
    BLOOM_KEY_RIGHT,
    BLOOM_KEY_UP,
    BLOOM_KEY_DOWN,
    BLOOM_KEY_HOME,
    BLOOM_KEY_END,
    BLOOM_KEY_DELETE,
    BLOOM_KEY_BACKSPACE,
    BLOOM_KEY_ENTER,
    BLOOM_KEY_ESCAPE,
    BLOOM_KEY_A,
    BLOOM_KEY_C,
    BLOOM_KEY_V,
    BLOOM_KEY_X,
    BLOOM_KEY_Y,
    BLOOM_KEY_Z,
    BLOOM_KEY_COUNT
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
    bloom_f32     mouse_wheel;
    bloom_bool    mouse_down[BLOOM_MOUSE_COUNT];
    bloom_bool    mouse_pressed[BLOOM_MOUSE_COUNT];
    bloom_bool    mouse_released[BLOOM_MOUSE_COUNT];

    bloom_bool    keys_down[BLOOM_KEY_COUNT];
    bloom_bool    keys_pressed[BLOOM_KEY_COUNT];
    bloom_bool    keys_released[BLOOM_KEY_COUNT];

    bloom_bool    ctrl_held;
    bloom_bool    shift_held;
    bloom_bool    alt_held;

    char          text_input[64];
    bloom_i32     text_input_len;
} bloom_input;

#if defined(BLOOM_INTERNAL_BUILD) || defined(BLOOM_ENABLE_LEGACY_API)
void bloom_input_begin(bloom_input *input);
void bloom_input_set_mouse_pos(bloom_input *input, bloom_f32 x, bloom_f32 y);
void bloom_input_set_mouse_button(bloom_input *input, int button, bloom_bool down);
void bloom_input_set_mouse_wheel(bloom_input *input, bloom_f32 delta);
void bloom_input_set_key(bloom_input *input, int key, bloom_bool down);
void bloom_input_add_char(bloom_input *input, char c);
#endif

#ifdef __cplusplus
}
#endif

#endif

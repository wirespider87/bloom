#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "core/runtime/input/input.h"
#include <string.h>

void bloom_input_begin(bloom_input *input)
{
    int i;
    bloom_vec2 prev_pos = input->mouse_pos;

    for (i = 0; i < BLOOM_MOUSE_COUNT; i++)
    {
        input->mouse_pressed[i] = BLOOM_FALSE;
        input->mouse_released[i] = BLOOM_FALSE;
    }
    for (i = 0; i < BLOOM_KEY_COUNT; i++)
    {
        input->keys_pressed[i] = BLOOM_FALSE;
        input->keys_released[i] = BLOOM_FALSE;
    }
    input->mouse_delta.x = 0;
    input->mouse_delta.y = 0;
    input->mouse_wheel = 0;
    input->text_input_len = 0;
    input->text_input[0] = '\0';
    input->mouse_pos = prev_pos;
}

void bloom_input_set_mouse_pos(bloom_input *input, float x, float y)
{
    float old_x = input->mouse_pos.x;
    float old_y = input->mouse_pos.y;
    input->mouse_pos.x = x;
    input->mouse_pos.y = y;
    input->mouse_delta.x = x - old_x;
    input->mouse_delta.y = y - old_y;
}

void bloom_input_set_mouse_button(bloom_input *input, int button, bool down)
{
    if (button < 0 || button >= BLOOM_MOUSE_COUNT)
    {
        return;
    }
    if (down && !input->mouse_down[button])
    {
        input->mouse_pressed[button] = BLOOM_TRUE;
    }
    if (!down && input->mouse_down[button])
    {
        input->mouse_released[button] = BLOOM_TRUE;
    }
    input->mouse_down[button] = down;
}

void bloom_input_set_mouse_wheel(bloom_input *input, float delta)
{
    input->mouse_wheel = delta;
}

void bloom_input_set_key(bloom_input *input, int key, bool down)
{
    unsigned int key_index = (unsigned int)key;

    if (key_index == BLOOM_KEY_NONE || key_index >= BLOOM_KEY_COUNT)
    {
        return;
    }
    if (down && !input->keys_down[key_index])
    {
        input->keys_pressed[key_index] = BLOOM_TRUE;
    }
    if (!down && input->keys_down[key_index])
    {
        input->keys_released[key_index] = BLOOM_TRUE;
    }
    input->keys_down[key_index] = down;
}

void bloom_input_add_char(bloom_input *input, char c)
{
    if (input->text_input_len < 63)
    {
        input->text_input[input->text_input_len++] = c;
        input->text_input[input->text_input_len] = '\0';
    }
}

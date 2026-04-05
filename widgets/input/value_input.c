#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "widgets/internal.h"
#include <stdlib.h>

#define BLOOM_NUMBER_STATE_CAPACITY 64
#define BLOOM_SCRUB_STATE_CAPACITY 32

typedef struct bloom_number_field_state
{
    bloom_id id;
    bloom_bool active;
    char buffer[64];
} bloom_number_field_state;

typedef struct bloom_scrub_state
{
    bloom_id id;
    bloom_bool active;
    bloom_f64 value;
} bloom_scrub_state;

static bloom_number_field_state g_number_states[BLOOM_NUMBER_STATE_CAPACITY];
static bloom_scrub_state g_scrub_states[BLOOM_SCRUB_STATE_CAPACITY];

static bloom_number_field_state *bloom_find_number_state(bloom_id id)
{
    bloom_i32 i;
    bloom_number_field_state *free_state = NULL;

    for (i = 0; i < BLOOM_NUMBER_STATE_CAPACITY; ++i)
    {
        if (g_number_states[i].active && g_number_states[i].id == id)
        {
            return &g_number_states[i];
        }
        if (!free_state && !g_number_states[i].active)
        {
            free_state = &g_number_states[i];
        }
    }

    if (!free_state)
    {
        free_state = &g_number_states[0];
    }

    free_state->active = BLOOM_TRUE;
    free_state->id = id;
    free_state->buffer[0] = '\0';
    return free_state;
}

static bloom_scrub_state *bloom_find_scrub_state(bloom_id id)
{
    bloom_i32 i;
    bloom_scrub_state *free_state = NULL;

    for (i = 0; i < BLOOM_SCRUB_STATE_CAPACITY; ++i)
    {
        if (g_scrub_states[i].active && g_scrub_states[i].id == id)
        {
            return &g_scrub_states[i];
        }
        if (!free_state && !g_scrub_states[i].active)
        {
            free_state = &g_scrub_states[i];
        }
    }

    if (!free_state)
    {
        free_state = &g_scrub_states[0];
    }

    free_state->active = BLOOM_TRUE;
    free_state->id = id;
    free_state->value = 0.0;
    return free_state;
}

static void bloom_trim_numeric_text(char *buf)
{
    char *dot;
    char *tail;

    if (!buf)
    {
        return;
    }

    dot = strchr(buf, '.');
    if (!dot)
    {
        return;
    }

    tail = buf + strlen(buf) - 1;
    while (tail > dot && *tail == '0')
    {
        *tail-- = '\0';
    }
    if (tail == dot)
    {
        *tail = '\0';
    }
}

static void bloom_format_numeric_value(char *buf, bloom_u32 buf_size,
                                       bloom_f64 value, bloom_value_kind kind)
{
    if (!buf || buf_size == 0)
    {
        return;
    }

    switch (kind)
    {
    case BLOOM_VALUE_KIND_INT:
        snprintf(buf, buf_size, "%d", (int)((value >= 0.0) ? floor(value + 0.5) : ceil(value - 0.5)));
        break;
    case BLOOM_VALUE_KIND_FLOAT:
        snprintf(buf, buf_size, "%.3f", (double)value);
        bloom_trim_numeric_text(buf);
        break;
    default:
        snprintf(buf, buf_size, "%.6f", (double)value);
        bloom_trim_numeric_text(buf);
        break;
    }
}

static bloom_bool bloom_parse_numeric_value(const char *buf, bloom_value_kind kind,
                                            bloom_f64 *out_value)
{
    char *end = NULL;
    bloom_f64 value;

    if (!buf || !buf[0] || !out_value)
    {
        return BLOOM_FALSE;
    }

    value = strtod(buf, &end);
    if (end == buf)
    {
        return BLOOM_FALSE;
    }

    while (*end == ' ' || *end == '\t')
    {
        end++;
    }
    if (*end != '\0')
    {
        return BLOOM_FALSE;
    }

    if (kind == BLOOM_VALUE_KIND_INT)
    {
        value = (value >= 0.0) ? floor(value + 0.5) : ceil(value - 0.5);
    }

    *out_value = value;
    return BLOOM_TRUE;
}

static bloom_f64 bloom_value_from_ptr(const void *value_ptr, bloom_value_kind kind)
{
    if (!value_ptr)
    {
        return 0.0;
    }

    switch (kind)
    {
    case BLOOM_VALUE_KIND_INT:
        return (bloom_f64)(*(const bloom_i32 *)value_ptr);
    case BLOOM_VALUE_KIND_FLOAT:
        return (bloom_f64)(*(const bloom_f32 *)value_ptr);
    default:
        return *(const bloom_f64 *)value_ptr;
    }
}

static bloom_bool bloom_value_store(void *value_ptr, bloom_value_kind kind,
                                    bloom_f64 value)
{
    if (!value_ptr)
    {
        return BLOOM_FALSE;
    }

    switch (kind)
    {
    case BLOOM_VALUE_KIND_INT:
    {
        bloom_i32 rounded = (bloom_i32)((value >= 0.0) ? floor(value + 0.5) : ceil(value - 0.5));
        if (*(bloom_i32 *)value_ptr != rounded)
        {
            *(bloom_i32 *)value_ptr = rounded;
            return BLOOM_TRUE;
        }
        break;
    }
    case BLOOM_VALUE_KIND_FLOAT:
    {
        bloom_f32 narrowed = (bloom_f32)value;
        if (fabs(*(bloom_f32 *)value_ptr - narrowed) > 0.00001f)
        {
            *(bloom_f32 *)value_ptr = narrowed;
            return BLOOM_TRUE;
        }
        break;
    }
    default:
        if (fabs(*(bloom_f64 *)value_ptr - value) > 0.0000001)
        {
            *(bloom_f64 *)value_ptr = value;
            return BLOOM_TRUE;
        }
        break;
    }

    return BLOOM_FALSE;
}

bloom_bool bloom_numeric_input(const char *label, void *value_ptr, bloom_value_kind kind)
{
    bloom_context *ctx = bloom_get_context();
    bloom_number_field_state *state;
    bloom_id input_id;
    bloom_bool focused;
    bloom_bool changed = BLOOM_FALSE;
    bloom_f64 current_value;
    bloom_f64 edited_value;

    if (!ctx || !ctx->current_window || !value_ptr)
    {
        return BLOOM_FALSE;
    }

    input_id = bloom_get_id(label);
    state = bloom_find_number_state(input_id);
    focused = (ctx->focus_id == input_id);
    current_value = bloom_value_from_ptr(value_ptr, kind);

    if (!focused || state->buffer[0] == '\0')
    {
        bloom_format_numeric_value(state->buffer, (bloom_u32)sizeof(state->buffer), current_value, kind);
    }

    if (bloom_text_input(label, state->buffer, (bloom_u32)sizeof(state->buffer)))
    {
        if (bloom_parse_numeric_value(state->buffer, kind, &edited_value))
        {
            changed |= bloom_value_store(value_ptr, kind, edited_value);
        }
    }

    if (ctx->focus_id != input_id)
    {
        current_value = bloom_value_from_ptr(value_ptr, kind);
        bloom_format_numeric_value(state->buffer, (bloom_u32)sizeof(state->buffer), current_value, kind);
    }

    return changed;
}

static bloom_bool bloom_value_field_internal(const char *label, void *value_ptr,
                                             bloom_value_kind kind, bloom_f64 step)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_vec2 pos;
    bloom_u32 visible_len;
    bloom_bool show_label;
    bloom_f32 label_h;
    bloom_f32 label_gap;
    bloom_f32 line_h;
    bloom_f32 control_h;
    bloom_f32 total_w;
    bloom_f32 button_w;
    bloom_f32 center_w;
    bloom_f32 saved_available;
    bloom_f64 current_value;
    bloom_f64 edited_value;
    bloom_id input_id;
    bloom_bool focused;
    bloom_bool changed = BLOOM_FALSE;
    bloom_number_field_state *state;

    if (!ctx || !ctx->current_window || !value_ptr)
    {
        return BLOOM_FALSE;
    }

    if (step <= 0.0)
    {
        step = (kind == BLOOM_VALUE_KIND_INT) ? 1.0 : (kind == BLOOM_VALUE_KIND_FLOAT ? 0.05 : 0.01);
    }

    s = &ctx->style;
    pos = ctx->current_window->layout.cursor;
    visible_len = bloom_label_visible_length(label);
    show_label = visible_len > 0;
    label_h = show_label ? bloom_scaled_line_height(ctx, s->font_size * 0.9f) : 0.0f;
    label_gap = show_label ? s->label_gap : 0.0f;
    line_h = bloom_scaled_line_height(ctx, s->font_size);
    control_h = line_h + s->field_padding_y * 2.0f;
    total_w = ctx->current_window->layout.available_width;
    button_w = control_h;
    center_w = total_w - button_w * 2.0f - s->item_inner_spacing * 2.0f;
    if (center_w < 56.0f)
    {
        center_w = 56.0f;
    }

    bloom_push_id(label);
    input_id = bloom_get_id("##value");
    state = bloom_find_number_state(input_id);
    focused = (ctx->focus_id == input_id);
    current_value = bloom_value_from_ptr(value_ptr, kind);

    if (!focused || state->buffer[0] == '\0')
    {
        bloom_format_numeric_value(state->buffer, (bloom_u32)sizeof(state->buffer), current_value, kind);
    }

    if (show_label)
    {
        bloom_draw_label(ctx,
            bloom_v2(pos.x, pos.y),
            label,
            focused ? s->input_cursor : s->text_disabled,
            s->font_size * 0.9f);
    }

    bloom_set_cursor_pos(pos.x, pos.y + label_h + label_gap);
    if (bloom_button_sized("-##decrement", button_w, control_h))
    {
        current_value -= step;
        changed |= bloom_value_store(value_ptr, kind, current_value);
        current_value = bloom_value_from_ptr(value_ptr, kind);
        bloom_format_numeric_value(state->buffer, (bloom_u32)sizeof(state->buffer), current_value, kind);
    }

    bloom_same_line();
    saved_available = ctx->current_window->layout.available_width;
    ctx->current_window->layout.available_width = center_w;
    if (bloom_text_input("##value", state->buffer, (bloom_u32)sizeof(state->buffer)))
    {
        if (bloom_parse_numeric_value(state->buffer, kind, &edited_value))
        {
            changed |= bloom_value_store(value_ptr, kind, edited_value);
        }
    }
    ctx->current_window->layout.available_width = saved_available;

    bloom_same_line();
    if (bloom_button_sized("+##increment", button_w, control_h))
    {
        current_value = bloom_value_from_ptr(value_ptr, kind) + step;
        changed |= bloom_value_store(value_ptr, kind, current_value);
        current_value = bloom_value_from_ptr(value_ptr, kind);
        bloom_format_numeric_value(state->buffer, (bloom_u32)sizeof(state->buffer), current_value, kind);
    }

    focused = (ctx->focus_id == input_id);
    if (!focused)
    {
        current_value = bloom_value_from_ptr(value_ptr, kind);
        bloom_format_numeric_value(state->buffer, (bloom_u32)sizeof(state->buffer), current_value, kind);
    }

    bloom_pop_id();
    return changed;
}

static bloom_bool bloom_value_scrub_internal(const char *label, void *value_ptr,
                                             bloom_value_kind kind, bloom_f64 min_value,
                                             bloom_f64 max_value, bloom_f64 step_per_pixel)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_vec2 pos;
    bloom_u32 visible_len;
    bloom_bool show_label;
    bloom_f32 label_h;
    bloom_f32 label_gap;
    bloom_f32 line_h;
    bloom_f32 control_h;
    bloom_f32 total_w;
    bloom_rect rect;
    bloom_id id;
    bloom_bool hovered;
    bloom_bool changed = BLOOM_FALSE;
    bloom_color bg;
    bloom_color border;
    bloom_color handle_col;
    bloom_f64 current_value;
    bloom_scrub_state *state;
    char value_text[64];
    bloom_f32 value_w;
    bloom_f32 marker_x;
    bloom_f32 handle_x;
    bloom_f32 handle_y;
    bloom_i32 dot_index;

    if (!ctx || !ctx->current_window || !value_ptr)
    {
        return BLOOM_FALSE;
    }

    if (step_per_pixel <= 0.0)
    {
        step_per_pixel = (kind == BLOOM_VALUE_KIND_INT) ? 1.0 : (kind == BLOOM_VALUE_KIND_FLOAT ? 0.02 : 0.01);
    }

    s = &ctx->style;
    pos = ctx->current_window->layout.cursor;
    visible_len = bloom_label_visible_length(label);
    show_label = visible_len > 0;
    label_h = show_label ? bloom_scaled_line_height(ctx, s->font_size * 0.9f) : 0.0f;
    label_gap = show_label ? s->label_gap : 0.0f;
    line_h = bloom_scaled_line_height(ctx, s->font_size);
    control_h = line_h + s->field_padding_y * 2.0f;
    total_w = ctx->current_window->layout.available_width;
    rect = bloom_make_rect(pos.x, pos.y + label_h + label_gap, total_w, control_h);
    id = bloom_get_id(label);
    hovered = bloom_widget_hovered(rect);
    state = bloom_find_scrub_state(id);
    current_value = bloom_value_from_ptr(value_ptr, kind);

    if (show_label)
    {
        bloom_draw_label(ctx,
            bloom_v2(pos.x, pos.y),
            label,
            (hovered || ctx->active_id == id) ? s->input_cursor : s->text_disabled,
            s->font_size * 0.9f);
    }

    if (hovered)
    {
        ctx->hot_id = id;
        if (ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT])
        {
            ctx->active_id = id;
            state->value = current_value;
        }
    }

    if (ctx->active_id == id)
    {
        if (ctx->input.mouse_down[BLOOM_MOUSE_LEFT])
        {
            current_value += (bloom_f64)ctx->input.mouse_delta.x * step_per_pixel;
            if (current_value < min_value) current_value = min_value;
            if (current_value > max_value) current_value = max_value;
            changed |= bloom_value_store(value_ptr, kind, current_value);
        }
        else
        {
            ctx->active_id = 0;
        }
    }

    bg = s->input_bg;
    if (hovered)
    {
        bg = bloom_apply_state_layer(bg, s->input_cursor, 0.05f);
    }
    if (ctx->active_id == id)
    {
        bg = bloom_apply_state_layer(bg, s->input_cursor, 0.10f);
    }

    border = s->input_border;
    if (hovered)
    {
        border = bloom_color_mix(s->input_border, s->input_cursor, 0.45f);
    }
    if (ctx->active_id == id)
    {
        border = s->input_cursor;
    }
    handle_col = (ctx->active_id == id) ? s->input_cursor : s->text_disabled;

    bloom_draw_rect_rounded(&ctx->draw_list, rect, bg, s->input_rounding);
    bloom_draw_rect_rounded_border(&ctx->draw_list,
                                   rect,
                                   border,
                                   s->input_rounding,
                                   (ctx->active_id == id) ? 2.0f : 1.0f);

    bloom_format_numeric_value(value_text, (bloom_u32)sizeof(value_text), bloom_value_from_ptr(value_ptr, kind), kind);
    value_w = bloom_text_width(value_text, s->font_size);
    bloom_draw_text(&ctx->draw_list,
        bloom_v2(rect.x + s->field_padding_x,
                 bloom_centered_text_y(ctx, rect.y, rect.h, s->font_size)),
        value_text,
        s->input_text,
        s->font_size,
        ctx->default_font.texture_id);

    handle_x = rect.x + rect.w - s->field_padding_x - 4.0f;
    handle_y = rect.y + rect.h * 0.5f - 6.0f;
    for (dot_index = 0; dot_index < 3; ++dot_index)
    {
        bloom_draw_circle_filled(&ctx->draw_list,
                                 bloom_v2(handle_x, handle_y + dot_index * 6.0f),
                                 1.4f,
                                 handle_col,
                                 s->circle_segments);
    }

    if (ctx->active_id == id)
    {
        marker_x = rect.x + s->field_padding_x + value_w + s->item_inner_spacing;
        bloom_draw_rect_filled(&ctx->draw_list,
            bloom_make_rect(marker_x, rect.y + 5.0f, 2.0f, rect.h - 10.0f),
            s->input_cursor);
    }

    bloom_advance_layout(total_w, label_h + label_gap + control_h + s->touch_padding);
    return changed;
}

bloom_bool bloom_int_field(const char *label, bloom_i32 *value, bloom_i32 step)
{
    return bloom_value_field_internal(label, value, BLOOM_VALUE_KIND_INT, (bloom_f64)step);
}

bloom_bool bloom_float_field(const char *label, bloom_f32 *value, bloom_f32 step)
{
    return bloom_value_field_internal(label, value, BLOOM_VALUE_KIND_FLOAT, (bloom_f64)step);
}

bloom_bool bloom_precise_field(const char *label, bloom_f64 *value, bloom_f64 step)
{
    return bloom_value_field_internal(label, value, BLOOM_VALUE_KIND_DOUBLE, step);
}

bloom_bool bloom_value_field(const char *label, void *value, bloom_value_kind kind, bloom_f64 step)
{
    return bloom_value_field_internal(label, value, kind, step);
}

bloom_bool bloom_int_scrub(const char *label, bloom_i32 *value, bloom_i32 min_val, bloom_i32 max_val,
                           bloom_f32 step_per_pixel)
{
    return bloom_value_scrub_internal(label, value, BLOOM_VALUE_KIND_INT,
                                      (bloom_f64)min_val, (bloom_f64)max_val,
                                      (bloom_f64)step_per_pixel);
}

bloom_bool bloom_float_scrub(const char *label, bloom_f32 *value, bloom_f32 min_val, bloom_f32 max_val,
                             bloom_f32 step_per_pixel)
{
    return bloom_value_scrub_internal(label, value, BLOOM_VALUE_KIND_FLOAT,
                                      (bloom_f64)min_val, (bloom_f64)max_val,
                                      (bloom_f64)step_per_pixel);
}

bloom_bool bloom_int_span(const char *label, bloom_i32 *min_value, bloom_i32 *max_value, bloom_i32 step)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_vec2 pos;
    bloom_bool changed = BLOOM_FALSE;
    bloom_u32 visible_len;
    bloom_bool show_label;
    bloom_f32 label_h;
    bloom_f32 label_gap;
    bloom_f32 total_w;
    bloom_f32 each_w;
    bloom_f32 saved_available;

    if (!ctx || !ctx->current_window)
    {
        return BLOOM_FALSE;
    }

    s = &ctx->style;
    pos = ctx->current_window->layout.cursor;
    visible_len = bloom_label_visible_length(label);
    show_label = visible_len > 0;
    label_h = show_label ? bloom_scaled_line_height(ctx, s->font_size * 0.9f) : 0.0f;
    label_gap = show_label ? s->label_gap : 0.0f;
    total_w = ctx->current_window->layout.available_width;
    each_w = (total_w - s->item_inner_spacing) * 0.5f;

    if (show_label)
    {
        bloom_draw_label(ctx, bloom_v2(pos.x, pos.y), label, s->text_disabled, s->font_size * 0.9f);
    }

    bloom_push_id(label);
    bloom_set_cursor_pos(pos.x, pos.y + label_h + label_gap);
    saved_available = ctx->current_window->layout.available_width;
    ctx->current_window->layout.available_width = each_w;
    changed |= bloom_numeric_input("##min", min_value, BLOOM_VALUE_KIND_INT);
    ctx->current_window->layout.available_width = saved_available;

    bloom_same_line();
    ctx->current_window->layout.available_width = each_w;
    changed |= bloom_numeric_input("##max", max_value, BLOOM_VALUE_KIND_INT);
    ctx->current_window->layout.available_width = saved_available;
    bloom_pop_id();

    if (*min_value > *max_value)
    {
        *max_value = *min_value;
        changed = BLOOM_TRUE;
    }
    return changed;
}

bloom_bool bloom_float_span(const char *label, bloom_f32 *min_value, bloom_f32 *max_value, bloom_f32 step)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_vec2 pos;
    bloom_bool changed = BLOOM_FALSE;
    bloom_u32 visible_len;
    bloom_bool show_label;
    bloom_f32 label_h;
    bloom_f32 label_gap;
    bloom_f32 total_w;
    bloom_f32 each_w;
    bloom_f32 saved_available;

    if (!ctx || !ctx->current_window)
    {
        return BLOOM_FALSE;
    }

    s = &ctx->style;
    pos = ctx->current_window->layout.cursor;
    visible_len = bloom_label_visible_length(label);
    show_label = visible_len > 0;
    label_h = show_label ? bloom_scaled_line_height(ctx, s->font_size * 0.9f) : 0.0f;
    label_gap = show_label ? s->label_gap : 0.0f;
    total_w = ctx->current_window->layout.available_width;
    each_w = (total_w - s->item_inner_spacing) * 0.5f;

    if (show_label)
    {
        bloom_draw_label(ctx, bloom_v2(pos.x, pos.y), label, s->text_disabled, s->font_size * 0.9f);
    }

    bloom_push_id(label);
    bloom_set_cursor_pos(pos.x, pos.y + label_h + label_gap);
    saved_available = ctx->current_window->layout.available_width;
    ctx->current_window->layout.available_width = each_w;
    changed |= bloom_numeric_input("##min", min_value, BLOOM_VALUE_KIND_FLOAT);
    ctx->current_window->layout.available_width = saved_available;

    bloom_same_line();
    ctx->current_window->layout.available_width = each_w;
    changed |= bloom_numeric_input("##max", max_value, BLOOM_VALUE_KIND_FLOAT);
    ctx->current_window->layout.available_width = saved_available;
    bloom_pop_id();

    if (*min_value > *max_value)
    {
        *max_value = *min_value;
        changed = BLOOM_TRUE;
    }
    return changed;
}
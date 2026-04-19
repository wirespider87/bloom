#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "widgets/internal.h"

static bloom_bool bloom_button_hit(bloom_context *ctx, bloom_id id, bloom_rect rect,
                                   bloom_bool *hovered_out, bloom_bool *held_out)
{
    bloom_bool hovered = bloom_widget_hovered(rect);
    bloom_bool held = (ctx->active_id == id);
    bloom_bool pressed = BLOOM_FALSE;

    if (hovered)
    {
        ctx->hot_id = id;
        if (ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT])
        {
            ctx->active_id = id;
            held = BLOOM_TRUE;
        }
    }

    if (ctx->active_id == id)
    {
        held = BLOOM_TRUE;
        if (ctx->input.mouse_released[BLOOM_MOUSE_LEFT])
        {
            if (hovered)
            {
                pressed = BLOOM_TRUE;
            }
            ctx->active_id = 0;
            held = BLOOM_FALSE;
        }
    }

    if (hovered_out)
    {
        *hovered_out = hovered;
    }
    if (held_out)
    {
        *held_out = held;
    }
    return pressed;
}

static bloom_corner_radii bloom_choice_strip_radii(bloom_i32 index, bloom_i32 count, bloom_f32 radius)
{
    if (count <= 1)
    {
        return bloom_make_corner_radii_all(radius);
    }
    if (index == 0)
    {
        return bloom_make_corner_radii(radius, 0.0f, 0.0f, radius);
    }
    if (index == count - 1)
    {
        return bloom_make_corner_radii(0.0f, radius, radius, 0.0f);
    }
    return bloom_make_corner_radii(0.0f, 0.0f, 0.0f, 0.0f);
}

bloom_bool bloom_button(const char *label)
{
    bloom_context *ctx = bloom_get_context();
    bloom_f32 text_w = bloom_label_width(ctx, label, ctx->style.font_size);
    bloom_f32 h = bloom_scaled_line_height(ctx, ctx->style.font_size) + ctx->style.control_padding_y * 2.0f;
    bloom_f32 w = text_w + ctx->style.control_padding_x * 2.0f;
    return bloom_button_sized(label, w, h);
}

bloom_bool bloom_button_sized(const char *label, bloom_f32 w, bloom_f32 h)
{
    bloom_context *ctx = bloom_get_context();
    if (!ctx || !ctx->current_window)
    {
        return BLOOM_FALSE;
    }

    bloom_id id = bloom_get_id(label);
    bloom_vec2 pos = ctx->current_window->layout.cursor;
    bloom_rect rect = bloom_make_rect(pos.x, pos.y, w, h);
    bloom_style *s = &ctx->style;

    bloom_bool hovered = BLOOM_FALSE;
    bloom_bool held = BLOOM_FALSE;
    bloom_bool pressed;

    bloom_color bg;
    bloom_color text_color;
    bloom_color outline = bloom_rgba(0, 0, 0, 0);
    bloom_color shadow = bloom_scale_alpha(s->shadow, 0.14f);
    bloom_f32 hover_t;
    bloom_f32 press_t;
    pressed = bloom_button_hit(ctx, id, rect, &hovered, &held);

    hover_t = bloom_window_anim_toggle(ctx, id ^ 0x5E01u, hovered || held, 18.0f);
    press_t = bloom_window_anim_pulse(ctx, id ^ 0x5E02u, held, 44.0f, 20.0f);

    bg = bloom_color_mix(s->button_bg, s->button_bg_hovered, hover_t);
    if (press_t > 0.001f)
        bg = bloom_color_mix(bg, s->button_bg_active, press_t);
    text_color = bloom_color_mix(s->button_text, s->text_default, hover_t * 0.35f);
    if (hover_t > 0.001f)
        outline = bloom_scale_alpha(s->button_text, hover_t * 0.10f + press_t * 0.06f);
    if (press_t > 0.001f)
        shadow = bloom_scale_alpha(s->shadow, 0.14f - press_t * 0.05f);

    /* hover state layer glow */
    if (hover_t > 0.001f || press_t > 0.001f)
    {
        bloom_draw_rect_rounded(&ctx->draw_list,
            bloom_make_rect(rect.x - 1.0f, rect.y - 1.0f, rect.w + 2.0f, rect.h + 2.0f),
            bloom_apply_state_layer(s->window_bg, s->input_cursor,
                                    0.03f + hover_t * 0.04f + press_t * 0.03f),
            s->button_rounding + 1.0f);
    }

    if (s->shadow_offset > 0.0f)
    {
        bloom_draw_rect_rounded(&ctx->draw_list,
                                bloom_make_rect(rect.x, rect.y + 2.0f, rect.w, rect.h),
                                shadow,
                                s->button_rounding);
    }

    bloom_draw_rect_rounded(&ctx->draw_list, rect, bg, s->button_rounding);
    if (outline.a > 0)
    {
        bloom_draw_rect_rounded_border(&ctx->draw_list, rect, outline, s->button_rounding, 1.0f);
    }

    bloom_f32 text_w = bloom_label_width(ctx, label, s->font_size);
    bloom_f32 text_x = rect.x + (rect.w - text_w) * 0.5f;
    bloom_f32 text_y = bloom_centered_text_y(ctx, rect.y, rect.h, s->font_size);
    
    if (bloom_label_visible_length(label) == 1)
    {
        char c = label[0];
        if (c == '+' || c == '-' || c == '*' || c == '/')
        {
            text_y += 1.0f; 
        }
    }
    
    bloom_draw_label(ctx, bloom_v2(text_x, text_y), label, text_color, s->font_size);

    bloom_advance_layout(w, h);
    return pressed;
}

bloom_bool bloom_button_mini(const char *label)
{
    bloom_context *ctx = bloom_get_context();
    bloom_f32 font_size;
    bloom_f32 text_w;
    bloom_f32 h;
    bloom_f32 w;

    if (!ctx)
    {
        return BLOOM_FALSE;
    }

    font_size = ctx->style.font_size * 0.82f;
    text_w = bloom_label_width(ctx, label, font_size);
    h = bloom_scaled_line_height(ctx, font_size) + ctx->style.control_padding_y * 1.2f;
    w = text_w + ctx->style.control_padding_x * 1.35f;
    return bloom_button_sized(label, w, h);
}

bloom_bool bloom_button_ghost(const char *id, bloom_f32 w, bloom_f32 h)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_vec2 pos;
    bloom_rect rect;
    bloom_id widget_id;
    bloom_bool hovered = BLOOM_FALSE;
    bloom_bool held = BLOOM_FALSE;
    bloom_bool pressed;
    bloom_color bg;
    bloom_color border;

    if (!ctx || !ctx->current_window)
    {
        return BLOOM_FALSE;
    }

    s = &ctx->style;
    pos = ctx->current_window->layout.cursor;
    if (w <= 0.0f)
    {
        w = bloom_scaled_line_height(ctx, s->font_size) * 3.5f;
    }
    if (h <= 0.0f)
    {
        h = bloom_scaled_line_height(ctx, s->font_size) + s->control_padding_y * 2.0f;
    }

    rect = bloom_make_rect(pos.x, pos.y, w, h);
    widget_id = bloom_get_id(id);
    pressed = bloom_button_hit(ctx, widget_id, rect, &hovered, &held);

    bg = bloom_rgba(0, 0, 0, 0);
    border = bloom_scale_alpha(s->input_border, 0.88f);
    if (hovered)
    {
        bg = bloom_apply_state_layer(s->input_bg, s->input_cursor, 0.08f);
        border = bloom_color_mix(s->input_border, s->input_cursor, 0.45f);
    }
    if (held)
    {
        bg = bloom_apply_state_layer(s->input_bg, s->input_cursor, 0.14f);
        border = s->input_cursor;
    }

    bloom_draw_rect_rounded(&ctx->draw_list, rect, bg, s->button_rounding);
    bloom_draw_rect_rounded_border(&ctx->draw_list, rect, border, s->button_rounding, 1.0f);

    bloom_advance_layout(w, h);
    return pressed;
}

bloom_bool bloom_button_direction(const char *id, bloom_direction direction)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_vec2 pos;
    bloom_rect rect;
    bloom_id widget_id;
    bloom_bool hovered = BLOOM_FALSE;
    bloom_bool held = BLOOM_FALSE;
    bloom_bool pressed;
    bloom_color bg;
    bloom_color border;
    bloom_color icon;
    bloom_f32 size;
    bloom_f32 cx;
    bloom_f32 cy;
    bloom_f32 r;
    bloom_vec2 a;
    bloom_vec2 b;
    bloom_vec2 c;

    if (!ctx || !ctx->current_window)
    {
        return BLOOM_FALSE;
    }

    s = &ctx->style;
    pos = ctx->current_window->layout.cursor;
    size = bloom_scaled_line_height(ctx, s->font_size) + s->control_padding_y * 1.8f;
    rect = bloom_make_rect(pos.x, pos.y, size, size);
    widget_id = bloom_get_id(id);
    pressed = bloom_button_hit(ctx, widget_id, rect, &hovered, &held);

    bg = s->input_bg;
    border = bloom_scale_alpha(s->input_border, 0.92f);
    if (hovered)
    {
        bg = bloom_apply_state_layer(s->input_bg, s->input_cursor, 0.07f);
        border = bloom_color_mix(s->input_border, s->input_cursor, 0.40f);
    }
    if (held)
    {
        bg = bloom_apply_state_layer(s->input_bg, s->input_cursor, 0.13f);
        border = s->input_cursor;
    }
    bloom_draw_rect_rounded(&ctx->draw_list, rect, bg, size * 0.28f);
    bloom_draw_rect_rounded_border(&ctx->draw_list, rect, border, size * 0.28f, 1.0f);

    icon = hovered || held ? s->input_cursor : s->text_default;
    cx = rect.x + rect.w * 0.5f;
    cy = rect.y + rect.h * 0.5f;
    r = size * 0.18f;

    switch (direction)
    {
    case BLOOM_DIRECTION_LEFT:
        a = bloom_v2(cx - r, cy);
        b = bloom_v2(cx + r * 0.85f, cy - r);
        c = bloom_v2(cx + r * 0.85f, cy + r);
        break;
    case BLOOM_DIRECTION_RIGHT:
        a = bloom_v2(cx + r, cy);
        b = bloom_v2(cx - r * 0.85f, cy - r);
        c = bloom_v2(cx - r * 0.85f, cy + r);
        break;
    case BLOOM_DIRECTION_UP:
        a = bloom_v2(cx, cy - r);
        b = bloom_v2(cx - r, cy + r * 0.85f);
        c = bloom_v2(cx + r, cy + r * 0.85f);
        break;
    default:
        a = bloom_v2(cx, cy + r);
        b = bloom_v2(cx - r, cy - r * 0.85f);
        c = bloom_v2(cx + r, cy - r * 0.85f);
        break;
    }

    bloom_draw_triangle(&ctx->draw_list, a, b, c, icon);
    bloom_advance_layout(size, size);
    return pressed;
}

bloom_bool bloom_choice_strip(const char *label, const char *const *items, bloom_i32 item_count, bloom_i32 *selected_index)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_vec2 pos;
    bloom_u32 visible_len;
    bloom_bool show_label;
    bloom_f32 label_h;
    bloom_f32 label_gap;
    bloom_f32 control_h;
    bloom_f32 total_w;
    bloom_f32 item_w;
    bloom_rect container_rect;
    bloom_f32 segment_padding;
    bloom_f32 active_rounding;
    bloom_i32 i;
    bloom_bool changed = BLOOM_FALSE;

    if (!ctx || !ctx->current_window || !items || item_count <= 0 || !selected_index)
    {
        return BLOOM_FALSE;
    }

    s = &ctx->style;
    pos = ctx->current_window->layout.cursor;
    visible_len = bloom_label_visible_length(label);
    show_label = visible_len > 0;
    label_h = show_label ? bloom_scaled_line_height(ctx, s->font_size * 0.9f) : 0.0f;
    label_gap = show_label ? s->label_gap : 0.0f;
    control_h = bloom_scaled_line_height(ctx, s->font_size) + s->control_padding_y * 2.0f;
    total_w = ctx->current_window->layout.available_width;
    item_w = total_w / (bloom_f32)item_count;
    segment_padding = 3.5f;
    active_rounding = s->button_rounding - segment_padding;
    if (item_w < 32.0f)
    {
        item_w = 32.0f;
    }
    if (active_rounding < 0.0f)
    {
        active_rounding = 0.0f;
    }

    container_rect = bloom_make_rect(pos.x,
                                     pos.y + label_h + label_gap,
                                     total_w,
                                     control_h);

    if (show_label)
    {
        bloom_draw_label(ctx,
            bloom_v2(pos.x, pos.y),
            label,
            s->text_disabled,
            s->font_size * 0.9f);
    }

    bloom_draw_rect_custom(&ctx->draw_list,
                           container_rect,
                           s->input_bg,
                           s->input_border,
                           1.0f,
                           bloom_make_corner_radii_all(s->button_rounding));

    bloom_push_id(label);
    for (i = 0; i < item_count; ++i)
    {
        bloom_rect rect = bloom_make_rect(pos.x + item_w * (bloom_f32)i,
                                          pos.y + label_h + label_gap,
                                          (i == item_count - 1) ? (total_w - item_w * (bloom_f32)i) : item_w,
                                          control_h);
        bloom_rect segment_rect = bloom_make_rect(rect.x + segment_padding,
                                                  rect.y + segment_padding,
                                                  rect.w - segment_padding * 2.0f,
                                                  rect.h - segment_padding * 2.0f);
        bloom_id id = bloom_get_id(items[i]);
        bloom_bool hovered = BLOOM_FALSE;
        bloom_bool held = BLOOM_FALSE;
        bloom_bool pressed = bloom_button_hit(ctx, id, rect, &hovered, &held);
        bloom_bool active = (*selected_index == i);
        bloom_color bg = bloom_rgba(0, 0, 0, 0);
        bloom_color border = bloom_rgba(0, 0, 0, 0);
        bloom_color text_col = active ? s->input_cursor : s->text_default;
        bloom_corner_radii radii = bloom_choice_strip_radii(i, item_count, active_rounding);
        bloom_f32 text_w = bloom_label_width(ctx, items[i], s->font_size * 0.92f);

        if (active)
        {
            bg = bloom_apply_state_layer(s->input_bg, s->input_cursor, 0.24f);
            border = bloom_scale_alpha(s->input_cursor, 0.72f);
            text_col = s->input_cursor;
        }
        else if (hovered)
        {
            bg = bloom_apply_state_layer(s->input_bg, s->text_default, 0.05f);
        }
        if (held)
        {
            bg = bloom_apply_state_layer(s->input_bg, s->input_cursor, active ? 0.32f : 0.12f);
            if (!active)
            {
                border = bloom_scale_alpha(s->input_cursor, 0.35f);
            }
        }

        if (bg.a > 0 || border.a > 0)
        {
            bloom_draw_rect_custom(&ctx->draw_list,
                                   segment_rect,
                                   bg,
                                   border,
                                   border.a > 0 ? 1.0f : 0.0f,
                                   radii);
        }

        if (i > 0)
        {
            bloom_draw_line(&ctx->draw_list,
                bloom_v2(rect.x, rect.y + 6.0f),
                bloom_v2(rect.x, rect.y + rect.h - 6.0f),
                bloom_scale_alpha(s->input_border, 0.45f),
                1.0f);
        }

        bloom_draw_label(ctx,
            bloom_v2(rect.x + (rect.w - text_w) * 0.5f,
                     bloom_centered_text_y(ctx, rect.y, rect.h, s->font_size * 0.92f)),
            items[i],
            text_col,
            s->font_size * 0.92f);

        if (pressed && *selected_index != i)
        {
            *selected_index = i;
            changed = BLOOM_TRUE;
        }
    }
    bloom_pop_id();

    bloom_advance_layout(total_w, label_h + label_gap + control_h + s->touch_padding);
    return changed;
}
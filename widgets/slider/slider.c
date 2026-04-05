#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "widgets/internal.h"
static bloom_bool bloom_slider_float_internal(const char *label, bloom_f32 *value,
                                              bloom_f32 min_val, bloom_f32 max_val,
                                              const bloom_slider_args *args,
                                              bloom_bool show_grab,
                                              bloom_bool integer_mode)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_slider_args resolved;
    bloom_id id;
    bloom_vec2 pos;
    bloom_f32 label_h;
    bloom_f32 slider_w;
    bloom_f32 control_h;
    bloom_f32 track_h;
    bloom_f32 thumb_r;
    bloom_f32 slider_y;
    bloom_rect control_rect;
    bloom_rect slider_rect;
    bloom_f32 track_x0;
    bloom_f32 track_w;
    bloom_f32 range;
    bloom_f32 t;
    bloom_f32 display_t;
    bloom_f32 display_pos_t;
    bloom_f32 hover_t;
    bloom_f32 active_t;
    bloom_bool hovered;
    bloom_bool changed = BLOOM_FALSE;
    char val_text[64];
    bloom_f32 val_tw;
    bloom_rect value_chip_rect;
    bloom_color value_chip_bg;
    bloom_color value_chip_border;
    bloom_color track_bg;
    bloom_color track_border;
    bloom_color fill_color;

    if (!ctx || !ctx->current_window)
    {
        return BLOOM_FALSE;
    }

    resolved = bloom_resolve_slider_args(args, show_grab);
    s = &ctx->style;
    id = bloom_get_id(label);
    pos = ctx->current_window->layout.cursor;
    label_h = bloom_scaled_line_height(ctx, s->font_size * 0.9f);
    slider_w = ctx->current_window->layout.available_width;
    control_h = bloom_scaled_line_height(ctx, s->font_size) + s->field_padding_y * 1.2f;
    track_h = resolved.show_grab ? 6.0f : 8.0f;
    thumb_r = bloom_scaled_line_height(ctx, s->font_size) * 0.43f;
    slider_y = pos.y + label_h + s->label_gap;
    control_rect = bloom_make_rect(pos.x, slider_y, slider_w, control_h);
    slider_rect = bloom_make_rect(pos.x, slider_y + (control_h - track_h) * 0.5f, slider_w, track_h);
    track_x0 = slider_rect.x + (resolved.show_grab ? thumb_r : 0.0f);
    track_w = slider_rect.w - (resolved.show_grab ? thumb_r * 2.0f : 0.0f);
    hovered = bloom_widget_hovered(control_rect);
    range = max_val - min_val;
    if (range <= 0.0f)
    {
        range = 1.0f;
    }

    t = (*value - min_val) / range;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    if (track_w < 1.0f)
    {
        track_x0 = slider_rect.x;
        track_w = slider_rect.w > 1.0f ? slider_rect.w : 1.0f;
    }

    if (hovered)
    {
        ctx->hot_id = id;
        if (ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT])
        {
            ctx->active_id = id;
        }
    }

    if (ctx->active_id == id)
    {
        if (ctx->input.mouse_down[BLOOM_MOUSE_LEFT])
        {
            bloom_f32 new_t = (ctx->input.mouse_pos.x - track_x0) / track_w;
            bloom_f32 new_value;
            if (new_t < 0.0f) new_t = 0.0f;
            if (new_t > 1.0f) new_t = 1.0f;
            new_value = min_val + new_t * range;
            if (integer_mode)
            {
                new_value = floorf(new_value + 0.5f);
            }
            if (fabsf(new_value - *value) > 0.0005f)
            {
                *value = new_value;
                changed = BLOOM_TRUE;
            }
            t = new_t;
        }
        else
        {
            ctx->active_id = 0;
        }
    }

    hover_t = bloom_window_anim_toggle(ctx, id ^ 0x4D11u, hovered || ctx->active_id == id, 18.0f);
    active_t = bloom_window_anim_pulse(ctx, id ^ 0x4D13u, ctx->active_id == id, 48.0f, 22.0f);
    display_t = bloom_window_anim_spring(ctx, id ^ 0x4D12u, t,
                                         180.0f + resolved.animation_response * 2.5f,
                                         20.0f);
    display_pos_t = display_t < 0.0f ? 0.0f : (display_t > 1.0f ? 1.0f : display_t);

    if (integer_mode)
    {
        snprintf(val_text, sizeof(val_text), "%d", (int)floorf((min_val + display_pos_t * range) + 0.5f));
    }
    else
    {
        snprintf(val_text, sizeof(val_text), "%.2f", min_val + display_pos_t * range);
    }
    val_tw = bloom_text_width(val_text, s->font_size * 0.82f);
    value_chip_rect = bloom_make_rect(pos.x + slider_w - (val_tw + 18.0f),
                                      pos.y - 2.0f,
                                      val_tw + 18.0f,
                                      label_h + 8.0f);
    value_chip_bg = bloom_apply_state_layer(s->input_bg, s->input_cursor,
                                            hovered || ctx->active_id == id ? 0.10f : 0.04f);
    value_chip_border = hovered || ctx->active_id == id
        ? bloom_color_mix(s->input_border, s->input_cursor, 0.48f)
        : bloom_scale_alpha(s->input_border, 0.82f);
    track_bg = bloom_apply_state_layer(s->input_bg, s->text_default, 0.10f);
    track_border = hovered || ctx->active_id == id
        ? bloom_color_mix(s->input_border, s->input_cursor, 0.38f)
        : bloom_scale_alpha(s->input_border, 0.72f);
    fill_color = bloom_color_mix(s->slider_grab, s->slider_grab_active, active_t * 0.65f);

    bloom_draw_label(ctx, bloom_v2(pos.x, pos.y), label,
                     hovered || ctx->active_id == id ? s->input_cursor : s->text_disabled,
                     s->font_size * 0.9f);
    bloom_draw_rect_rounded(&ctx->draw_list, value_chip_rect, value_chip_bg, value_chip_rect.h * 0.5f);
    bloom_draw_rect_rounded_border(&ctx->draw_list,
                                   value_chip_rect,
                                   value_chip_border,
                                   value_chip_rect.h * 0.5f,
                                   1.0f);

    if (hover_t > 0.001f || active_t > 0.001f)
    {
        bloom_f32 glow_alpha = 0.04f + hover_t * 0.04f + active_t * 0.06f;
        bloom_draw_rect_rounded(&ctx->draw_list,
            bloom_make_rect(slider_rect.x - 1.0f, slider_rect.y - 2.0f, slider_rect.w + 2.0f, slider_rect.h + 4.0f),
            bloom_scale_alpha(s->input_cursor, glow_alpha),
            s->slider_rounding + 2.0f);
    }

    bloom_draw_rect_rounded(&ctx->draw_list,
                            slider_rect,
                            track_bg,
                            s->slider_rounding + (resolved.show_grab ? 0.0f : 2.0f));
    bloom_draw_rect_rounded_border(&ctx->draw_list,
                                   slider_rect,
                                   track_border,
                                   s->slider_rounding + (resolved.show_grab ? 0.0f : 2.0f),
                                   1.0f);

    {
        bloom_f32 thumb_center_x = track_x0 + display_pos_t * track_w;
        bloom_f32 fill_w = resolved.show_grab ? (thumb_center_x - slider_rect.x) : (slider_rect.w * display_pos_t);
        if (fill_w > 0.0f)
        {
            bloom_draw_rect_rounded(&ctx->draw_list,
                bloom_make_rect(slider_rect.x, slider_rect.y, fill_w, slider_rect.h),
                fill_color,
                s->slider_rounding + (resolved.show_grab ? 0.0f : 2.0f));
        }

        if (resolved.show_grab)
        {
            bloom_f32 thumb_visual_r = thumb_r + hover_t * 0.35f + active_t * 0.75f;
            if (hover_t > 0.001f || active_t > 0.001f)
            {
                bloom_draw_circle_filled(&ctx->draw_list,
                    bloom_v2(thumb_center_x, control_rect.y + control_rect.h * 0.5f),
                    thumb_visual_r + s->touch_padding * 0.55f,
                    bloom_scale_alpha(s->input_cursor, 0.08f + hover_t * 0.06f + active_t * 0.08f),
                    s->circle_segments + 12);
            }

            bloom_draw_circle_filled(&ctx->draw_list,
                bloom_v2(thumb_center_x, control_rect.y + control_rect.h * 0.5f),
                thumb_visual_r + 1.5f,
                s->window_bg,
                s->circle_segments + 8);
            bloom_draw_soft_circle(ctx,
                bloom_v2(thumb_center_x, control_rect.y + control_rect.h * 0.5f),
                thumb_visual_r,
                fill_color);
        }
    }

    bloom_draw_text(&ctx->draw_list,
                    bloom_v2(value_chip_rect.x + (value_chip_rect.w - val_tw) * 0.5f,
                             bloom_centered_text_y(ctx, value_chip_rect.y, value_chip_rect.h, s->font_size * 0.82f)),
                    val_text,
                    hovered || ctx->active_id == id ? s->text_default : s->text_disabled,
                    s->font_size * 0.82f,
                    ctx->default_font.texture_id);

    bloom_advance_layout(slider_w, label_h + s->label_gap + control_h + s->touch_padding);
    return changed;
}

bloom_bool bloom_slider_float(const char *label, bloom_f32 *value, bloom_f32 min_val, bloom_f32 max_val)
{
    return bloom_slider_float_ex(label, value, min_val, max_val, NULL);
}

bloom_bool bloom_slider_int(const char *label, bloom_i32 *value, bloom_i32 min_val, bloom_i32 max_val)
{
    return bloom_slider_int_ex(label, value, min_val, max_val, NULL);
}

bloom_bool bloom_slider_float_ex(const char *label, bloom_f32 *value, bloom_f32 min_val, bloom_f32 max_val,
                                 const bloom_slider_args *args)
{
    return bloom_slider_float_internal(label, value, min_val, max_val, args, BLOOM_TRUE, BLOOM_FALSE);
}

bloom_bool bloom_slider_int_ex(const char *label, bloom_i32 *value, bloom_i32 min_val, bloom_i32 max_val,
                               const bloom_slider_args *args)
{
    bloom_f32 fval = (bloom_f32)*value;
    bloom_bool changed = bloom_slider_float_internal(label, &fval,
                                                     (bloom_f32)min_val, (bloom_f32)max_val,
                                                     args, BLOOM_TRUE, BLOOM_TRUE);
    bloom_i32 rounded = (bloom_i32)floorf(fval + 0.5f);
    if (rounded < min_val) rounded = min_val;
    if (rounded > max_val) rounded = max_val;
    if (*value != rounded)
    {
        *value = rounded;
        changed = BLOOM_TRUE;
    }
    return changed;
}

bloom_bool bloom_slider_float_bar(const char *label, bloom_f32 *value, bloom_f32 min_val, bloom_f32 max_val,
                                  const bloom_slider_args *args)
{
    return bloom_slider_float_internal(label, value, min_val, max_val, args, BLOOM_FALSE, BLOOM_FALSE);
}

bloom_bool bloom_slider_int_bar(const char *label, bloom_i32 *value, bloom_i32 min_val, bloom_i32 max_val,
                                const bloom_slider_args *args)
{
    bloom_f32 fval = (bloom_f32)*value;
    bloom_bool changed = bloom_slider_float_internal(label, &fval,
                                                     (bloom_f32)min_val, (bloom_f32)max_val,
                                                     args, BLOOM_FALSE, BLOOM_TRUE);
    bloom_i32 rounded = (bloom_i32)floorf(fval + 0.5f);
    if (rounded < min_val) rounded = min_val;
    if (rounded > max_val) rounded = max_val;
    if (*value != rounded)
    {
        *value = rounded;
        changed = BLOOM_TRUE;
    }
    return changed;
}

static bloom_bool bloom_slider_vertical_internal(const char *label, bloom_f32 *value,
                                                 bloom_f32 min_val, bloom_f32 max_val,
                                                 bloom_f32 height, bloom_bool integer_mode)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_vec2 pos;
    bloom_u32 visible_len;
    bloom_bool show_label;
    bloom_f32 label_h;
    bloom_f32 label_gap;
    bloom_f32 line_h;
    bloom_f32 track_w;
    bloom_f32 thumb_r;
    bloom_rect slider_rect;
    bloom_rect hit_rect;
    bloom_bool hovered;
    bloom_bool changed = BLOOM_FALSE;
    bloom_id id;
    bloom_f32 range;
    bloom_f32 t;
    bloom_f32 display_t;
    bloom_f32 hover_t;
    bloom_f32 active_t;
    bloom_f32 total_w;
    bloom_f32 value_line_h;
    char value_text[64];
    bloom_f32 value_w;
    bloom_f32 label_w;
    bloom_f32 track_x;
    bloom_f32 thumb_center_y;
    bloom_color fill_color;
    bloom_rect value_chip_rect;
    bloom_color chip_bg;
    bloom_color chip_border;
    bloom_color track_bg;
    bloom_color track_border;

    if (!ctx || !ctx->current_window)
    {
        return BLOOM_FALSE;
    }

    if (height <= 0.0f)
    {
        height = 140.0f;
    }

    s = &ctx->style;
    pos = ctx->current_window->layout.cursor;
    visible_len = bloom_label_visible_length(label);
    show_label = visible_len > 0;
    label_h = show_label ? bloom_scaled_line_height(ctx, s->font_size * 0.9f) : 0.0f;
    label_gap = show_label ? s->label_gap : 0.0f;
    line_h = bloom_scaled_line_height(ctx, s->font_size);
    track_w = line_h * 0.82f;
    if (track_w < 18.0f)
    {
        track_w = 18.0f;
    }
    thumb_r = line_h * 0.40f;
    value_line_h = bloom_scaled_line_height(ctx, s->font_size * 0.85f);
    id = bloom_get_id(label);
    range = max_val - min_val;
    if (range <= 0.0f)
    {
        range = 1.0f;
    }

    t = (*value - min_val) / range;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    if (integer_mode)
    {
        snprintf(value_text, sizeof(value_text), "%d", (int)floorf(*value + 0.5f));
    }
    else
    {
        snprintf(value_text, sizeof(value_text), "%.2f", *value);
    }
    value_w = bloom_text_width(value_text, s->font_size * 0.85f);
    label_w = bloom_label_width(ctx, label, s->font_size * 0.9f);
    total_w = track_w;
    if (value_w > total_w) total_w = value_w;
    if (label_w > total_w) total_w = label_w;
    slider_rect = bloom_make_rect(pos.x + (total_w - track_w) * 0.5f,
                                  pos.y + label_h + label_gap,
                                  track_w,
                                  height);
    hit_rect = bloom_make_rect(slider_rect.x - s->touch_padding,
                               slider_rect.y,
                               slider_rect.w + s->touch_padding * 2.0f,
                               slider_rect.h);
    hovered = bloom_widget_hovered(hit_rect);

    if (hovered)
    {
        ctx->hot_id = id;
        if (ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT])
        {
            ctx->active_id = id;
        }
    }

    if (ctx->active_id == id)
    {
        if (ctx->input.mouse_down[BLOOM_MOUSE_LEFT])
        {
            bloom_f32 new_t = 1.0f - ((ctx->input.mouse_pos.y - slider_rect.y) / slider_rect.h);
            bloom_f32 new_value;
            if (new_t < 0.0f) new_t = 0.0f;
            if (new_t > 1.0f) new_t = 1.0f;
            new_value = min_val + new_t * range;
            if (integer_mode)
            {
                new_value = floorf(new_value + 0.5f);
            }
            if (fabsf(new_value - *value) > 0.0005f)
            {
                *value = new_value;
                changed = BLOOM_TRUE;
            }
            t = new_t;
        }
        else
        {
            ctx->active_id = 0;
        }
    }

    hover_t = bloom_window_anim_toggle(ctx, id ^ 0x6A21u, hovered || ctx->active_id == id, 18.0f);
    active_t = bloom_window_anim_pulse(ctx, id ^ 0x6A22u, ctx->active_id == id, 48.0f, 22.0f);
    display_t = bloom_window_anim_spring(ctx, id ^ 0x6A23u, t, 180.0f, 20.0f);
    if (display_t < 0.0f) display_t = 0.0f;
    if (display_t > 1.0f) display_t = 1.0f;

    if (show_label)
    {
        bloom_draw_label(ctx,
            bloom_v2(pos.x, pos.y),
            label,
            hovered || ctx->active_id == id ? s->input_cursor : s->text_disabled,
            s->font_size * 0.9f);
    }

    chip_bg = bloom_apply_state_layer(s->input_bg, s->input_cursor,
                                      hovered || ctx->active_id == id ? 0.10f : 0.04f);
    chip_border = hovered || ctx->active_id == id
        ? bloom_color_mix(s->input_border, s->input_cursor, 0.48f)
        : bloom_scale_alpha(s->input_border, 0.82f);
    track_bg = bloom_apply_state_layer(s->input_bg, s->text_default, 0.10f);
    track_border = hovered || ctx->active_id == id
        ? bloom_color_mix(s->input_border, s->input_cursor, 0.38f)
        : bloom_scale_alpha(s->input_border, 0.72f);

    bloom_draw_rect_rounded(&ctx->draw_list, slider_rect, track_bg, s->slider_rounding + 3.0f);
    bloom_draw_rect_rounded_border(&ctx->draw_list,
                                   slider_rect,
                                   track_border,
                                   s->slider_rounding + 3.0f,
                                   1.0f);
    fill_color = bloom_color_mix(s->slider_grab, s->slider_grab_active, active_t);
    thumb_center_y = slider_rect.y + slider_rect.h - display_t * slider_rect.h;
    if (display_t > 0.0f)
    {
        bloom_draw_rect_rounded(&ctx->draw_list,
            bloom_make_rect(slider_rect.x,
                            slider_rect.y + slider_rect.h - slider_rect.h * display_t,
                            slider_rect.w,
                            slider_rect.h * display_t),
            fill_color,
            s->slider_rounding + 3.0f);
    }

    track_x = slider_rect.x + slider_rect.w * 0.5f;
    if (hover_t > 0.001f || active_t > 0.001f)
    {
        bloom_draw_circle_filled(&ctx->draw_list,
            bloom_v2(track_x, thumb_center_y),
            thumb_r + s->touch_padding * 0.55f,
            bloom_scale_alpha(s->input_cursor, 0.08f + hover_t * 0.06f + active_t * 0.08f),
            s->circle_segments + 12);
    }
    bloom_draw_circle_filled(&ctx->draw_list,
        bloom_v2(track_x, thumb_center_y),
        thumb_r + 1.5f,
        s->window_bg,
        s->circle_segments + 8);
    bloom_draw_soft_circle(ctx, bloom_v2(track_x, thumb_center_y), thumb_r + hover_t * 0.4f, fill_color);

    if (integer_mode)
    {
        snprintf(value_text, sizeof(value_text), "%d", (int)floorf((min_val + display_t * range) + 0.5f));
    }
    else
    {
        snprintf(value_text, sizeof(value_text), "%.2f", min_val + display_t * range);
    }
    value_w = bloom_text_width(value_text, s->font_size * 0.85f);
    value_chip_rect = bloom_make_rect(pos.x + (total_w - (value_w + 18.0f)) * 0.5f,
                                      slider_rect.y + slider_rect.h + s->label_gap - 2.0f,
                                      value_w + 18.0f,
                                      value_line_h + 8.0f);
    bloom_draw_rect_rounded(&ctx->draw_list, value_chip_rect, chip_bg, value_chip_rect.h * 0.5f);
    bloom_draw_rect_rounded_border(&ctx->draw_list, value_chip_rect, chip_border, value_chip_rect.h * 0.5f, 1.0f);
    bloom_draw_text(&ctx->draw_list,
        bloom_v2(value_chip_rect.x + (value_chip_rect.w - value_w) * 0.5f,
                 bloom_centered_text_y(ctx, value_chip_rect.y, value_chip_rect.h, s->font_size * 0.85f)),
        value_text,
        hovered || ctx->active_id == id ? s->text_default : s->text_disabled,
        s->font_size * 0.85f,
        ctx->default_font.texture_id);

    bloom_advance_layout(total_w,
        label_h + label_gap + height + s->label_gap + value_chip_rect.h + s->touch_padding);
    return changed;
}

bloom_bool bloom_slider_float_tall(const char *label, bloom_f32 *value, bloom_f32 min_val, bloom_f32 max_val,
                                   bloom_f32 height)
{
    return bloom_slider_vertical_internal(label, value, min_val, max_val, height, BLOOM_FALSE);
}

bloom_bool bloom_slider_int_tall(const char *label, bloom_i32 *value, bloom_i32 min_val, bloom_i32 max_val,
                                 bloom_f32 height)
{
    bloom_f32 fval = (bloom_f32)*value;
    bloom_bool changed = bloom_slider_vertical_internal(label, &fval,
                                                        (bloom_f32)min_val, (bloom_f32)max_val,
                                                        height, BLOOM_TRUE);
    bloom_i32 rounded = (bloom_i32)floorf(fval + 0.5f);
    if (rounded < min_val) rounded = min_val;
    if (rounded > max_val) rounded = max_val;
    if (*value != rounded)
    {
        *value = rounded;
        changed = BLOOM_TRUE;
    }
    return changed;
}

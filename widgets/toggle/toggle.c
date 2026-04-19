#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "widgets/internal.h"
bloom_bool bloom_checkbox(const char *label, bloom_bool *value)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_id id;
    bloom_vec2 pos;
    bloom_f32 box_size;
    bloom_f32 total_h;
    bloom_f32 text_w;
    bloom_f32 total_w;
    bloom_rect hit_rect;
    bloom_rect box_rect;
    bloom_bool hovered;
    bloom_bool changed = BLOOM_FALSE;
    bloom_f32 hover_t;
    bloom_f32 checked_t;
    bloom_f32 press_t;
    bloom_f32 check_draw_t;
    bloom_color box_bg;
    bloom_color box_border;

    if (!ctx || !ctx->current_window)
    {
        return BLOOM_FALSE;
    }

    s = &ctx->style;
    id = bloom_get_id(label);
    pos = ctx->current_window->layout.cursor;
    box_size = bloom_scaled_line_height(ctx, s->font_size) * 0.86f;
    total_h = box_size + s->touch_padding * 1.6f;
    text_w = bloom_label_width(ctx, label, s->font_size);
    total_w = box_size + s->item_inner_spacing + text_w;
    hit_rect = bloom_make_rect(pos.x, pos.y, total_w, total_h);
    box_rect = bloom_make_rect(pos.x, pos.y + (total_h - box_size) * 0.5f, box_size, box_size);
    hovered = bloom_widget_hovered(hit_rect);

    if (hovered)
    {
        ctx->hot_id = id;
        if (ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT])
        {
            ctx->active_id = id;
        }
    }

    if (bloom_widget_released_inside(ctx, id, hovered))
    {
        *value = !(*value);
        changed = BLOOM_TRUE;
    }

    hover_t = bloom_window_anim_toggle(ctx, id ^ 0x1A21u, hovered || ctx->active_id == id, 18.0f);
    checked_t = bloom_window_anim_toggle(ctx, id ^ 0x1A22u, *value, 14.0f);
    press_t = bloom_window_anim_pulse(ctx, id ^ 0x1A23u, ctx->active_id == id, 40.0f, 18.0f);
    check_draw_t = bloom_anim_ease_apply(checked_t, BLOOM_ANIM_EASE_OUT_BACK);

    if (hover_t > 0.001f || press_t > 0.001f)
    {
        bloom_draw_rect_rounded(&ctx->draw_list,
            bloom_make_rect(pos.x - s->touch_padding,
                            pos.y,
                            total_w + s->touch_padding * 2.0f,
                            total_h),
            bloom_apply_state_layer(s->window_bg, s->input_cursor, 0.04f + hover_t * 0.05f + press_t * 0.04f),
            s->checkbox_rounding + 12.0f);
    }

    box_bg = checked_t > 0.0f ? bloom_color_mix(s->input_bg, s->input_cursor, checked_t) : s->input_bg;
    box_border = checked_t > 0.0f ? bloom_color_mix(s->input_border, s->input_cursor, checked_t)
                                  : bloom_color_mix(s->input_border, s->input_cursor, hover_t * 0.32f);

    bloom_draw_rect_rounded(&ctx->draw_list, box_rect, box_bg, s->checkbox_rounding);
    bloom_draw_rect_rounded_border(&ctx->draw_list,
                                   box_rect,
                                   box_border,
                                   s->checkbox_rounding,
                                   checked_t > 0.0f || hovered ? 1.6f : 1.2f);

    if (check_draw_t > 0.001f)
    {
        bloom_f32 mark_scale = 0.94f + (check_draw_t > 1.0f ? 0.04f : check_draw_t * 0.04f) - press_t * 0.02f;
        bloom_f32 stroke = box_rect.w * 0.14f + checked_t * 0.08f;
        bloom_vec2 center = bloom_v2(box_rect.x + box_rect.w * 0.5f, box_rect.y + box_rect.h * 0.5f);
        
        bloom_vec2 p0 = bloom_v2(center.x - box_rect.w * 0.28f * mark_scale,
                                 center.y + box_rect.h * 0.04f * mark_scale);
        bloom_vec2 p1 = bloom_v2(center.x - box_rect.w * 0.06f * mark_scale,
                                 center.y + box_rect.h * 0.28f * mark_scale);
        bloom_vec2 p2 = bloom_v2(center.x + box_rect.w * 0.34f * mark_scale,
                                 center.y - box_rect.h * 0.24f * mark_scale);
                                 
        bloom_f32 first_t = checked_t < 0.55f ? (checked_t / 0.55f) : 1.0f;
        bloom_f32 second_t = checked_t <= 0.35f ? 0.0f : ((checked_t - 0.35f) / 0.65f);
        
        bloom_color mark_color = bloom_scale_alpha(s->window_bg, checked_t > 1.0f ? 1.0f : checked_t);
        if (checked_t > 0.9f)
        {
            mark_color = bloom_scale_alpha(s->window_bg, checked_t);
        }

        bloom_vec2 mid = bloom_v2(bloom_lerp_f32(p0.x, p1.x, first_t), bloom_lerp_f32(p0.y, p1.y, first_t));
        bloom_vec2 end = bloom_v2(bloom_lerp_f32(p1.x, p2.x, second_t), bloom_lerp_f32(p1.y, p2.y, second_t));

        bloom_draw_line(&ctx->draw_list, p0, mid, mark_color, stroke);
        if (second_t > 0.0f)
        {
            bloom_draw_line(&ctx->draw_list, p1, end, mark_color, stroke);
        }
    }

    bloom_draw_label(ctx,
        bloom_v2(pos.x + box_size + s->item_inner_spacing,
                 bloom_centered_text_y(ctx, pos.y, total_h, s->font_size)),
        label,
        s->text_default,
        s->font_size);

    bloom_advance_layout(total_w, total_h);
    return changed;
}

bloom_bool bloom_toggle_ex(const char *label, bloom_bool *value, const bloom_toggle_args *args)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_toggle_args resolved;
    bloom_id id;
    bloom_vec2 pos;
    bloom_f32 track_h;
    bloom_f32 track_w;
    bloom_f32 thumb_r;
    bloom_f32 total_h;
    bloom_f32 text_w;
    bloom_f32 total_w;
    bloom_rect hit_rect;
    bloom_rect track_rect;
    bloom_bool hovered;
    bloom_bool changed = BLOOM_FALSE;
    bloom_f32 hover_t;
    bloom_f32 toggle_t;
    bloom_f32 press_t;
    bloom_f32 toggle_pos_t;
    bloom_color track_bg;
    bloom_color track_border;
    bloom_vec2 thumb_center;

    if (!ctx || !ctx->current_window)
    {
        return BLOOM_FALSE;
    }

    s = &ctx->style;
    resolved = bloom_resolve_toggle_args(args);
    id = bloom_get_id(label);
    pos = ctx->current_window->layout.cursor;
    track_h = bloom_scaled_line_height(ctx, s->font_size) * 0.98f;
    track_w = track_h * 1.75f;
    thumb_r = track_h * 0.5f - 4.0f;
    total_h = track_h + s->touch_padding * 2.0f;
    text_w = bloom_label_width(ctx, label, s->font_size);
    total_w = track_w + s->item_inner_spacing + text_w;
    hit_rect = bloom_make_rect(pos.x, pos.y, total_w, total_h);
    track_rect = bloom_make_rect(pos.x, pos.y + (total_h - track_h) * 0.5f, track_w, track_h);
    hovered = bloom_widget_hovered(hit_rect);

    if (hovered)
    {
        ctx->hot_id = id;
        if (ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT])
        {
            ctx->active_id = id;
        }
    }

    if (bloom_widget_released_inside(ctx, id, hovered))
    {
        *value = !(*value);
        changed = BLOOM_TRUE;
    }

    hover_t = bloom_window_anim_toggle(ctx, id ^ 0x2B11u, hovered || ctx->active_id == id, 18.0f);
    toggle_t = bloom_window_anim_spring(ctx, id ^ 0x2B12u, *value ? 1.0f : 0.0f,
                                        170.0f + resolved.animation_response * 2.5f,
                                        20.0f);
    press_t = bloom_window_anim_pulse(ctx, id ^ 0x2B13u, ctx->active_id == id, 44.0f, 20.0f);
    toggle_pos_t = toggle_t < 0.0f ? 0.0f : (toggle_t > 1.0f ? 1.0f : toggle_t);

    if (hover_t > 0.001f || press_t > 0.001f)
    {
        bloom_draw_rect_rounded(&ctx->draw_list,
            bloom_make_rect(pos.x - s->touch_padding,
                            pos.y,
                            total_w + s->touch_padding * 2.0f,
                            total_h),
            bloom_apply_state_layer(s->window_bg, s->input_cursor, 0.03f + hover_t * 0.05f + press_t * 0.04f),
            s->checkbox_rounding + 12.0f);
    }

    track_bg = bloom_color_mix(s->input_bg, s->input_cursor, toggle_pos_t * 0.62f);
    track_border = bloom_color_mix(s->checkbox_border, s->checkbox_mark,
                                   toggle_t > hover_t ? toggle_t * 0.75f : hover_t * 0.32f);
    bloom_draw_rect_rounded(&ctx->draw_list, track_rect, track_bg, track_rect.h * 0.5f);
    bloom_draw_rect_rounded_border(&ctx->draw_list,
                                   track_rect,
                                   track_border,
                                   track_rect.h * 0.5f,
                                   hovered || toggle_pos_t > 0.0f ? 1.5f : 1.1f);

    thumb_center = bloom_v2(track_rect.x + 4.0f + thumb_r + toggle_pos_t * (track_rect.w - 8.0f - thumb_r * 2.0f),
                            track_rect.y + track_rect.h * 0.5f);
    if (hover_t > 0.001f || press_t > 0.001f)
    {
        bloom_draw_circle_filled(&ctx->draw_list,
            thumb_center,
            thumb_r + 3.0f + hover_t * 0.6f + press_t * 0.8f,
            bloom_scale_alpha(s->input_cursor, 0.08f + hover_t * 0.06f + press_t * 0.10f),
            s->circle_segments + 12);
    }
    bloom_draw_circle_filled(&ctx->draw_list,
        thumb_center,
        thumb_r + 1.5f,
        s->window_bg,
        s->circle_segments + 10);
    bloom_draw_soft_circle(ctx,
        thumb_center,
        thumb_r + press_t * 0.7f,
        bloom_color_mix(s->text_disabled, s->button_text, toggle_pos_t));

    bloom_draw_label(ctx,
        bloom_v2(pos.x + track_w + s->item_inner_spacing,
                 bloom_centered_text_y(ctx, pos.y, total_h, s->font_size)),
        label,
        s->text_default,
        s->font_size);

    bloom_advance_layout(total_w, total_h);
    return changed;
}

bloom_bool bloom_toggle(const char *label, bloom_bool *value)
{
    return bloom_toggle_ex(label, value, NULL);
}

bloom_bool bloom_radio_button(const char *label, bloom_bool active)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_id id;
    bloom_vec2 pos;
    bloom_f32 radius;
    bloom_f32 text_w;
    bloom_f32 total_w;
    bloom_f32 total_h;
    bloom_rect hit_rect;
    bloom_bool hovered;
    bloom_bool clicked = BLOOM_FALSE;
    bloom_f32 hover_t;
    bloom_f32 active_t;
    bloom_f32 press_t;
    bloom_color ring_color;
    bloom_vec2 center;

    if (!ctx || !ctx->current_window)
    {
        return BLOOM_FALSE;
    }

    s = &ctx->style;
    id = bloom_get_id(label);
    pos = ctx->current_window->layout.cursor;
    radius = s->font_size * 0.48f + 2.5f;
    text_w = bloom_label_width(ctx, label, s->font_size);
    total_w = radius * 2.0f + s->item_inner_spacing + text_w;
    total_h = radius * 2.0f + s->touch_padding * 2.0f;
    hit_rect = bloom_make_rect(pos.x, pos.y, total_w, total_h);
    hovered = bloom_widget_hovered(hit_rect);

    if (hovered)
    {
        ctx->hot_id = id;
        if (ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT])
        {
            ctx->active_id = id;
        }
    }

    if (bloom_widget_released_inside(ctx, id, hovered))
    {
        clicked = BLOOM_TRUE;
    }

    hover_t = bloom_window_anim_toggle(ctx, id ^ 0x3C11u, hovered || ctx->active_id == id, 18.0f);
    active_t = bloom_window_anim_spring(ctx, id ^ 0x3C12u, active ? 1.0f : 0.0f, 180.0f, 22.0f);
    press_t = bloom_window_anim_pulse(ctx, id ^ 0x3C13u, ctx->active_id == id, 40.0f, 18.0f);
    center = bloom_v2(pos.x + radius, pos.y + total_h * 0.5f);
    {
        bloom_draw_rect_rounded(&ctx->draw_list,
            bloom_make_rect(pos.x - s->touch_padding,
                            pos.y,
                            total_w + s->touch_padding * 2.0f,
                            total_h),
                bloom_apply_state_layer(s->window_bg, s->input_cursor, 0.03f + hover_t * 0.05f + press_t * 0.04f),
            s->checkbox_rounding + 12.0f);
    }

    ring_color = bloom_color_mix(s->checkbox_border, s->checkbox_mark,
                         active_t > hover_t ? active_t : hover_t * 0.28f);
    bloom_draw_soft_circle(ctx, center, radius, ring_color);
            bloom_draw_circle_filled(&ctx->draw_list, center, radius - 1.8f, s->input_bg, s->circle_segments + 12);
    if (active_t > 0.001f)
    {
        bloom_draw_soft_circle(ctx, center, radius * (0.14f + active_t * 0.28f), s->checkbox_mark);
    }

    bloom_draw_label(ctx,
        bloom_v2(pos.x + radius * 2.0f + s->item_inner_spacing,
                 bloom_centered_text_y(ctx, pos.y, total_h, s->font_size)),
        label,
        s->text_default,
        s->font_size);

    bloom_advance_layout(total_w, total_h);
    return clicked;
}
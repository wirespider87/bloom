#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "widgets/internal.h"

bloom_bool bloom_hyperlink(const char *label)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_id id;
    bloom_vec2 pos;
    bloom_f32 text_w;
    bloom_f32 h;
    bloom_rect rect;
    bloom_bool hovered;
    bloom_bool held;
    bloom_bool pressed;
    bloom_f32 hover_t;
    bloom_color text_color;

    if (!ctx || !ctx->current_window)
    {
        return BLOOM_FALSE;
    }

    s = &ctx->style;
    id = bloom_get_id(label);
    pos = ctx->current_window->layout.cursor;
    text_w = bloom_label_width(ctx, label, s->font_size);
    h = bloom_scaled_line_height(ctx, s->font_size);
    rect = bloom_make_rect(pos.x, pos.y, text_w, h);

    hovered = bloom_widget_hovered(rect);
    held = (ctx->active_id == id);
    pressed = BLOOM_FALSE;

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

    hover_t = bloom_window_anim_toggle(ctx, id ^ 0x7A01u, hovered || held, 16.0f);
    text_color = bloom_color_mix(s->input_cursor, bloom_scale_alpha(s->input_cursor, 0.7f), hover_t);

    bloom_draw_label(ctx, pos, label, text_color, s->font_size);

    /* underline */
    {
        bloom_f32 underline_y = pos.y + h - 1.0f;
        bloom_color underline_col = bloom_scale_alpha(text_color, 0.5f + hover_t * 0.5f);
        bloom_draw_line(&ctx->draw_list,
                        bloom_v2(pos.x, underline_y),
                        bloom_v2(pos.x + text_w, underline_y),
                        underline_col, 1.0f);
    }

    bloom_advance_layout(text_w, h);
    return pressed;
}

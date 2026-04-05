#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "widgets/internal.h"

bloom_bool bloom_splitter(bloom_bool vertical, bloom_f32 thickness, bloom_f32 *size1, bloom_f32 *size2,
                          bloom_f32 min_size1, bloom_f32 min_size2)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_id id;
    bloom_vec2 pos;
    bloom_rect bar_rect;
    bloom_bool hovered;
    bloom_bool held;
    bloom_bool changed = BLOOM_FALSE;
    bloom_f32 hover_t;
    bloom_color bar_color;

    if (!ctx || !ctx->current_window || !size1 || !size2)
    {
        return BLOOM_FALSE;
    }

    s = &ctx->style;
    id = bloom_get_id("##splitter");
    pos = ctx->current_window->layout.cursor;

    if (thickness <= 0.0f)
    {
        thickness = 4.0f;
    }
    if (min_size1 <= 0.0f)
    {
        min_size1 = 20.0f;
    }
    if (min_size2 <= 0.0f)
    {
        min_size2 = 20.0f;
    }

    if (vertical)
    {
        bar_rect = bloom_make_rect(pos.x + *size1, pos.y, thickness,
                                   ctx->current_window->layout.available_width);
    }
    else
    {
        bar_rect = bloom_make_rect(pos.x, pos.y + *size1, 
                                   ctx->current_window->layout.available_width, thickness);
    }

    hovered = bloom_widget_hovered(bar_rect);

    if (hovered && ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT])
    {
        ctx->active_id = id;
    }

    held = (ctx->active_id == id);

    if (held)
    {
        if (ctx->input.mouse_down[BLOOM_MOUSE_LEFT])
        {
            bloom_f32 delta = vertical ? ctx->input.mouse_delta.x : ctx->input.mouse_delta.y;
            if (delta != 0.0f)
            {
                bloom_f32 new_size1 = *size1 + delta;
                bloom_f32 new_size2 = *size2 - delta;

                if (new_size1 < min_size1)
                {
                    new_size2 -= (min_size1 - new_size1);
                    new_size1 = min_size1;
                }
                if (new_size2 < min_size2)
                {
                    new_size1 -= (min_size2 - new_size2);
                    new_size2 = min_size2;
                }

                if (new_size1 >= min_size1 && new_size2 >= min_size2)
                {
                    *size1 = new_size1;
                    *size2 = new_size2;
                    changed = BLOOM_TRUE;
                }
            }
        }
        else
        {
            ctx->active_id = 0;
        }
    }

    hover_t = bloom_window_anim_toggle(ctx, id ^ 0x7C01u, hovered || held, 12.0f);
    bar_color = bloom_color_mix(s->separator, s->input_cursor, hover_t);
    bloom_draw_rect_filled(&ctx->draw_list, bar_rect, bar_color);

    if (!vertical)
    {
        bloom_advance_layout(ctx->current_window->layout.available_width, *size1 + thickness + *size2);
    }

    return changed;
}

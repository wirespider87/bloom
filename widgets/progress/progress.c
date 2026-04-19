#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "widgets/internal.h"
void bloom_progress_bar(bloom_f32 fraction, bloom_f32 w, bloom_f32 h)
{
    bloom_context *ctx = bloom_get_context();
    if (!ctx || !ctx->current_window)
    {
        return;
    }

    bloom_style *s = &ctx->style;
    bloom_vec2 pos = ctx->current_window->layout.cursor;
    if (w <= 0) w = ctx->current_window->layout.available_width;
    if (h <= 0) h = bloom_scaled_line_height(ctx, s->font_size) * 0.6f + s->field_padding_y * 0.4f;
    if (fraction < 0) fraction = 0;
    if (fraction > 1) fraction = 1;

    bloom_draw_rect_rounded(&ctx->draw_list, bloom_make_rect(pos.x, pos.y, w, h),
                           s->slider_bg, s->frame_rounding);
    if (fraction > 0)
    {
        bloom_draw_rect_rounded(&ctx->draw_list,
            bloom_make_rect(pos.x, pos.y, w * fraction, h),
            s->slider_grab, s->frame_rounding);

        if (bloom_window_animations_enabled(ctx) && w * fraction > h * 0.8f)
        {
            bloom_f32 shimmer = bloom_anim_ping_pong(0.7f, 0.18f);
            bloom_f32 sheen_w = w * 0.18f;
            bloom_f32 sheen_x = pos.x + (w * fraction - sheen_w) * shimmer;
            if (sheen_x < pos.x)
            {
                sheen_x = pos.x;
            }
            if (sheen_x + sheen_w > pos.x + w * fraction)
            {
                sheen_w = pos.x + w * fraction - sheen_x;
            }
            if (sheen_w > 1.0f)
            {
                bloom_draw_rect_rounded(&ctx->draw_list,
                    bloom_make_rect(sheen_x, pos.y, sheen_w, h),
                    bloom_scale_alpha(s->text_default, 0.12f),
                    s->frame_rounding);
            }
        }
    }

    char pct[16];
    snprintf(pct, sizeof(pct), "%d%%", (int)(fraction * 100));
    bloom_f32 tw = bloom_text_width(pct, s->font_size * 0.85f);
    bloom_draw_text(&ctx->draw_list,
        bloom_v2(pos.x + (w - tw) * 0.5f, bloom_centered_text_y(ctx, pos.y, h, s->font_size * 0.85f)),
        pct, s->text_default, s->font_size * 0.85f, ctx->default_font.texture_id);

    bloom_advance_layout(w, h);
}
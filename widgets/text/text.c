#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "widgets/internal.h"
void bloom_text(const char *text)
{
    bloom_context *ctx = bloom_get_context();
    if (!ctx || !ctx->current_window)
    {
        return;
    }
    bloom_vec2 pos = ctx->current_window->layout.cursor;
    bloom_f32 h = bloom_scaled_line_height(ctx, ctx->style.font_size);
    bloom_draw_text(&ctx->draw_list, pos, text, ctx->style.text_default,
                   ctx->style.font_size, ctx->default_font.texture_id);
    bloom_f32 w = bloom_text_width(text, ctx->style.font_size);
    bloom_advance_layout(w, h);
}

void bloom_text_wrapped(const char *text)
{
    bloom_context *ctx = bloom_get_context();
    bloom_f32 w;
    bloom_f32 h;
    bloom_vec2 pos;
    bloom_f32 max_width;

    if (!ctx || !ctx->current_window)
    {
        return;
    }

    pos = ctx->current_window->layout.cursor;
    max_width = ctx->current_window->layout.available_width;
    if (max_width <= 0.0f)
    {
        max_width = 1.0f;
    }

    bloom_draw_wrapped_text_lines(ctx, pos, text ? text : "", ctx->style.text_default,
                                  ctx->style.font_size, max_width, &w, &h);
    bloom_advance_layout(max_width, h);
}

void bloom_text_colored(const char *text, bloom_color col)
{
    bloom_context *ctx = bloom_get_context();
    if (!ctx || !ctx->current_window)
    {
        return;
    }
    bloom_vec2 pos = ctx->current_window->layout.cursor;
    bloom_f32 h = bloom_scaled_line_height(ctx, ctx->style.font_size);
    bloom_draw_text(&ctx->draw_list, pos, text, col,
                   ctx->style.font_size, ctx->default_font.texture_id);
    bloom_f32 w = bloom_text_width(text, ctx->style.font_size);
    bloom_advance_layout(w, h);
}

void bloom_text_disabled(const char *text)
{
    bloom_text_colored(text, bloom_get_context()->style.text_disabled);
}

void bloom_list_bullet(const char *text)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_vec2 pos;
    bloom_f32 bullet_r;
    bloom_f32 total_h;
    bloom_f32 text_x;
    bloom_f32 text_w;

    if (!ctx || !ctx->current_window)
    {
        return;
    }

    s = &ctx->style;
    pos = ctx->current_window->layout.cursor;
    bullet_r = bloom_scaled_line_height(ctx, s->font_size) * 0.14f + 1.0f;
    total_h = bloom_scaled_line_height(ctx, s->font_size) + s->touch_padding;
    text_x = pos.x + bullet_r * 2.0f + s->item_inner_spacing + 2.0f;
    text_w = bloom_label_width(ctx, text ? text : "", s->font_size);

    bloom_draw_soft_circle(ctx,
        bloom_v2(pos.x + bullet_r, pos.y + total_h * 0.5f),
        bullet_r,
        s->text_default);

    if (text && text[0] != '\0')
    {
        bloom_draw_label(ctx,
            bloom_v2(text_x, bloom_centered_text_y(ctx, pos.y, total_h, s->font_size)),
            text,
            s->text_default,
            s->font_size);
    }

    bloom_advance_layout((text_x - pos.x) + text_w, total_h);
}
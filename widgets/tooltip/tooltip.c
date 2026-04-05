#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "widgets/internal.h"
void bloom_tooltip(const char *text)
{
    bloom_context *ctx = bloom_get_context();
    if (!ctx)
    {
        return;
    }
    bloom_style *s = &ctx->style;
    bloom_f32 text_w = bloom_text_width(text, s->font_size);
    bloom_f32 pad = 6.0f;
    bloom_f32 x = ctx->input.mouse_pos.x + 12;
    bloom_f32 y = ctx->input.mouse_pos.y + 12;

    bloom_draw_rect_rounded(&ctx->draw_list,
        bloom_make_rect(x - pad, y - pad, text_w + pad * 2, s->font_size + pad * 2),
        s->tooltip_bg, 4.0f);
    bloom_draw_rect_rounded_border(&ctx->draw_list,
        bloom_make_rect(x - pad, y - pad, text_w + pad * 2, s->font_size + pad * 2),
        s->tooltip_border, 4.0f, 1.0f);
    bloom_draw_text(&ctx->draw_list, bloom_v2(x, y), text,
                   s->tooltip_text, s->font_size, ctx->default_font.texture_id);
}

void bloom_set_tooltip(const char *text)
{
    bloom_tooltip(text);
}

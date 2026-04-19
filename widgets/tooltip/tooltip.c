#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "widgets/internal.h"

/* deferred tooltip - stored each frame, flushed at end-of-frame into
    foreground_draw_list so it always renders on top of all other widgets */
static char   g_tooltip_text[512];
static bloom_bool g_tooltip_pending = BLOOM_FALSE;

static void bloom_tooltip_store(const char *text)
{
    if (!text || !text[0])
    {
        return;
    }
    strncpy(g_tooltip_text, text, sizeof(g_tooltip_text) - 1);
    g_tooltip_text[sizeof(g_tooltip_text) - 1] = '\0';
    g_tooltip_pending = BLOOM_TRUE;
}

/* Called by bloom_widgets_begin_frame() to reset state each frame */
void bloom_tooltip_begin_frame(void)
{
    g_tooltip_pending = BLOOM_FALSE;
    g_tooltip_text[0] = '\0';
}

/* Called by bloom_widgets_end_frame() to draw the deferred tooltip on top */
void bloom_tooltip_flush_deferred(void)
{
    bloom_context *ctx;
    bloom_style *s;
    bloom_draw_list *dl;
    bloom_f32 text_w;
    bloom_f32 pad;
    bloom_f32 x;
    bloom_f32 y;
    bloom_f32 tw;
    bloom_f32 th;

    if (!g_tooltip_pending)
    {
        return;
    }

    ctx = bloom_get_context();
    if (!ctx)
    {
        return;
    }

    s  = &ctx->style;
    dl = &ctx->foreground_draw_list;

    text_w = bloom_text_width(g_tooltip_text, s->font_size);
    pad    = 6.0f;
    tw     = text_w + pad * 2.0f;
    th     = s->font_size + pad * 2.0f;
    x      = ctx->input.mouse_pos.x + 14.0f;
    y      = ctx->input.mouse_pos.y + 14.0f;

    /* keep tooltip inside the display area */
    if (x + tw > ctx->display_size.x - 4.0f)
    {
        x = ctx->display_size.x - tw - 4.0f;
    }
    if (y + th > ctx->display_size.y - 4.0f)
    {
        y = ctx->display_size.y - th - 4.0f;
    }

    bloom_draw_rect_rounded(dl,
        bloom_make_rect(x - pad, y - pad, tw, th),
        s->tooltip_bg, s->tooltip_rounding);
    bloom_draw_rect_rounded_border(dl,
        bloom_make_rect(x - pad, y - pad, tw, th),
        s->tooltip_border, s->tooltip_rounding, 1.0f);
    bloom_draw_text(dl, bloom_v2(x, y), g_tooltip_text,
                    s->tooltip_text, s->font_size, ctx->default_font.texture_id);

    g_tooltip_pending = BLOOM_FALSE;
}

void bloom_tooltip(const char *text)
{
    bloom_context *ctx = bloom_get_context();
    bloom_rect last_rect;

    if (!ctx || !ctx->current_window)
    {
        return;
    }

    last_rect = bloom_make_rect(
        ctx->current_window->layout.last_item_pos.x,
        ctx->current_window->layout.last_item_pos.y,
        ctx->current_window->layout.last_item_size.x,
        ctx->current_window->layout.last_item_size.y);

    if (bloom_widget_hovered(last_rect))
    {
        bloom_tooltip_store(text);
    }
}

void bloom_set_tooltip(const char *text)
{
    bloom_tooltip_store(text);
}

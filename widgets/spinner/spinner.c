#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "widgets/internal.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void bloom_spinner(const char *label, bloom_f32 radius)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_vec2 pos;
    bloom_f32 diameter;
    bloom_vec2 center;
    bloom_f32 angle;
    bloom_f32 arc_len;
    bloom_i32 segments;
    bloom_i32 i;

    if (!ctx || !ctx->current_window)
    {
        return;
    }

    s = &ctx->style;
    if (radius <= 0.0f)
    {
        radius = bloom_scaled_line_height(ctx, s->font_size) * 0.5f;
    }

    pos = ctx->current_window->layout.cursor;
    diameter = radius * 2.0f;
    center = bloom_v2(pos.x + radius, pos.y + radius);

    angle = bloom_anim_loop(3.0f, 0.0f) * (bloom_f32)(2.0 * M_PI);
    arc_len = (bloom_f32)(M_PI * 0.75);
    segments = 24;

    for (i = 0; i < segments; ++i)
    {
        bloom_f32 t0 = angle + arc_len * ((bloom_f32)i / (bloom_f32)segments);
        bloom_f32 t1 = angle + arc_len * ((bloom_f32)(i + 1) / (bloom_f32)segments);
        bloom_f32 alpha = (bloom_f32)(i + 1) / (bloom_f32)segments;
        bloom_color col = bloom_scale_alpha(s->input_cursor, alpha);
        bloom_vec2 a = bloom_v2(center.x + cosf(t0) * radius, center.y + sinf(t0) * radius);
        bloom_vec2 b = bloom_v2(center.x + cosf(t1) * radius, center.y + sinf(t1) * radius);
        bloom_draw_line(&ctx->draw_list, a, b, col, 2.0f);
    }

    bloom_advance_layout(diameter, diameter);
}

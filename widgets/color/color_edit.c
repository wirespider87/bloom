#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "widgets/internal.h"

#define BLOOM_HEX_STATE_CAPACITY 32

typedef struct bloom_hex_state
{
    bloom_id id;
    bloom_bool active;
    char buffer[24];
} bloom_hex_state;

static bloom_hex_state g_hex_states[BLOOM_HEX_STATE_CAPACITY];

static void bloom_color_to_hex(char *buf, bloom_u32 buf_size, const bloom_f32 col[4], bloom_bool with_alpha);

static bloom_hex_state *bloom_find_hex_state(bloom_id id)
{
    bloom_i32 i;
    bloom_hex_state *free_state = NULL;

    for (i = 0; i < BLOOM_HEX_STATE_CAPACITY; ++i)
    {
        if (g_hex_states[i].active && g_hex_states[i].id == id)
        {
            return &g_hex_states[i];
        }
        if (!free_state && !g_hex_states[i].active)
        {
            free_state = &g_hex_states[i];
        }
    }

    if (!free_state)
    {
        free_state = &g_hex_states[0];
    }

    free_state->active = BLOOM_TRUE;
    free_state->id = id;
    free_state->buffer[0] = '\0';
    return free_state;
}

#define BLOOM_COLOR_PICK_STATE_CAPACITY 32

static struct
{
    bloom_id id;
    bloom_bool open;
} g_color_pick_state[BLOOM_COLOR_PICK_STATE_CAPACITY];
static int g_color_pick_count = 0;

static bloom_bool *bloom_color_pick_find_state(bloom_id id)
{
    int i;
    for (i = 0; i < g_color_pick_count; i++)
    {
        if (g_color_pick_state[i].id == id)
        {
            return &g_color_pick_state[i].open;
        }
    }
    if (g_color_pick_count < BLOOM_COLOR_PICK_STATE_CAPACITY)
    {
        g_color_pick_state[g_color_pick_count].id = id;
        g_color_pick_state[g_color_pick_count].open = BLOOM_FALSE;
        return &g_color_pick_state[g_color_pick_count++].open;
    }
    return NULL;
}

static bloom_f32 bloom_color_clamp01(bloom_f32 value)
{
    if (value < 0.0f)
    {
        return 0.0f;
    }
    if (value > 1.0f)
    {
        return 1.0f;
    }
    return value;
}

static void bloom_color_copy4(bloom_f32 out[4], const bloom_f32 in[4])
{
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
    out[3] = in[3];
}

static void bloom_color_sanitize(bloom_f32 col[4], bloom_bool with_alpha)
{
    col[0] = bloom_color_clamp01(col[0]);
    col[1] = bloom_color_clamp01(col[1]);
    col[2] = bloom_color_clamp01(col[2]);
    col[3] = with_alpha ? bloom_color_clamp01(col[3]) : 1.0f;
}

static bloom_color bloom_color_from_f32(const bloom_f32 col[4])
{
    return bloom_rgba((bloom_u8)(bloom_color_clamp01(col[0]) * 255.0f + 0.5f),
                      (bloom_u8)(bloom_color_clamp01(col[1]) * 255.0f + 0.5f),
                      (bloom_u8)(bloom_color_clamp01(col[2]) * 255.0f + 0.5f),
                      (bloom_u8)(bloom_color_clamp01(col[3]) * 255.0f + 0.5f));
}

static bloom_f32 bloom_color_luma3(const bloom_f32 col[3])
{
    return col[0] * 0.2126f + col[1] * 0.7152f + col[2] * 0.0722f;
}

static void bloom_color_draw_checkerboard(bloom_context *ctx, bloom_rect rect, bloom_f32 cell_size, bloom_f32 rounding)
{
    bloom_style *s;
    bloom_color light;
    bloom_color dark;
    bloom_i32 row;
    bloom_i32 column;
    bloom_i32 rows;
    bloom_i32 columns;

    if (!ctx)
    {
        return;
    }

    s = &ctx->style;
    if (cell_size < 4.0f)
    {
        cell_size = 4.0f;
    }

    light = bloom_apply_state_layer(s->window_bg, s->text_default, 0.05f);
    dark = bloom_apply_state_layer(s->window_bg, s->text_default, 0.14f);

    bloom_draw_rect_rounded(&ctx->draw_list, rect, light, rounding);

    bloom_draw_push_clip(&ctx->draw_list, rect);
    rows = (bloom_i32)(rect.h / cell_size) + 2;
    columns = (bloom_i32)(rect.w / cell_size) + 2;
    for (row = 0; row < rows; ++row)
    {
        for (column = 0; column < columns; ++column)
        {
            if (((row + column) & 1) == 0)
            {
                continue;
            }

            bloom_draw_rect_filled(&ctx->draw_list,
                                   bloom_make_rect(rect.x + column * cell_size,
                                                   rect.y + row * cell_size,
                                                   cell_size,
                                                   cell_size),
                                   dark);
        }
    }
    bloom_draw_pop_clip(&ctx->draw_list);
}

static void bloom_color_marker_colors(const bloom_f32 rgb[3], bloom_color *outer, bloom_color *inner)
{
    if (bloom_color_luma3(rgb) > 0.55f)
    {
        *outer = bloom_rgba(24, 28, 36, 230);
        *inner = bloom_rgba(255, 255, 255, 235);
    }
    else
    {
        *outer = bloom_rgba(255, 255, 255, 235);
        *inner = bloom_rgba(24, 28, 36, 230);
    }
}

static void bloom_color_draw_picker_marker(bloom_context *ctx, bloom_vec2 center, const bloom_f32 rgb[3])
{
    bloom_color outer;
    bloom_color inner;
    bloom_color preview;

    bloom_color_marker_colors(rgb, &outer, &inner);
    preview = bloom_rgba((bloom_u8)(bloom_color_clamp01(rgb[0]) * 255.0f + 0.5f),
                         (bloom_u8)(bloom_color_clamp01(rgb[1]) * 255.0f + 0.5f),
                         (bloom_u8)(bloom_color_clamp01(rgb[2]) * 255.0f + 0.5f),
                         255);
    bloom_draw_circle_filled(&ctx->draw_list, center, 7.0f, outer, ctx->style.circle_segments + 10);
    bloom_draw_circle_filled(&ctx->draw_list, center, 5.0f, inner, ctx->style.circle_segments + 10);
    bloom_draw_circle_filled(&ctx->draw_list, center, 3.0f, preview, ctx->style.circle_segments + 10);
}

static void bloom_color_draw_bar_marker(bloom_context *ctx, bloom_rect rect, bloom_f32 y, const bloom_f32 rgb[3])
{
    bloom_color outer;
    bloom_color inner;

    bloom_color_marker_colors(rgb, &outer, &inner);
    bloom_draw_rect_filled(&ctx->draw_list,
                           bloom_make_rect(rect.x - 2.0f, y - 2.5f, rect.w + 4.0f, 5.0f),
                           outer);
    bloom_draw_rect_filled(&ctx->draw_list,
                           bloom_make_rect(rect.x - 1.0f, y - 1.0f, rect.w + 2.0f, 2.0f),
                           inner);
}

static void bloom_color_draw_corner_masks(bloom_context *ctx, bloom_rect rect, bloom_f32 r)
{
    /*
     * Fill the crescent between each square corner and its rounding arc
     * using a triangle fan.  bloom_draw_rect_custom uses max_r uniformly
     * (SDF limitation), so per-corner radii don't work — the old masks
     * rendered as circles instead of one-corner-rounded rects.
     */
    bloom_color card_fill;
    const bloom_f32 pi = 3.14159265f;
    const bloom_f32 half_pi = 1.57079633f;
    const int segs = 6;
    bloom_f32 a_step;
    bloom_f32 corner_x[4], corner_y[4], center_x[4], center_y[4], angle0[4];
    int ci, k;

    if (!ctx)
    {
        return;
    }

    card_fill = bloom_apply_state_layer(ctx->style.input_bg, ctx->style.input_cursor, 0.04f);
    a_step = half_pi / (bloom_f32)segs;

    /* top-left */
    corner_x[0] = rect.x;              corner_y[0] = rect.y;
    center_x[0] = rect.x + r;          center_y[0] = rect.y + r;
    angle0[0]   = pi;

    /* top-right */
    corner_x[1] = rect.x + rect.w;     corner_y[1] = rect.y;
    center_x[1] = rect.x + rect.w - r; center_y[1] = rect.y + r;
    angle0[1]   = pi + half_pi;

    /* bottom-right */
    corner_x[2] = rect.x + rect.w;     corner_y[2] = rect.y + rect.h;
    center_x[2] = rect.x + rect.w - r; center_y[2] = rect.y + rect.h - r;
    angle0[2]   = 0.0f;

    /* bottom-left */
    corner_x[3] = rect.x;              corner_y[3] = rect.y + rect.h;
    center_x[3] = rect.x + r;          center_y[3] = rect.y + rect.h - r;
    angle0[3]   = half_pi;

    for (ci = 0; ci < 4; ci++)
    {
        bloom_vec2 cp = bloom_v2(corner_x[ci], corner_y[ci]);
        bloom_f32 a0 = angle0[ci];
        bloom_vec2 prev_pt = bloom_v2(center_x[ci] + r * cosf(a0),
                                      center_y[ci] + r * sinf(a0));

        for (k = 1; k <= segs; k++)
        {
            bloom_f32 a = a0 + a_step * (bloom_f32)k;
            bloom_vec2 cur_pt = bloom_v2(center_x[ci] + r * cosf(a),
                                         center_y[ci] + r * sinf(a));
            bloom_draw_triangle(&ctx->draw_list, cp, prev_pt, cur_pt, card_fill);
            prev_pt = cur_pt;
        }
    }
}

static void bloom_color_commit_manual_block(bloom_context *ctx, bloom_vec2 origin, bloom_f32 width, bloom_f32 height)
{
    bloom_window *win;
    bloom_f32 item_bottom;

    if (!ctx || !ctx->current_window)
    {
        return;
    }

    win = ctx->current_window;
    item_bottom = origin.y + height + win->scroll_y - win->content_rect.y;
    if (item_bottom > win->content_extent_y)
    {
        win->content_extent_y = item_bottom;
    }

    win->layout.last_item_pos = origin;
    win->layout.last_item_size = bloom_v2(width, height);
    win->layout.cursor.x = win->layout.start.x + win->layout.indent;
    win->layout.cursor.y = origin.y + height + ctx->style.item_spacing;
    win->layout.max_row_height = 0.0f;
    win->layout.type = BLOOM_LAYOUT_VERTICAL;
}

static void bloom_color_draw_card(bloom_context *ctx, bloom_rect rect, bloom_f32 rounding)
{
    bloom_color fill;
    bloom_color border;

    if (!ctx)
    {
        return;
    }

    fill = bloom_apply_state_layer(ctx->style.input_bg, ctx->style.input_cursor, 0.04f);
    border = bloom_scale_alpha(ctx->style.input_border, 0.90f);
    bloom_draw_rect_rounded(&ctx->draw_list, rect, fill, rounding);
    bloom_draw_rect_rounded_border(&ctx->draw_list, rect, border, rounding, 1.0f);
}

static void bloom_color_draw_preview_banner(const char *label, const bloom_f32 col[4], bloom_bool with_alpha)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_vec2 pos;
    bloom_rect rect;
    bloom_rect swatch_rect;
    bloom_color preview;
    bloom_f32 width;
    bloom_f32 height;
    char hex_text[24];
    char meta_text[64];
    bloom_f32 label_y;
    bloom_f32 hex_y;
    bloom_f32 meta_y;

    if (!ctx || !ctx->current_window)
    {
        return;
    }

    s = &ctx->style;
    pos = ctx->current_window->layout.cursor;
    width = ctx->current_window->layout.available_width;
    height = 78.0f;
    rect = bloom_make_rect(pos.x, pos.y, width, height);
    swatch_rect = bloom_make_rect(rect.x + rect.w - 68.0f, rect.y + 11.0f, 52.0f, height - 22.0f);
    preview = bloom_color_from_f32(col);

    bloom_color_to_hex(hex_text, (bloom_u32)sizeof(hex_text), col, with_alpha);
    snprintf(meta_text,
             sizeof(meta_text),
             with_alpha ? "RGBA  %d%% opacity" : "RGB  Material surface editor",
             (int)(bloom_color_clamp01(col[3]) * 100.0f + 0.5f));

    bloom_color_draw_card(ctx, rect, s->input_rounding + 4.0f);

    {
        bloom_f32 line_label = bloom_scaled_line_height(ctx, s->font_size * 0.82f);
        bloom_f32 line_hex = bloom_scaled_line_height(ctx, s->font_size * 0.96f);
        bloom_f32 line_meta = bloom_scaled_line_height(ctx, s->font_size * 0.78f);
        bloom_f32 text_block = line_label + 6.0f + line_hex + 4.0f + line_meta;
        bloom_f32 top_pad = (height - text_block) * 0.5f;
        label_y = rect.y + top_pad;
        hex_y = label_y + line_label + 6.0f;
        meta_y = hex_y + line_hex + 4.0f;
    }

    bloom_draw_label(ctx,
                     bloom_v2(rect.x + 16.0f, label_y),
                     label,
                     s->text_disabled,
                     s->font_size * 0.82f);
    bloom_draw_text(&ctx->draw_list,
                    bloom_v2(rect.x + 16.0f, hex_y),
                    hex_text,
                    s->text_default,
                    s->font_size * 0.96f,
                    ctx->default_font.texture_id);
    bloom_draw_text(&ctx->draw_list,
                    bloom_v2(rect.x + 16.0f, meta_y),
                    meta_text,
                    s->text_disabled,
                    s->font_size * 0.78f,
                    ctx->default_font.texture_id);

    if (preview.a < 255)
    {
        bloom_color_draw_checkerboard(ctx, swatch_rect, 6.0f, s->color_preview_rounding + 2.0f);
    }
    bloom_draw_rect_rounded(&ctx->draw_list, swatch_rect, preview, s->color_preview_rounding + 2.0f);
    bloom_draw_rect_rounded_border(&ctx->draw_list,
                                   swatch_rect,
                                   bloom_scale_alpha(s->input_border, 0.95f),
                                   s->color_preview_rounding + 2.0f,
                                   1.0f);

    bloom_color_commit_manual_block(ctx, pos, width, height);
}

static void bloom_rgb_to_hsv(const bloom_f32 rgb[3], bloom_f32 *out_h,
                             bloom_f32 *out_s, bloom_f32 *out_v)
{
    bloom_f32 r = bloom_color_clamp01(rgb[0]);
    bloom_f32 g = bloom_color_clamp01(rgb[1]);
    bloom_f32 b = bloom_color_clamp01(rgb[2]);
    bloom_f32 max_c = r;
    bloom_f32 min_c = r;
    bloom_f32 delta;
    bloom_f32 h = 0.0f;
    bloom_f32 s;
    bloom_f32 v;

    if (g > max_c) max_c = g;
    if (b > max_c) max_c = b;
    if (g < min_c) min_c = g;
    if (b < min_c) min_c = b;

    delta = max_c - min_c;
    v = max_c;
    s = max_c <= 0.00001f ? 0.0f : (delta / max_c);

    if (delta > 0.00001f)
    {
        if (max_c == r)
        {
            h = (g - b) / delta;
            if (g < b)
            {
                h += 6.0f;
            }
        }
        else if (max_c == g)
        {
            h = 2.0f + (b - r) / delta;
        }
        else
        {
            h = 4.0f + (r - g) / delta;
        }
        h /= 6.0f;
    }

    *out_h = h;
    *out_s = s;
    *out_v = v;
}

static void bloom_hsv_to_rgb(bloom_f32 h, bloom_f32 s, bloom_f32 v, bloom_f32 rgb[3])
{
    bloom_f32 c;
    bloom_f32 x;
    bloom_f32 m;
    bloom_f32 hh;

    h -= floorf(h);
    s = bloom_color_clamp01(s);
    v = bloom_color_clamp01(v);

    c = v * s;
    hh = h * 6.0f;
    x = c * (1.0f - fabsf(fmodf(hh, 2.0f) - 1.0f));
    m = v - c;

    if (hh < 1.0f)
    {
        rgb[0] = c + m; rgb[1] = x + m; rgb[2] = m;
    }
    else if (hh < 2.0f)
    {
        rgb[0] = x + m; rgb[1] = c + m; rgb[2] = m;
    }
    else if (hh < 3.0f)
    {
        rgb[0] = m; rgb[1] = c + m; rgb[2] = x + m;
    }
    else if (hh < 4.0f)
    {
        rgb[0] = m; rgb[1] = x + m; rgb[2] = c + m;
    }
    else if (hh < 5.0f)
    {
        rgb[0] = x + m; rgb[1] = m; rgb[2] = c + m;
    }
    else
    {
        rgb[0] = c + m; rgb[1] = m; rgb[2] = x + m;
    }
}

static bloom_f32 bloom_color_linearize(bloom_f32 value)
{
    value = bloom_color_clamp01(value);
    if (value <= 0.04045f)
    {
        return value / 12.92f;
    }
    return powf((value + 0.055f) / 1.055f, 2.4f);
}

static bloom_f32 bloom_color_encode(bloom_f32 value)
{
    if (value <= 0.0031308f)
    {
        value *= 12.92f;
    }
    else
    {
        value = 1.055f * powf(value, 1.0f / 2.4f) - 0.055f;
    }
    return bloom_color_clamp01(value);
}

static bloom_f32 bloom_color_xyz_pivot(bloom_f32 value)
{
    if (value > 0.008856f)
    {
        return powf(value, 1.0f / 3.0f);
    }
    return (7.787f * value) + (16.0f / 116.0f);
}

static bloom_f32 bloom_color_xyz_unpivot(bloom_f32 value)
{
    bloom_f32 cubed = value * value * value;
    if (cubed > 0.008856f)
    {
        return cubed;
    }
    return (value - 16.0f / 116.0f) / 7.787f;
}

static void bloom_rgb_to_lab(const bloom_f32 rgb[3], bloom_f32 *out_l,
                             bloom_f32 *out_a, bloom_f32 *out_b)
{
    bloom_f32 r = bloom_color_linearize(rgb[0]);
    bloom_f32 g = bloom_color_linearize(rgb[1]);
    bloom_f32 b = bloom_color_linearize(rgb[2]);
    bloom_f32 x = (r * 0.4124564f + g * 0.3575761f + b * 0.1804375f) / 0.95047f;
    bloom_f32 y = (r * 0.2126729f + g * 0.7151522f + b * 0.0721750f) / 1.00000f;
    bloom_f32 z = (r * 0.0193339f + g * 0.1191920f + b * 0.9503041f) / 1.08883f;
    bloom_f32 fx = bloom_color_xyz_pivot(x);
    bloom_f32 fy = bloom_color_xyz_pivot(y);
    bloom_f32 fz = bloom_color_xyz_pivot(z);

    *out_l = 116.0f * fy - 16.0f;
    *out_a = 500.0f * (fx - fy);
    *out_b = 200.0f * (fy - fz);
}

static void bloom_lab_to_rgb(bloom_f32 l, bloom_f32 a, bloom_f32 b, bloom_f32 rgb[3])
{
    bloom_f32 fy = (l + 16.0f) / 116.0f;
    bloom_f32 fx = fy + (a / 500.0f);
    bloom_f32 fz = fy - (b / 200.0f);
    bloom_f32 x = 0.95047f * bloom_color_xyz_unpivot(fx);
    bloom_f32 y = 1.00000f * bloom_color_xyz_unpivot(fy);
    bloom_f32 z = 1.08883f * bloom_color_xyz_unpivot(fz);
    bloom_f32 lr = x * 3.2404542f + y * -1.5371385f + z * -0.4985314f;
    bloom_f32 lg = x * -0.9692660f + y * 1.8760108f + z * 0.0415560f;
    bloom_f32 lb = x * 0.0556434f + y * -0.2040259f + z * 1.0572252f;

    if (lr < 0.0f) lr = 0.0f;
    if (lg < 0.0f) lg = 0.0f;
    if (lb < 0.0f) lb = 0.0f;

    rgb[0] = bloom_color_encode(lr);
    rgb[1] = bloom_color_encode(lg);
    rgb[2] = bloom_color_encode(lb);
}

static void bloom_color_to_hex(char *buf, bloom_u32 buf_size, const bloom_f32 col[4], bloom_bool with_alpha)
{
    bloom_u32 r = (bloom_u32)(bloom_color_clamp01(col[0]) * 255.0f + 0.5f);
    bloom_u32 g = (bloom_u32)(bloom_color_clamp01(col[1]) * 255.0f + 0.5f);
    bloom_u32 b = (bloom_u32)(bloom_color_clamp01(col[2]) * 255.0f + 0.5f);
    bloom_u32 a = (bloom_u32)(bloom_color_clamp01(col[3]) * 255.0f + 0.5f);

    if (with_alpha)
    {
        snprintf(buf, buf_size, "#%02X%02X%02X%02X", r, g, b, a);
    }
    else
    {
        snprintf(buf, buf_size, "#%02X%02X%02X", r, g, b);
    }
}

static bloom_bool bloom_parse_hex_color(const char *text, bloom_f32 col[4], bloom_bool with_alpha)
{
    const char *scan = text;
    unsigned int r;
    unsigned int g;
    unsigned int b;
    unsigned int a = 255;

    if (!scan)
    {
        return BLOOM_FALSE;
    }
    if (*scan == '#')
    {
        scan++;
    }

    if (with_alpha)
    {
        if (strlen(scan) != 8 || sscanf(scan, "%02x%02x%02x%02x", &r, &g, &b, &a) != 4)
        {
            return BLOOM_FALSE;
        }
    }
    else
    {
        if (strlen(scan) != 6 || sscanf(scan, "%02x%02x%02x", &r, &g, &b) != 3)
        {
            return BLOOM_FALSE;
        }
    }

    col[0] = (bloom_f32)r / 255.0f;
    col[1] = (bloom_f32)g / 255.0f;
    col[2] = (bloom_f32)b / 255.0f;
    col[3] = (bloom_f32)a / 255.0f;
    return BLOOM_TRUE;
}

static bloom_bool bloom_color_swatch_row(const char *label, const bloom_f32 col[4],
                                         bloom_f32 w, bloom_f32 h, bloom_bool clickable)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_vec2 pos;
    bloom_rect swatch_rect;
    bloom_rect hit_rect;
    bloom_color preview;
    bloom_f32 label_w;
    bloom_f32 total_w;
    bloom_bool hovered;
    bloom_id id;
    bloom_bool pressed = BLOOM_FALSE;

    if (!ctx || !ctx->current_window)
    {
        return BLOOM_FALSE;
    }

    s = &ctx->style;
    pos = ctx->current_window->layout.cursor;
    if (w <= 0.0f) w = bloom_scaled_line_height(ctx, s->font_size) * 1.4f;
    if (h <= 0.0f) h = bloom_scaled_line_height(ctx, s->font_size) + s->touch_padding * 1.6f;

    label_w = bloom_label_width(ctx, label, s->font_size);
    total_w = w + (label_w > 0.0f ? (s->item_inner_spacing + label_w) : 0.0f);
    swatch_rect = bloom_make_rect(pos.x, pos.y + (h - bloom_scaled_line_height(ctx, s->font_size)) * 0.15f, w, h);
    hit_rect = bloom_make_rect(pos.x, pos.y, total_w, h);
    preview = bloom_color_from_f32(col);
    hovered = clickable ? bloom_widget_hovered(hit_rect) : BLOOM_FALSE;
    id = bloom_get_id(label);

    if (clickable && hovered)
    {
        ctx->hot_id = id;
        if (ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT])
        {
            ctx->active_id = id;
        }
    }
    if (clickable && ctx->active_id == id)
    {
        if (ctx->input.mouse_released[BLOOM_MOUSE_LEFT])
        {
            if (hovered)
            {
                pressed = BLOOM_TRUE;
            }
            ctx->active_id = 0;
        }
    }

    if (preview.a < 255)
    {
        bloom_color_draw_checkerboard(ctx, swatch_rect, 6.0f, s->color_preview_rounding);
    }
    bloom_draw_rect_rounded(&ctx->draw_list, swatch_rect, preview, s->color_preview_rounding);
    bloom_draw_rect_rounded_border(&ctx->draw_list,
        swatch_rect,
        hovered ? s->input_cursor : s->checkbox_border,
        s->color_preview_rounding,
        s->color_preview_border_width > 0.0f ? s->color_preview_border_width : 1.0f);
    if (label_w > 0.0f)
    {
        bloom_draw_label(ctx,
            bloom_v2(pos.x + w + s->item_inner_spacing,
                     bloom_centered_text_y(ctx, pos.y, h, s->font_size)),
            label,
            hovered ? s->input_cursor : s->text_default,
            s->font_size);
    }

    bloom_advance_layout(total_w, h);
    return pressed;
}

static bloom_bool bloom_color_apply_rgb_panel(bloom_f32 col[4], bloom_bool with_alpha)
{
    bloom_bool changed = BLOOM_FALSE;

    changed |= bloom_numeric_input("R", &col[0], BLOOM_VALUE_KIND_FLOAT);
    changed |= bloom_numeric_input("G", &col[1], BLOOM_VALUE_KIND_FLOAT);
    changed |= bloom_numeric_input("B", &col[2], BLOOM_VALUE_KIND_FLOAT);
    if (with_alpha)
    {
        changed |= bloom_numeric_input("A", &col[3], BLOOM_VALUE_KIND_FLOAT);
    }

    bloom_color_sanitize(col, with_alpha);
    return changed;
}

static bloom_bool bloom_color_apply_hsv_panel(bloom_f32 col[4], bloom_bool with_alpha)
{
    bloom_bool changed = BLOOM_FALSE;
    bloom_f32 hsv[3];
    bloom_f32 hue_degrees;

    bloom_rgb_to_hsv(col, &hsv[0], &hsv[1], &hsv[2]);
    hue_degrees = hsv[0] * 360.0f;

    changed |= bloom_numeric_input("Hue", &hue_degrees, BLOOM_VALUE_KIND_FLOAT);
    changed |= bloom_numeric_input("Sat", &hsv[1], BLOOM_VALUE_KIND_FLOAT);
    changed |= bloom_numeric_input("Val", &hsv[2], BLOOM_VALUE_KIND_FLOAT);
    if (with_alpha)
    {
        changed |= bloom_numeric_input("Alpha", &col[3], BLOOM_VALUE_KIND_FLOAT);
    }

    if (changed)
    {
        hsv[0] = hue_degrees / 360.0f;
        bloom_hsv_to_rgb(hsv[0], hsv[1], hsv[2], col);
        bloom_color_sanitize(col, with_alpha);
    }
    return changed;
}

static bloom_bool bloom_color_apply_lab_panel(bloom_f32 col[4], bloom_bool with_alpha)
{
    bloom_bool changed = BLOOM_FALSE;
    bloom_f32 l;
    bloom_f32 a;
    bloom_f32 b;

    bloom_rgb_to_lab(col, &l, &a, &b);
    changed |= bloom_numeric_input("Light", &l, BLOOM_VALUE_KIND_FLOAT);
    changed |= bloom_numeric_input("Axis A", &a, BLOOM_VALUE_KIND_FLOAT);
    changed |= bloom_numeric_input("Axis B", &b, BLOOM_VALUE_KIND_FLOAT);
    if (with_alpha)
    {
        changed |= bloom_numeric_input("Alpha", &col[3], BLOOM_VALUE_KIND_FLOAT);
    }

    if (changed)
    {
        bloom_lab_to_rgb(l, a, b, col);
        bloom_color_sanitize(col, with_alpha);
    }
    return changed;
}

static bloom_bool bloom_color_apply_hex_panel(const char *scope_label, bloom_f32 col[4], bloom_bool with_alpha)
{
    bloom_context *ctx = bloom_get_context();
    bloom_hex_state *state;
    bloom_id id;
    bloom_bool focused;
    bloom_bool changed = BLOOM_FALSE;
    bloom_f32 parsed[4];

    if (!ctx)
    {
        return BLOOM_FALSE;
    }

    bloom_push_id(scope_label);
    id = bloom_get_id("Hex");
    state = bloom_find_hex_state(id);
    focused = (ctx->focus_id == id);

    if (!focused || state->buffer[0] == '\0')
    {
        bloom_color_to_hex(state->buffer, (bloom_u32)sizeof(state->buffer), col, with_alpha);
    }

    if (bloom_text_input("Hex", state->buffer, (bloom_u32)sizeof(state->buffer)))
    {
        if (bloom_parse_hex_color(state->buffer, parsed, with_alpha))
        {
            col[0] = parsed[0];
            col[1] = parsed[1];
            col[2] = parsed[2];
            col[3] = with_alpha ? parsed[3] : 1.0f;
            changed = BLOOM_TRUE;
        }
    }

    if (!(ctx->focus_id == id))
    {
        bloom_color_to_hex(state->buffer, (bloom_u32)sizeof(state->buffer), col, with_alpha);
    }

    bloom_pop_id();
    return changed;
}

static void bloom_color_draw_picker_square(bloom_context *ctx, bloom_rect rect, bloom_f32 hue)
{
    const bloom_i32 steps = 28;
    bloom_i32 y;
    bloom_i32 x;

    bloom_draw_push_clip(&ctx->draw_list, rect);
    for (y = 0; y < steps; ++y)
    {
        bloom_f32 v0 = 1.0f - ((bloom_f32)y / (bloom_f32)steps);
        bloom_f32 v1 = 1.0f - ((bloom_f32)(y + 1) / (bloom_f32)steps);
        bloom_f32 y0 = rect.y + rect.h * ((bloom_f32)y / (bloom_f32)steps);
        bloom_f32 y1 = rect.y + rect.h * ((bloom_f32)(y + 1) / (bloom_f32)steps);

        for (x = 0; x < steps; ++x)
        {
            bloom_f32 s0 = (bloom_f32)x / (bloom_f32)steps;
            bloom_f32 s1 = (bloom_f32)(x + 1) / (bloom_f32)steps;
            bloom_f32 x0 = rect.x + rect.w * ((bloom_f32)x / (bloom_f32)steps);
            bloom_f32 x1 = rect.x + rect.w * ((bloom_f32)(x + 1) / (bloom_f32)steps);
            bloom_f32 rgb[3];

            bloom_hsv_to_rgb(hue, (s0 + s1) * 0.5f, (v0 + v1) * 0.5f, rgb);
            bloom_draw_rect_filled(&ctx->draw_list,
                bloom_make_rect(x0, y0, x1 - x0 + 1.0f, y1 - y0 + 1.0f),
                bloom_rgb((bloom_u8)(rgb[0] * 255.0f + 0.5f),
                          (bloom_u8)(rgb[1] * 255.0f + 0.5f),
                          (bloom_u8)(rgb[2] * 255.0f + 0.5f)));
        }
    }
    bloom_color_draw_corner_masks(ctx, rect, 8.0f);
    bloom_draw_pop_clip(&ctx->draw_list);
}

static void bloom_color_draw_hue_bar(bloom_context *ctx, bloom_rect rect)
{
    const bloom_i32 steps = 36;
    bloom_i32 i;

    bloom_draw_push_clip(&ctx->draw_list, rect);
    for (i = 0; i < steps; ++i)
    {
        bloom_f32 h0 = (bloom_f32)i / (bloom_f32)steps;
        bloom_f32 h1 = (bloom_f32)(i + 1) / (bloom_f32)steps;
        bloom_f32 rgb[3];
        bloom_f32 y0 = rect.y + rect.h * h0;
        bloom_f32 y1 = rect.y + rect.h * h1;

        bloom_hsv_to_rgb((h0 + h1) * 0.5f, 1.0f, 1.0f, rgb);
        bloom_draw_rect_filled(&ctx->draw_list,
            bloom_make_rect(rect.x, y0, rect.w, y1 - y0 + 1.0f),
            bloom_rgb((bloom_u8)(rgb[0] * 255.0f + 0.5f),
                      (bloom_u8)(rgb[1] * 255.0f + 0.5f),
                      (bloom_u8)(rgb[2] * 255.0f + 0.5f)));
    }
    bloom_color_draw_corner_masks(ctx, rect, 8.0f);
    bloom_draw_pop_clip(&ctx->draw_list);
}

static void bloom_color_draw_alpha_bar(bloom_context *ctx, bloom_rect rect, const bloom_f32 col[4])
{
    const bloom_i32 steps = 36;
    bloom_i32 i;

    bloom_draw_push_clip(&ctx->draw_list, rect);
    bloom_color_draw_checkerboard(ctx, rect, 6.0f, 8.0f);

    for (i = 0; i < steps; ++i)
    {
        bloom_f32 t0 = (bloom_f32)i / (bloom_f32)steps;
        bloom_f32 t1 = (bloom_f32)(i + 1) / (bloom_f32)steps;
        bloom_f32 y0 = rect.y + rect.h * t0;
        bloom_f32 y1 = rect.y + rect.h * t1;
        bloom_f32 alpha = (t0 + t1) * 0.5f;

        bloom_draw_rect_filled(&ctx->draw_list,
            bloom_make_rect(rect.x, y0, rect.w, y1 - y0 + 1.0f),
            bloom_rgba((bloom_u8)(col[0] * 255.0f + 0.5f),
                       (bloom_u8)(col[1] * 255.0f + 0.5f),
                       (bloom_u8)(col[2] * 255.0f + 0.5f),
                       (bloom_u8)(alpha * 255.0f + 0.5f)));
    }
    bloom_color_draw_corner_masks(ctx, rect, 8.0f);
    bloom_draw_pop_clip(&ctx->draw_list);
}

static bloom_bool bloom_color_edit_internal(const char *label, bloom_f32 col[4],
                                            bloom_bool with_alpha, bloom_u32 flags)
{
    bloom_bool changed = BLOOM_FALSE;

    bloom_color_sanitize(col, with_alpha);
    bloom_color_draw_preview_banner(label, col, with_alpha);

    bloom_push_id(label);
    if ((flags & BLOOM_COLOR_FLAGS_NO_RGB) == 0)
    {
        bloom_text_disabled("RGB Channels");
        changed |= bloom_color_apply_rgb_panel(col, with_alpha);
        bloom_spacing();
    }
    if ((flags & BLOOM_COLOR_FLAGS_NO_HEX) == 0)
    {
        bloom_text_disabled("Hex");
        changed |= bloom_color_apply_hex_panel(label, col, with_alpha);
        bloom_spacing();
    }
    if ((flags & BLOOM_COLOR_FLAGS_NO_HSV) == 0)
    {
        bloom_text_disabled("Hue, Saturation, Value");
        changed |= bloom_color_apply_hsv_panel(col, with_alpha);
        bloom_spacing();
    }
    if ((flags & BLOOM_COLOR_FLAGS_NO_LAB) == 0)
    {
        bloom_text_disabled("Perceptual Lab");
        changed |= bloom_color_apply_lab_panel(col, with_alpha);
    }
    bloom_pop_id();

    bloom_color_sanitize(col, with_alpha);
    return changed;
}

static bloom_bool bloom_color_pick_internal(const char *label, bloom_f32 col[4],
                                            bloom_bool with_alpha, bloom_u32 flags)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_vec2 banner_pos;
    bloom_vec2 pos;
    bloom_f32 available_w;
    bloom_f32 square_size;
    bloom_f32 bar_w;
    bloom_f32 spacing;
    bloom_rect square_rect;
    bloom_rect hue_rect;
    bloom_rect alpha_rect;
    bloom_rect picker_card_rect;
    bloom_rect swatch_hit_rect;
    bloom_f32 hsv[3];
    bloom_id square_id;
    bloom_id hue_id;
    bloom_id alpha_id;
    bloom_id toggle_id;
    bloom_bool changed = BLOOM_FALSE;
    bloom_bool *picker_open;
    bloom_f32 rgb[3];
    bloom_f32 total_h;
    bloom_f32 card_padding;
    bloom_f32 header_h;

    if (!ctx || !ctx->current_window)
    {
        return BLOOM_FALSE;
    }

    s = &ctx->style;
    bloom_color_sanitize(col, with_alpha);

    /* --- preview banner with clickable swatch --- */
    banner_pos = ctx->current_window->layout.cursor;
    available_w = ctx->current_window->layout.available_width;
    bloom_color_draw_preview_banner(label, col, with_alpha);

    bloom_push_id(label);
    toggle_id = bloom_get_id("##pickerToggle");
    picker_open = bloom_color_pick_find_state(toggle_id);

    swatch_hit_rect = bloom_make_rect(banner_pos.x + available_w - 68.0f,
                                      banner_pos.y + 11.0f,
                                      52.0f, 78.0f - 22.0f);

    if (picker_open && bloom_widget_hovered(swatch_hit_rect) &&
        ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT])
    {
        *picker_open = !(*picker_open);
    }

    if (!picker_open || !(*picker_open))
    {
        bloom_pop_id();
        return BLOOM_FALSE;
    }

    /* --- picker card --- */
    pos = ctx->current_window->layout.cursor;
    bar_w = 18.0f;
    spacing = 10.0f;
    card_padding = 16.0f;
    header_h = bloom_scaled_line_height(ctx, s->font_size * 0.82f) + 8.0f;
    square_size = available_w - card_padding * 2.0f - bar_w - spacing;
    if (with_alpha)
    {
        square_size -= bar_w + spacing;
    }
    if (square_size > 220.0f) square_size = 220.0f;
    if (square_size < 120.0f) square_size = 120.0f;

    picker_card_rect = bloom_make_rect(pos.x,
                                       pos.y,
                                       available_w,
                                       card_padding * 2.0f + header_h + square_size);
    square_rect = bloom_make_rect(pos.x + card_padding,
                                  pos.y + card_padding + header_h,
                                  square_size,
                                  square_size);
    hue_rect = bloom_make_rect(square_rect.x + square_rect.w + spacing,
                               square_rect.y,
                               bar_w,
                               square_size);
    alpha_rect = bloom_make_rect(hue_rect.x + hue_rect.w + spacing,
                                 square_rect.y,
                                 bar_w,
                                 square_size);
    total_h = picker_card_rect.h;

    bloom_rgb_to_hsv(col, &hsv[0], &hsv[1], &hsv[2]);
    square_id = bloom_get_id("##pickerSquare");
    hue_id = bloom_get_id("##pickerHue");
    alpha_id = bloom_get_id("##pickerAlpha");

    if (bloom_widget_hovered(square_rect) && ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT])
    {
        ctx->active_id = square_id;
    }
    if (bloom_widget_hovered(hue_rect) && ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT])
    {
        ctx->active_id = hue_id;
    }
    if (with_alpha && bloom_widget_hovered(alpha_rect) && ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT])
    {
        ctx->active_id = alpha_id;
    }

    if (ctx->active_id == square_id)
    {
        if (ctx->input.mouse_down[BLOOM_MOUSE_LEFT])
        {
            hsv[1] = (ctx->input.mouse_pos.x - square_rect.x) / square_rect.w;
            hsv[2] = 1.0f - ((ctx->input.mouse_pos.y - square_rect.y) / square_rect.h);
            hsv[1] = bloom_color_clamp01(hsv[1]);
            hsv[2] = bloom_color_clamp01(hsv[2]);
            bloom_hsv_to_rgb(hsv[0], hsv[1], hsv[2], rgb);
            col[0] = rgb[0];
            col[1] = rgb[1];
            col[2] = rgb[2];
            changed = BLOOM_TRUE;
        }
        else
        {
            ctx->active_id = 0;
        }
    }
    if (ctx->active_id == hue_id)
    {
        if (ctx->input.mouse_down[BLOOM_MOUSE_LEFT])
        {
            hsv[0] = (ctx->input.mouse_pos.y - hue_rect.y) / hue_rect.h;
            hsv[0] = bloom_color_clamp01(hsv[0]);
            bloom_hsv_to_rgb(hsv[0], hsv[1], hsv[2], rgb);
            col[0] = rgb[0];
            col[1] = rgb[1];
            col[2] = rgb[2];
            changed = BLOOM_TRUE;
        }
        else
        {
            ctx->active_id = 0;
        }
    }
    if (with_alpha && ctx->active_id == alpha_id)
    {
        if (ctx->input.mouse_down[BLOOM_MOUSE_LEFT])
        {
            col[3] = (ctx->input.mouse_pos.y - alpha_rect.y) / alpha_rect.h;
            col[3] = bloom_color_clamp01(col[3]);
            changed = BLOOM_TRUE;
        }
        else
        {
            ctx->active_id = 0;
        }
    }

    bloom_color_draw_card(ctx, picker_card_rect, s->input_rounding + 4.0f);
    bloom_draw_text(&ctx->draw_list,
                    bloom_v2(picker_card_rect.x + card_padding,
                             picker_card_rect.y + card_padding),
                    with_alpha ? "Tone, chroma, and opacity" : "Tone and chroma",
                    s->text_disabled,
                    s->font_size * 0.82f,
                    ctx->default_font.texture_id);

    bloom_color_draw_picker_square(ctx, square_rect, hsv[0]);
    bloom_draw_rect_rounded_border(&ctx->draw_list, square_rect, s->input_border, 8.0f, 1.0f);
    bloom_color_draw_hue_bar(ctx, hue_rect);
    bloom_draw_rect_rounded_border(&ctx->draw_list, hue_rect, s->input_border, 8.0f, 1.0f);
    if (with_alpha)
    {
        bloom_color_draw_alpha_bar(ctx, alpha_rect, col);
        bloom_draw_rect_rounded_border(&ctx->draw_list, alpha_rect, s->input_border, 8.0f, 1.0f);
    }

    bloom_color_draw_picker_marker(ctx,
        bloom_v2(square_rect.x + hsv[1] * square_rect.w,
                 square_rect.y + (1.0f - hsv[2]) * square_rect.h),
        col);
    bloom_color_draw_bar_marker(ctx,
        hue_rect,
        hue_rect.y + hsv[0] * hue_rect.h,
        col);
    if (with_alpha)
    {
        bloom_color_draw_bar_marker(ctx,
            alpha_rect,
            alpha_rect.y + col[3] * alpha_rect.h,
            col);
    }

    bloom_color_commit_manual_block(ctx, pos, available_w, total_h);

    /* inline hex input */
    if ((flags & BLOOM_COLOR_FLAGS_NO_HEX) == 0)
    {
        changed |= bloom_color_apply_hex_panel(label, col, with_alpha);
    }

    bloom_pop_id();
    bloom_color_sanitize(col, with_alpha);
    return changed;
}

bloom_bool bloom_color_swatch(const char *label, const bloom_f32 col[4], bloom_f32 w, bloom_f32 h)
{
    bloom_f32 col4[4];
    bloom_color_copy4(col4, col);
    bloom_color_sanitize(col4, BLOOM_TRUE);
    return bloom_color_swatch_row(label, col4, w, h, BLOOM_TRUE);
}

bloom_bool bloom_color_edit3(const char *label, bloom_f32 col[3])
{
    return bloom_color_edit_rgb(label, col, BLOOM_COLOR_FLAGS_NONE);
}

bloom_bool bloom_color_edit4(const char *label, bloom_f32 col[4])
{
    return bloom_color_edit_rgba(label, col, BLOOM_COLOR_FLAGS_NONE);
}

bloom_bool bloom_color_edit_rgb(const char *label, bloom_f32 col[3], bloom_u32 flags)
{
    bloom_f32 col4[4] = { col[0], col[1], col[2], 1.0f };
    bloom_bool changed = bloom_color_edit_internal(label, col4, BLOOM_FALSE, flags | BLOOM_COLOR_FLAGS_NO_ALPHA);
    col[0] = col4[0];
    col[1] = col4[1];
    col[2] = col4[2];
    return changed;
}

bloom_bool bloom_color_edit_rgba(const char *label, bloom_f32 col[4], bloom_u32 flags)
{
    return bloom_color_edit_internal(label, col, BLOOM_TRUE, flags);
}

bloom_bool bloom_color_pick_rgb(const char *label, bloom_f32 col[3], bloom_u32 flags)
{
    bloom_f32 col4[4] = { col[0], col[1], col[2], 1.0f };
    bloom_bool changed = bloom_color_pick_internal(label, col4, BLOOM_FALSE, flags | BLOOM_COLOR_FLAGS_NO_ALPHA);
    col[0] = col4[0];
    col[1] = col4[1];
    col[2] = col4[2];
    return changed;
}

bloom_bool bloom_color_pick_rgba(const char *label, bloom_f32 col[4], bloom_u32 flags)
{
    return bloom_color_pick_internal(label, col, BLOOM_TRUE, flags);
}
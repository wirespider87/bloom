#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "widgets/internal.h"

#define BLOOM_HEX_STATE_CAPACITY 32

typedef struct bloom_hex_state
{
    bloom_id id;
    bloom_bool active;
    char buffer[32];
} bloom_hex_state;

static bloom_hex_state g_hex_states[BLOOM_HEX_STATE_CAPACITY];

typedef enum
{
    BLOOM_COLOR_TAB_RGBA = 0,
    BLOOM_COLOR_TAB_HEX,
    BLOOM_COLOR_TAB_HSLA,
    BLOOM_COLOR_TAB_LAB
} bloom_color_tab;

#define BLOOM_COLOR_TAB_CAP 32
typedef struct { bloom_id id; bloom_color_tab tab; } bloom_color_tab_rec;
static bloom_color_tab_rec g_tab_recs[BLOOM_COLOR_TAB_CAP];
static int g_tab_rec_count = 0;

/* interaction state */
static struct {
    bloom_id id;
    bloom_f32 h, s, v, a;
    bloom_bool valid;
} g_color_active;

#define BLOOM_COLOR_PICK_STATE_CAPACITY 32
typedef struct bloom_color_pick_state_rec
{
    bloom_id id;
    bloom_bool open;
    bloom_f32 scroll_y_at_open;
} bloom_color_pick_state_rec;

static bloom_color_pick_state_rec g_pick_recs[BLOOM_COLOR_PICK_STATE_CAPACITY];
static int g_pick_rec_count = 0;

static bloom_color_tab *bloom_color_find_tab(bloom_id id)
{
    int i;
    for (i = 0; i < g_tab_rec_count; i++) {
        if (g_tab_recs[i].id == id) return &g_tab_recs[i].tab;
    }
    if (g_tab_rec_count < BLOOM_COLOR_TAB_CAP) {
        g_tab_recs[g_tab_rec_count].id  = id;
        g_tab_recs[g_tab_rec_count].tab = BLOOM_COLOR_TAB_RGBA;
        return &g_tab_recs[g_tab_rec_count++].tab;
    }
    return NULL;
}

static bloom_color_pick_state_rec *bloom_find_pick_rec(bloom_id id)
{
    for (int i = 0; i < g_pick_rec_count; i++) if (g_pick_recs[i].id == id) return &g_pick_recs[i];
    if (g_pick_rec_count < BLOOM_COLOR_PICK_STATE_CAPACITY) {
        g_pick_recs[g_pick_rec_count].id = id;
        g_pick_recs[g_pick_rec_count].open = BLOOM_FALSE;
        return &g_pick_recs[g_pick_rec_count++];
    }
    return NULL;
}

static bloom_hex_state *bloom_find_hex_state(bloom_id id)
{
    bloom_i32 i;
    bloom_hex_state *free_state = NULL;
    for (i = 0; i < BLOOM_HEX_STATE_CAPACITY; ++i) {
        if (g_hex_states[i].active && g_hex_states[i].id == id) return &g_hex_states[i];
        if (!free_state && !g_hex_states[i].active) free_state = &g_hex_states[i];
    }
    if (!free_state) free_state = &g_hex_states[0];
    free_state->active = BLOOM_TRUE;
    free_state->id = id;
    free_state->buffer[0] = '\0';
    return free_state;
}

static bloom_f32 bloom_color_clamp01(bloom_f32 value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

static void bloom_color_sanitize(bloom_f32 col[4], bloom_bool with_alpha)
{
    col[0] = bloom_color_clamp01(col[0]);
    col[1] = bloom_color_clamp01(col[1]);
    col[2] = bloom_color_clamp01(col[2]);
    col[3] = with_alpha ? bloom_color_clamp01(col[3]) : 1.0f;
}

static void bloom_color_copy4(bloom_f32 out[4], const bloom_f32 in[4])
{
    out[0] = in[0]; out[1] = in[1]; out[2] = in[2]; out[3] = in[3];
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

static void bloom_color_to_hex(char *buf, bloom_u32 buf_size, const bloom_f32 col[4], bloom_bool with_alpha)
{
    bloom_u8 r = (bloom_u8)(col[0]*255+0.5f), g = (bloom_u8)(col[1]*255+0.5f), b = (bloom_u8)(col[2]*255+0.5f), a = (bloom_u8)(col[3]*255+0.5f);
    if (with_alpha) snprintf(buf, buf_size, "#%02X%02X%02X%02X", r, g, b, a);
    else snprintf(buf, buf_size, "#%02X%02X%02X", r, g, b);
}

static bloom_bool bloom_parse_hex_color(const char *text, bloom_f32 col[4], bloom_bool with_alpha)
{
    if (!text || text[0] == '\0') return BLOOM_FALSE;
    const char *s = (text[0] == '#') ? text+1 : text;
    unsigned int r, g, b, a = 255;
    if (with_alpha) { if (sscanf(s, "%02x%02x%02x%02x", &r, &g, &b, &a) != 4) return BLOOM_FALSE; }
    else { if (sscanf(s, "%02x%02x%02x", &r, &g, &b) != 3) return BLOOM_FALSE; }
    col[0] = r/255.f; col[1] = g/255.f; col[2] = b/255.f; col[3] = a/255.f;
    return BLOOM_TRUE;
}

static void bloom_rgb_to_hsv(const bloom_f32 rgb[3], bloom_f32 *out_h, bloom_f32 *out_s, bloom_f32 *out_v)
{
    bloom_f32 r = rgb[0], g = rgb[1], b = rgb[2];
    bloom_f32 max_c = fmaxf(r, fmaxf(g, b)), min_c = fminf(r, fminf(g, b));
    bloom_f32 delta = max_c - min_c, h = 0.0f;
    if (delta > 0.00001f) {
        if (max_c == r) h = fmodf((g - b) / delta, 6.0f);
        else if (max_c == g) h = ((b - r) / delta) + 2.0f;
        else h = ((r - g) / delta) + 4.0f;
        h /= 6.0f; if (h < 0.0f) h += 1.0f;
    }
    *out_h = h; *out_s = (max_c <= 0.0f) ? 0.0f : delta / max_c; *out_v = max_c;
}

static void bloom_hsv_to_rgb(bloom_f32 h, bloom_f32 s, bloom_f32 v, bloom_f32 rgb[3])
{
    bloom_f32 c = v * s, x = c * (1.0f - fabsf(fmodf(h * 6.0f, 2.0f) - 1.0f)), m = v - c;
    bloom_f32 r = 0, g = 0, b = 0;
    if      (h < 1.0f/6.0f) { r = c; g = x; b = 0; }
    else if (h < 2.0f/6.0f) { r = x; g = c; b = 0; }
    else if (h < 3.0f/6.0f) { r = 0; g = c; b = x; }
    else if (h < 4.0f/6.0f) { r = 0; g = x; b = c; }
    else if (h < 5.0f/6.0f) { r = x; g = 0; b = c; }
    else                    { r = c; g = 0; b = x; }
    rgb[0] = r + m; rgb[1] = g + m; rgb[2] = b + m;
}

static bloom_f32 bloom_color_linearize(bloom_f32 value) {
    return (value <= 0.04045f) ? (value / 12.92f) : powf((value + 0.055f) / 1.055f, 2.4f);
}
static bloom_f32 bloom_color_encode(bloom_f32 value) {
    return (value <= 0.0031308f) ? (value * 12.92f) : (1.055f * powf(value, 1.0f / 2.4f) - 0.055f);
}
static bloom_f32 bloom_xyz_f(bloom_f32 t) {
    return (t > 0.008856f) ? powf(t, 1.0f/3.0f) : (7.787f * t + 16.0f/116.0f);
}
static bloom_f32 bloom_xyz_finv(bloom_f32 t) {
    return (t*t*t > 0.008856f) ? (t*t*t) : ((t - 16.0f/116.0f) / 7.787f);
}

static void bloom_rgb_to_lab(const bloom_f32 rgb[3], bloom_f32 lab[3]) {
    bloom_f32 r = bloom_color_linearize(rgb[0]), g = bloom_color_linearize(rgb[1]), b = bloom_color_linearize(rgb[2]);
    bloom_f32 x = (r * 0.4124f + g * 0.3576f + b * 0.1805f) / 0.95047f;
    bloom_f32 y = (r * 0.2126f + g * 0.7152f + b * 0.0722f) / 1.00000f;
    bloom_f32 z = (r * 0.0193f + g * 0.1192f + b * 0.9505f) / 1.08883f;
    lab[0] = 116.0f * bloom_xyz_f(y) - 16.0f;
    lab[1] = 500.0f * (bloom_xyz_f(x) - bloom_xyz_f(y));
    lab[2] = 200.0f * (bloom_xyz_f(y) - bloom_xyz_f(z));
}

static void bloom_lab_to_rgb(const bloom_f32 lab[3], bloom_f32 rgb[3]) {
    bloom_f32 y = (lab[0] + 16.0f) / 116.0f, x = lab[1] / 500.0f + y, z = y - lab[2] / 200.0f;
    bloom_f32 rx = 0.95047f * bloom_xyz_finv(x), ry = 1.00000f * bloom_xyz_finv(y), rz = 1.08883f * bloom_xyz_finv(z);
    rgb[0] = bloom_color_encode(rx * 3.2406f + ry * -1.5372f + rz * -0.4986f);
    rgb[1] = bloom_color_encode(rx * -0.9689f + ry * 1.8758f + rz * 0.0415f);
    rgb[2] = bloom_color_encode(rx * 0.0557f + ry * -0.2040f + rz * 1.0570f);
}

static void bloom_rgb_to_hsl(const bloom_f32 rgb[3], bloom_f32 hsl[3]) {
    bloom_f32 r = rgb[0], g = rgb[1], b = rgb[2];
    bloom_f32 max_c = fmaxf(r, fmaxf(g, b)), min_c = fminf(r, fminf(g, b));
    bloom_f32 h, s, l = (max_c + min_c) * 0.5f;
    if (max_c == min_c) h = s = 0;
    else {
        bloom_f32 d = max_c - min_c;
        s = l > 0.5f ? d / (2.0f - max_c - min_c) : d / (max_c + min_c);
        if      (max_c == r) h = (g - b) / d + (g < b ? 6 : 0);
        else if (max_c == g) h = (b - r) / d + 2;
        else                 h = (r - g) / d + 4;
        h /= 6.0f;
    }
    hsl[0] = h; hsl[1] = s; hsl[2] = l;
}

static void bloom_color_draw_card(bloom_context *ctx, bloom_rect rect, bloom_f32 rounding)
{
    bloom_style *s = &ctx->style;
    bloom_color fill = bloom_apply_state_layer(s->window_bg, s->text_default, 0.04f);
    bloom_color border = bloom_scale_alpha(s->window_border, 0.85f);
    bloom_draw_rect_rounded(&ctx->draw_list, rect, fill, rounding);
    bloom_draw_rect_rounded_border(&ctx->draw_list, rect, border, rounding, 1.0f);
}

static void bloom_color_draw_checkerboard(bloom_context *ctx, bloom_rect rect, bloom_f32 cell_size, bloom_f32 rounding)
{
    bloom_style *s = &ctx->style;
    bloom_color col = bloom_apply_state_layer(s->window_bg, s->text_default, 0.1f);
    bloom_draw_checkerboard(&ctx->draw_list, rect, cell_size, col, bloom_make_corner_radii_all(rounding));
}

static bloom_bool bloom_color_swatch_row(const char *label, const bloom_f32 col[4], bloom_f32 w, bloom_f32 h, bloom_bool clickable)
{
    bloom_context *ctx = bloom_get_context();
    if (!ctx || !ctx->current_window) return BLOOM_FALSE;
    bloom_style *s = &ctx->style;
    bloom_vec2 pos = ctx->current_window->layout.cursor;

    bloom_f32 swatch_size = bloom_scaled_line_height(ctx, s->font_size);
    if (h <= 0.0f) h = swatch_size + s->touch_padding * 1.6f;

    bloom_f32 label_w = bloom_label_width(ctx, label, s->font_size);
    bloom_f32 total_w = swatch_size + (label_w > 0.0f ? (s->item_inner_spacing + label_w) : 0.0f);
    if (w > total_w) total_w = w;

    bloom_rect swatch_rect = bloom_make_rect(pos.x, pos.y + (h - swatch_size) * 0.5f, swatch_size, swatch_size);
    bloom_rect hit_rect = bloom_make_rect(pos.x, pos.y, total_w, h);
    bloom_color preview = bloom_color_from_f32(col);
    bloom_bool hovered = clickable ? bloom_widget_hovered(hit_rect) : BLOOM_FALSE;
    bloom_id id = bloom_get_id(label);
    bloom_bool pressed = BLOOM_FALSE;

    if (clickable && hovered) {
        ctx->hot_id = id;
        if (ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT]) ctx->active_id = id;
    }
    if (clickable && ctx->active_id == id && ctx->input.mouse_released[BLOOM_MOUSE_LEFT]) {
        if (hovered) pressed = BLOOM_TRUE;
        ctx->active_id = 0;
    }

    if (preview.a < 255) bloom_color_draw_checkerboard(ctx, swatch_rect, 6.0f, s->color_preview_rounding);
    bloom_draw_rect_rounded(&ctx->draw_list, swatch_rect, preview, s->color_preview_rounding);
    bloom_draw_rect_rounded_border(&ctx->draw_list, swatch_rect, hovered ? s->input_cursor : s->checkbox_border, s->color_preview_rounding, 1.0f);

    if (label_w > 0.0f) {
        bloom_draw_label(ctx,
                         bloom_v2(pos.x + swatch_size + s->item_inner_spacing,
                                  bloom_centered_text_y(ctx, pos.y, h, s->font_size)),
                         label, hovered ? s->input_cursor : s->text_default, s->font_size);
    }

    bloom_advance_layout(total_w, h);
    return pressed;
}

static void bloom_color_draw_ring_marker(bloom_context *ctx, bloom_vec2 center, bloom_f32 radius, bloom_color color)
{
    bloom_draw_circle_filled(&ctx->draw_list, center, radius, bloom_rgba(255, 255, 255, 220), 24);
    bloom_draw_circle_filled(&ctx->draw_list, center, radius * 0.75f, color, 24);
    bloom_draw_circle_filled(&ctx->draw_list, center, radius * 0.45f, bloom_rgba(255, 255, 255, 255), 24);
}

static void bloom_color_draw_picker_square(bloom_context *ctx, bloom_rect rect, bloom_f32 hsv[4], bloom_bool with_alpha, int active_tab)
{
    bloom_f32 r = ctx->style.window_rounding;
    bloom_f32 hue_rgb[3]; bloom_hsv_to_rgb(hsv[0], 1.0f, 1.0f, hue_rgb);
    bloom_color hue_col = bloom_rgb((bloom_u8)(hue_rgb[0]*255), (bloom_u8)(hue_rgb[1]*255), (bloom_u8)(hue_rgb[2]*255));

    bloom_draw_color_picker_square(&ctx->draw_list, rect, hue_col, bloom_make_corner_radii_all(r));

    char val_buf[64]; bloom_f32 rgb[3]; bloom_hsv_to_rgb(hsv[0], hsv[1], hsv[2], rgb);
    if (active_tab == BLOOM_COLOR_TAB_RGBA) {
        if (with_alpha) snprintf(val_buf, sizeof(val_buf), "RGBA (%d, %d, %d, %.2f)", (int)(rgb[0]*255), (int)(rgb[1]*255), (int)(rgb[2]*255), hsv[3]);
        else snprintf(val_buf, sizeof(val_buf), "RGB (%d, %d, %d)", (int)(rgb[0]*255), (int)(rgb[1]*255), (int)(rgb[2]*255));
    } else if (active_tab == BLOOM_COLOR_TAB_HEX) {
        bloom_color_to_hex(val_buf, sizeof(val_buf), (bloom_f32[]){rgb[0],rgb[1],rgb[2],hsv[3]}, with_alpha);
    } else if (active_tab == BLOOM_COLOR_TAB_HSLA) {
        bloom_f32 hsl[3]; bloom_rgb_to_hsl(rgb, hsl);
        snprintf(val_buf, sizeof(val_buf), "HSLA (%d, %d%%, %d%%, %.2f)", (int)(hsl[0]*360), (int)(hsl[1]*100), (int)(hsl[2]*100), hsv[3]);
    } else if (active_tab == BLOOM_COLOR_TAB_LAB) {
        bloom_f32 lab[3]; bloom_rgb_to_lab(rgb, lab);
        snprintf(val_buf, sizeof(val_buf), "LAB (%.1f, %.1f, %.1f)", lab[0], lab[1], lab[2]);
    }

    bloom_f32 tw = bloom_text_width(val_buf, 12.0f);
    bloom_rect text_rect = bloom_make_rect(rect.x + (rect.w - tw)*0.5f - 4, rect.y + rect.h - 26, tw + 8, 20);
    bloom_bool text_hovered = bloom_widget_hovered(text_rect);
    bloom_id text_id = bloom_get_id("##val_text");
    if (text_hovered && ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT]) ctx->focus_id = text_id;

    if (ctx->focus_id == text_id) {
        bloom_hex_state *hs = bloom_find_hex_state(text_id);
        if (hs->buffer[0] == '\0') bloom_color_to_hex(hs->buffer, sizeof(hs->buffer), (bloom_f32[]){rgb[0],rgb[1],rgb[2],hsv[3]}, with_alpha);
        bloom_vec2 sc = ctx->current_window->layout.cursor; ctx->current_window->layout.cursor = bloom_v2(text_rect.x, text_rect.y);
        if (bloom_text_input("##edit_val", hs->buffer, sizeof(hs->buffer))) {
            bloom_f32 c4[4]; if (bloom_parse_hex_color(hs->buffer, c4, with_alpha)) {
                bloom_color_sanitize(c4, with_alpha);
                bloom_rgb_to_hsv(c4, &hsv[0], &hsv[1], &hsv[2]); hsv[3] = c4[3];
            }
        }
        ctx->current_window->layout.cursor = sc;
    } else {
        bloom_draw_text(&ctx->draw_list, bloom_v2(text_rect.x+4, text_rect.y+4), val_buf, text_hovered ? bloom_rgba(255,255,255,255) : bloom_rgba(255,255,255,160), 12.0f, ctx->default_font.texture_id);
    }
}

static void bloom_color_draw_hue_bar_horiz(bloom_context *ctx, bloom_rect rect, bloom_f32 hue) {
    bloom_f32 r = ctx->style.checkerboard_rounding;
    bloom_draw_hue_bar(&ctx->draw_list, rect, bloom_make_corner_radii_all(r));
}

static void bloom_color_draw_alpha_bar_horiz(bloom_context *ctx, bloom_rect rect, const bloom_f32 col[4]) {
    bloom_f32 r = ctx->style.checkerboard_rounding;
    bloom_color_draw_checkerboard(ctx, rect, 6.0f, r);
    bloom_draw_alpha_bar(&ctx->draw_list, rect, bloom_color_from_f32(col), bloom_make_corner_radii_all(r));
}

static bloom_bool bloom_color_picker_card(const char *label, bloom_f32 col[4], bloom_bool with_alpha, bloom_u32 flags, bloom_bool always_open, bloom_bool advance_layout)
{
    bloom_context *ctx = bloom_get_context(); if (!ctx || !ctx->current_window) return BLOOM_FALSE;
    bloom_style *s = &ctx->style; bloom_vec2 pos = ctx->current_window->layout.cursor; bloom_f32 w = ctx->current_window->layout.available_width;
    bloom_f32 pad = 12, sq_h = 160, bar_h = 22;
    bloom_f32 hsv[4];
    bloom_id id = bloom_get_id(label);
    bloom_id sq_id = bloom_get_id("##sq"), hue_id = bloom_get_id("##hue"), alpha_id = bloom_get_id("##alpha");
    if (g_color_active.id == id && g_color_active.valid) {
        hsv[0] = g_color_active.h; hsv[1] = g_color_active.s; hsv[2] = g_color_active.v; hsv[3] = g_color_active.a;
    } else {
        bloom_rgb_to_hsv(col, &hsv[0], &hsv[1], &hsv[2]); hsv[3] = col[3];
    }
    bloom_bool changed = BLOOM_FALSE;
    bloom_id tab_id = bloom_get_id("##picker_tabs"); bloom_color_tab *active_tab = bloom_color_find_tab(tab_id);

    bloom_f32 gap_sq_hue = 12;
    bloom_f32 gap_hue_alpha = 10;
    bloom_f32 gap_alpha_foot = 14;
    bloom_f32 foot_h = 20;

    bloom_f32 total_h = pad + sq_h + gap_sq_hue + bar_h + (with_alpha ? (gap_hue_alpha + bar_h) : 0) + gap_alpha_foot + foot_h + pad;

    bloom_rect card_rect = bloom_make_rect(pos.x, pos.y, w, total_h);
    bloom_color_draw_card(ctx, card_rect, s->window_rounding);

    bloom_rect sq_r = bloom_make_rect(pos.x + pad, pos.y + pad, w - pad*2, sq_h);
    bloom_id text_id = bloom_get_id("##val_text");
    bloom_f32 sq_mr = 6.0f;
    if (ctx->focus_id != text_id) {
        if (bloom_widget_hovered(sq_r) && ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT]) ctx->active_id = sq_id;
        if (ctx->active_id == sq_id) {
            if (ctx->input.mouse_down[BLOOM_MOUSE_LEFT]) {
                hsv[1] = bloom_color_clamp01((ctx->input.mouse_pos.x - (sq_r.x + sq_mr))/(sq_r.w - sq_mr*2));
                hsv[2] = 1.0f - bloom_color_clamp01((ctx->input.mouse_pos.y - (sq_r.y + sq_mr))/(sq_r.h - sq_mr*2));
                changed = BLOOM_TRUE;
            } else ctx->active_id = 0;
        }
    }
    bloom_color_draw_picker_square(ctx, sq_r, hsv, with_alpha, (int)*active_tab);
    bloom_vec2 sq_mpos = bloom_v2(sq_r.x + sq_mr + hsv[1]*(sq_r.w - sq_mr*2), sq_r.y + sq_mr + (1-hsv[2])*(sq_r.h - sq_mr*2));
    bloom_color_draw_ring_marker(ctx, sq_mpos, sq_mr, bloom_color_from_f32(col));

    bloom_f32 bar_mr = 10.0f;
    bloom_rect hue_r = bloom_make_rect(pos.x + pad, sq_r.y + sq_r.h + gap_sq_hue, w - pad*2, bar_h);
    if (bloom_widget_hovered(hue_r) && ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT]) ctx->active_id = hue_id;
    if (ctx->active_id == hue_id) {
        if (ctx->input.mouse_down[BLOOM_MOUSE_LEFT]) {
            hsv[0] = bloom_color_clamp01((ctx->input.mouse_pos.x - (hue_r.x + bar_mr))/(hue_r.w - bar_mr*2));
            changed = BLOOM_TRUE;
        }
        else ctx->active_id = 0;
    }
    bloom_color_draw_hue_bar_horiz(ctx, hue_r, hsv[0]);
    bloom_color_draw_ring_marker(ctx, bloom_v2(hue_r.x + bar_mr + hsv[0]*(hue_r.w - bar_mr*2), hue_r.y + bar_h*0.5f), bar_mr, bloom_rgba(255,255,255,255));

    bloom_f32 last_bar_y = hue_r.y + hue_r.h;
    if (with_alpha) {
        bloom_rect al_r = bloom_make_rect(pos.x + pad, hue_r.y + hue_r.h + gap_hue_alpha, w - pad*2, bar_h);
        if (bloom_widget_hovered(al_r) && ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT]) ctx->active_id = alpha_id;
        if (ctx->active_id == alpha_id) {
            if (ctx->input.mouse_down[BLOOM_MOUSE_LEFT]) {
                hsv[3] = bloom_color_clamp01((ctx->input.mouse_pos.x - (al_r.x + bar_mr))/(al_r.w - bar_mr*2));
                col[3] = hsv[3];
                changed = BLOOM_TRUE;
            }
            else ctx->active_id = 0;
        }
        bloom_color_draw_alpha_bar_horiz(ctx, al_r, col);
        bloom_color_draw_ring_marker(ctx, bloom_v2(al_r.x + bar_mr + hsv[3]*(al_r.w - bar_mr*2), al_r.y + bar_h*0.5f), bar_mr, bloom_rgba(255,255,255,255));
        last_bar_y = al_r.y + al_r.h;
    }

    const char* footer = "RGBA / HEX / HSLA / LAB"; bloom_f32 ftw = bloom_text_width(footer, 10);
    bloom_rect foot_r = bloom_make_rect(pos.x + (w-ftw)*0.5f, last_bar_y + gap_alpha_foot, ftw, foot_h);
    bloom_draw_text(&ctx->draw_list, bloom_v2(foot_r.x, foot_r.y), footer, bloom_rgba(150,150,150,255), 10, ctx->default_font.texture_id);
    if (bloom_widget_hovered(foot_r) && ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT]) { *active_tab = (*active_tab + 1) % 4; bloom_anim_reset(bloom_get_id("##val_text")); }

    if (changed || ctx->active_id == sq_id || ctx->active_id == hue_id || ctx->active_id == alpha_id) {
        g_color_active.id = id; g_color_active.h = hsv[0]; g_color_active.s = hsv[1]; g_color_active.v = hsv[2]; g_color_active.a = hsv[3]; g_color_active.valid = BLOOM_TRUE;
        if (changed) {
            bloom_hsv_to_rgb(hsv[0], hsv[1], hsv[2], col);
            if (with_alpha) col[3] = hsv[3];
        }
    } else if (g_color_active.id == id) {
        bloom_f32 cur_hsv[3];
        bloom_rgb_to_hsv(col, &cur_hsv[0], &cur_hsv[1], &cur_hsv[2]);
        if (cur_hsv[1] > 0.01f && cur_hsv[2] > 0.01f) {
             if (fabsf(cur_hsv[1] - g_color_active.s) > 0.01f || fabsf(cur_hsv[2] - g_color_active.v) > 0.01f) {
                 g_color_active.valid = BLOOM_FALSE; /* Force re-sync */
             }
        }
    }
    if (advance_layout) bloom_advance_layout(w, total_h);
    return changed;
}

bloom_bool bloom_color_edit_rgba(const char *label, bloom_f32 col[4], bloom_u32 flags) { return bloom_color_picker_card(label, col, BLOOM_TRUE, flags, BLOOM_TRUE, BLOOM_TRUE); }
bloom_bool bloom_color_edit_rgb(const char *label, bloom_f32 col[3], bloom_u32 flags) { bloom_f32 c4[4]={col[0],col[1],col[2],1.0f}; bloom_bool r=bloom_color_picker_card(label, c4, BLOOM_FALSE, flags, BLOOM_TRUE, BLOOM_TRUE); col[0]=c4[0];col[1]=c4[1];col[2]=c4[2]; return r; }

bloom_bool bloom_color_pick_rgba(const char *label, bloom_f32 col[4], bloom_u32 flags) {
    bloom_context *ctx = bloom_get_context(); bloom_id id = bloom_get_id(label);
    bloom_color_pick_state_rec *state = bloom_find_pick_rec(id);
    bloom_window *win = ctx->current_window;
    if (bloom_color_swatch_row(label, col, 0, 0, BLOOM_TRUE)) {
        state->open = !state->open;
        if (state->open && win) {
            state->scroll_y_at_open = win->scroll_y;
            g_color_active.id = id;
            bloom_rgb_to_hsv(col, &g_color_active.h, &g_color_active.s, &g_color_active.v);
            g_color_active.a = col[3];
            g_color_active.valid = BLOOM_TRUE;
        }
    }
    if (state->open) {
        if (win && fabsf(win->scroll_y - state->scroll_y_at_open) > 2.0f) { state->open = BLOOM_FALSE; return BLOOM_FALSE; }
        bloom_rect sw_r = bloom_make_rect(ctx->current_window->layout.last_item_pos.x, ctx->current_window->layout.last_item_pos.y, ctx->current_window->layout.last_item_size.x, ctx->current_window->layout.last_item_size.y);
        bloom_popup_begin_deferred_draw(ctx);
        bloom_rect pr = bloom_make_rect(sw_r.x, sw_r.y + sw_r.h + 8, 260, 340);
        if (pr.x + pr.w > ctx->display_size.x) pr.x = ctx->display_size.x - pr.w - 8;
        bloom_vec2 sc = ctx->current_window->layout.cursor; bloom_f32 sa = ctx->current_window->layout.available_width;
        ctx->current_window->layout.cursor = bloom_v2(pr.x, pr.y); ctx->current_window->layout.available_width = pr.w;
        bloom_bool changed = bloom_color_picker_card(label, col, BLOOM_TRUE, flags, BLOOM_TRUE, BLOOM_FALSE);
        ctx->current_window->layout.cursor = sc; ctx->current_window->layout.available_width = sa;
        bloom_popup_end_deferred_draw(ctx);

        if (changed) {
            bloom_f32 swatch_size = bloom_scaled_line_height(ctx, ctx->style.font_size);
            bloom_rect swatch_rect = bloom_make_rect(sw_r.x, sw_r.y + (sw_r.h - swatch_size) * 0.5f, swatch_size, swatch_size);
            bloom_draw_push_clip(&ctx->draw_list, swatch_rect);
            bloom_color_draw_checkerboard(ctx, swatch_rect, 6.0f, ctx->style.color_preview_rounding);
            bloom_draw_rect_rounded(&ctx->draw_list, swatch_rect, bloom_color_from_f32(col), ctx->style.color_preview_rounding);
            bloom_draw_pop_clip(&ctx->draw_list);
        }

        if (ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT] && !bloom_rect_contains(pr, ctx->input.mouse_pos) && !bloom_rect_contains(sw_r, ctx->input.mouse_pos)) state->open = BLOOM_FALSE;
        return changed;
    }
    return BLOOM_FALSE;
}
bloom_bool bloom_color_swatch(const char *label, const bloom_f32 col[4], bloom_f32 w, bloom_f32 h) { return bloom_color_swatch_row(label, col, w, h, BLOOM_TRUE); }
bloom_bool bloom_color_edit4(const char *label, bloom_f32 col[4]) { return bloom_color_edit_rgba(label, col, 0); }

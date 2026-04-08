#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "widgets/internal.h"
#include <stdio.h>

#define BLOOM_POPUP_MAX_VERTICES 32768
#define BLOOM_POPUP_MAX_INDICES  65536
#define BLOOM_POPUP_MAX_CMDS     1024

static bloom_vertex g_popup_vertices[BLOOM_POPUP_MAX_VERTICES];
static bloom_draw_idx g_popup_indices[BLOOM_POPUP_MAX_INDICES];
static bloom_draw_cmd g_popup_commands[BLOOM_POPUP_MAX_CMDS];
static bloom_draw_list g_popup_draw_list;
static bloom_draw_list g_popup_saved_main_draw_list;
static bloom_bool g_popup_draw_list_initialized = BLOOM_FALSE;
bloom_bool g_popup_draw_redirect_active = BLOOM_FALSE;

void bloom_advance_layout(bloom_f32 w, bloom_f32 h)
{
    bloom_context *ctx = bloom_get_context();
    if (!ctx || !ctx->current_window)
    {
        return;
    }
    bloom_window *win = ctx->current_window;
    bloom_vec2 item_pos = win->layout.cursor;

    if (h > win->layout.max_row_height)
    {
        win->layout.max_row_height = h;
    }

    {
        bloom_f32 item_bottom = item_pos.y + win->layout.max_row_height + win->scroll_y - win->content_rect.y;
        if (item_bottom > win->content_extent_y)
        {
            win->content_extent_y = item_bottom;
        }
    }

    win->layout.last_item_pos = item_pos;
    win->layout.last_item_size = bloom_v2(w, h);

    if (win->layout.type == BLOOM_LAYOUT_HORIZONTAL)
    {
        win->layout.cursor.x = win->layout.start.x + win->layout.indent;
        win->layout.cursor.y = item_pos.y + win->layout.max_row_height + ctx->style.item_spacing;
        win->layout.max_row_height = 0.0f;
        win->layout.type = BLOOM_LAYOUT_VERTICAL;
    }
    else if (win->layout.type == BLOOM_LAYOUT_FLOW)
    {
        bloom_f32 row_end = win->layout.start.x + win->layout.indent + win->layout.available_width;
        bloom_f32 next_x = item_pos.x + w + ctx->style.item_inner_spacing;

        if (next_x >= row_end)
        {
            win->layout.cursor.x = win->layout.start.x + win->layout.indent;
            win->layout.cursor.y = item_pos.y + win->layout.max_row_height + ctx->style.item_spacing;
            win->layout.max_row_height = 0.0f;
        }
        else
        {
            win->layout.cursor.x = next_x;
            win->layout.cursor.y = item_pos.y;
        }
    }
    else
    {
        win->layout.cursor.x = win->layout.start.x + win->layout.indent;
        win->layout.cursor.y = item_pos.y + h + ctx->style.item_spacing;
        win->layout.max_row_height = 0;
    }
}

bloom_bool g_popup_input_block = BLOOM_FALSE;
bloom_rect g_popup_input_rect;
bloom_bool g_popup_persist_open = BLOOM_FALSE;
bloom_rect g_popup_persist_rect;

static void bloom_popup_draw_list_init_once(void)
{
    if (!g_popup_draw_list_initialized)
    {
        bloom_draw_list_init(&g_popup_draw_list,
                             g_popup_vertices, BLOOM_POPUP_MAX_VERTICES,
                             g_popup_indices, BLOOM_POPUP_MAX_INDICES,
                             g_popup_commands, BLOOM_POPUP_MAX_CMDS);
        g_popup_draw_list_initialized = BLOOM_TRUE;
    }
}

void bloom_popup_begin_deferred_draw(bloom_context *ctx)
{
    bloom_popup_draw_list_init_once();
    bloom_draw_list_clear(&g_popup_draw_list);
    g_popup_draw_list.current_texture = 0;
    g_popup_saved_main_draw_list = ctx->draw_list;
    ctx->draw_list = g_popup_draw_list;
    g_popup_draw_redirect_active = BLOOM_TRUE;
}

void bloom_popup_end_deferred_draw(bloom_context *ctx)
{
    if (!g_popup_draw_redirect_active)
    {
        return;
    }

    g_popup_draw_list = ctx->draw_list;
    ctx->draw_list = g_popup_saved_main_draw_list;
    g_popup_draw_redirect_active = BLOOM_FALSE;
}

void bloom_widgets_begin_frame(void)
{
    bloom_popup_draw_list_init_once();
    bloom_draw_list_clear(&g_popup_draw_list);
    g_popup_draw_list.current_texture = 0;
    g_popup_draw_redirect_active = BLOOM_FALSE;
    g_popup_input_block = g_popup_persist_open;
    g_popup_input_rect = g_popup_persist_open ? g_popup_persist_rect : bloom_make_rect(0.0f, 0.0f, 0.0f, 0.0f);
    bloom_tooltip_begin_frame();
}

void bloom_widgets_end_frame(void)
{
    bloom_context *ctx = bloom_get_context();
    if (!ctx)
    {
        return;
    }

    if (g_popup_draw_redirect_active)
    {
        bloom_popup_end_deferred_draw(ctx);
    }

    bloom_draw_list_append(&ctx->draw_list, &g_popup_draw_list);
    bloom_tooltip_flush_deferred();
}

bloom_bool bloom_widget_hovered(bloom_rect rect)
{
    bloom_context *ctx = bloom_get_context();
    if (!ctx)
    {
        return BLOOM_FALSE;
    }
    if (!bloom_rect_contains(rect, ctx->input.mouse_pos))
    {
        return BLOOM_FALSE;
    }
    if (g_popup_input_block && bloom_rect_contains(g_popup_input_rect, ctx->input.mouse_pos))
    {
        return BLOOM_FALSE;
    }
    if (ctx->current_window && !bloom_window_accepts_input(ctx->current_window, ctx->input.mouse_pos))
    {
        return BLOOM_FALSE;
    }
    return BLOOM_TRUE;
}

bloom_color bloom_color_mix(bloom_color a, bloom_color b, bloom_f32 t)
{
    bloom_color out;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    out.r = (bloom_u8)((bloom_f32)a.r + ((bloom_f32)b.r - (bloom_f32)a.r) * t);
    out.g = (bloom_u8)((bloom_f32)a.g + ((bloom_f32)b.g - (bloom_f32)a.g) * t);
    out.b = (bloom_u8)((bloom_f32)a.b + ((bloom_f32)b.b - (bloom_f32)a.b) * t);
    out.a = (bloom_u8)((bloom_f32)a.a + ((bloom_f32)b.a - (bloom_f32)a.a) * t);
    return out;
}

bloom_color bloom_apply_state_layer(bloom_color base, bloom_color state, bloom_f32 amount)
{
    return bloom_color_mix(base, state, amount);
}

bloom_f32 bloom_lerp_f32(bloom_f32 a, bloom_f32 b, bloom_f32 t)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return a + (b - a) * t;
}

bloom_f32 bloom_table_row_height(bloom_context *ctx)
{
    return ctx->default_font.line_height + 16.0f;
}

bloom_u32 bloom_label_visible_length(const char *label)
{
    const char *scan = label;
    while (*scan)
    {
        if (scan[0] == '#' && scan[1] == '#')
        {
            return (bloom_u32)(scan - label);
        }
        scan++;
    }
    return (bloom_u32)(scan - label);
}

bloom_f32 bloom_scaled_line_height(bloom_context *ctx, bloom_f32 font_size)
{
    if (ctx->default_font.valid && ctx->default_font.size > 0.0f)
    {
        return ctx->default_font.line_height * (font_size / ctx->default_font.size);
    }
    return font_size;
}

bloom_f32 bloom_centered_text_y(bloom_context *ctx, bloom_f32 top, bloom_f32 height, bloom_f32 font_size)
{
    return top + (height - bloom_scaled_line_height(ctx, font_size)) * 0.5f;
}

bloom_bool bloom_window_animations_enabled(bloom_context *ctx)
{
    if (!ctx || !ctx->current_window)
    {
        return BLOOM_TRUE;
    }
    return (ctx->current_window->flags & BLOOM_WINDOW_NO_ANIMATIONS) == 0;
}

bloom_f32 bloom_window_anim_toggle(bloom_context *ctx, bloom_id id, bloom_bool on, bloom_f32 response)
{
    if (!bloom_window_animations_enabled(ctx))
    {
        return on ? 1.0f : 0.0f;
    }
    return bloom_anim_toggle(id, on, response);
}

bloom_f32 bloom_window_anim_pulse(bloom_context *ctx, bloom_id id, bloom_bool active,
                                         bloom_f32 attack, bloom_f32 decay)
{
    if (!bloom_window_animations_enabled(ctx))
    {
        return active ? 1.0f : 0.0f;
    }
    return bloom_anim_pulse(id, active, attack, decay);
}

bloom_f32 bloom_window_anim_spring(bloom_context *ctx, bloom_id id, bloom_f32 target,
                                          bloom_f32 stiffness, bloom_f32 damping)
{
    if (!bloom_window_animations_enabled(ctx))
    {
        return target;
    }
    return bloom_anim_spring(id, target, stiffness, damping);
}

bloom_f32 bloom_window_anim_state(bloom_context *ctx, bloom_id id, bloom_f32 target,
                                         bloom_f32 response)
{
    if (!bloom_window_animations_enabled(ctx))
    {
        return target;
    }
    return bloom_anim_state(id, target, response);
}

void bloom_draw_wrapped_text_lines(bloom_context *ctx, bloom_vec2 pos, const char *text,
                                          bloom_color col, bloom_f32 font_size,
                                          bloom_f32 max_width, bloom_f32 *out_width,
                                          bloom_f32 *out_height)
{
    const char *line_start = text;
    bloom_f32 line_y = pos.y;
    bloom_f32 line_height = bloom_scaled_line_height(ctx, font_size);
    bloom_f32 max_line_width = 0.0f;

    while (line_start && *line_start)
    {
        const char *scan = line_start;
        const char *last_space = NULL;
        const char *break_at = NULL;
        bloom_f32 width = 0.0f;

        while (*scan)
        {
            bloom_f32 candidate_width;

            if (*scan == '\n')
            {
                break_at = scan;
                break;
            }

            if (*scan == ' ' || *scan == '\t')
            {
                last_space = scan;
            }

            candidate_width = bloom_text_width_n(line_start, (bloom_u32)(scan - line_start + 1), font_size);
            if (candidate_width > max_width && scan > line_start)
            {
                break_at = last_space ? last_space : scan;
                break;
            }

            width = candidate_width;
            scan++;
        }

        if (!break_at)
        {
            break_at = scan;
            width = bloom_text_width_n(line_start, (bloom_u32)(break_at - line_start), font_size);
        }

        while (break_at > line_start && (break_at[-1] == ' ' || break_at[-1] == '\t'))
        {
            break_at--;
            width = bloom_text_width_n(line_start, (bloom_u32)(break_at - line_start), font_size);
        }

        if (break_at > line_start)
        {
            bloom_draw_text_n(&ctx->draw_list, bloom_v2(pos.x, line_y), line_start,
                              (bloom_u32)(break_at - line_start), col, font_size,
                              ctx->default_font.texture_id);
            if (width > max_line_width)
            {
                max_line_width = width;
            }
        }

        if (*scan == '\0')
        {
            line_y += line_height;
            break;
        }

        line_start = break_at;
        while (*line_start == ' ' || *line_start == '\t') line_start++;
        if (*line_start == '\n') line_start++;
        line_y += line_height;
    }

    if (line_start == text || text[0] == '\0')
    {
        line_y += line_height;
    }

    if (out_width)
    {
        *out_width = max_line_width;
    }
    if (out_height)
    {
        *out_height = line_y - pos.y;
    }
}

bloom_f32 bloom_label_width(bloom_context *ctx, const char *label, bloom_f32 font_size)
{
    bloom_u32 visible_len = bloom_label_visible_length(label);
    bloom_f32 width = bloom_text_width_n(label, visible_len, font_size);
    if (visible_len > 0 && width > 0.0f)
    {
        width += 1.0f;
    }
    return width;
}

void bloom_draw_label(bloom_context *ctx, bloom_vec2 pos, const char *label, bloom_color col, bloom_f32 font_size)
{
    bloom_u32 visible_len = bloom_label_visible_length(label);
    if (visible_len == 0)
    {
        return;
    }
    bloom_draw_text_n(&ctx->draw_list, pos, label, visible_len, col, font_size, ctx->default_font.texture_id);
}

void bloom_buffer_clear(char *buf, bloom_u32 buf_size)
{
    if (buf_size > 0)
    {
        buf[0] = '\0';
    }
}

bloom_bool bloom_buffer_append(char *buf, bloom_u32 buf_size, const char *text)
{
    bloom_u32 len;
    bloom_u32 avail;
    bloom_u32 i;
    bloom_bool changed = BLOOM_FALSE;

    if (!buf || !text || buf_size == 0)
    {
        return BLOOM_FALSE;
    }

    len = (bloom_u32)strlen(buf);
    if (len >= buf_size - 1)
    {
        return BLOOM_FALSE;
    }

    avail = (buf_size - 1) - len;
    for (i = 0; text[i] != '\0' && i < avail; ++i)
    {
        if ((unsigned char)text[i] < 32)
        {
            continue;
        }
        buf[len++] = text[i];
        changed = BLOOM_TRUE;
    }
    buf[len] = '\0';
    return changed;
}

bloom_color bloom_scale_alpha(bloom_color color, bloom_f32 scale)
{
    bloom_color out = color;
    bloom_f32 alpha = (bloom_f32)out.a * scale;
    if (alpha < 0.0f)
    {
        alpha = 0.0f;
    }
    if (alpha > 255.0f)
    {
        alpha = 255.0f;
    }
    out.a = (bloom_u8)(alpha + 0.5f);
    return out;
}

void bloom_draw_soft_circle(bloom_context *ctx, bloom_vec2 center, bloom_f32 radius, bloom_color color)
{
    bloom_style *s = &ctx->style;

    bloom_draw_circle_filled(&ctx->draw_list, center, radius + 0.9f,
                             bloom_scale_alpha(color, 0.22f),
                             s->circle_segments + 12);
    bloom_draw_circle_filled(&ctx->draw_list, center, radius, color, s->circle_segments + 12);
}

void bloom_draw_soft_line(bloom_context *ctx, bloom_vec2 a, bloom_vec2 b,
                                 bloom_color color, bloom_f32 thickness)
{
    bloom_draw_line(&ctx->draw_list, a, b, bloom_scale_alpha(color, 0.20f), thickness + 1.4f);
    bloom_draw_line(&ctx->draw_list, a, b, color, thickness);
}

bloom_toggle_args bloom_resolve_toggle_args(const bloom_toggle_args *args)
{
    bloom_toggle_args resolved = BLOOM_TOGGLE_ARGS_DEFAULT;
    if (args)
    {
        resolved = *args;
    }
    if (resolved.animation_response <= 0.0f)
    {
        resolved.animation_response = 14.0f;
    }
    return resolved;
}

bloom_slider_args bloom_resolve_slider_args(const bloom_slider_args *args, bloom_bool show_grab)
{
    bloom_slider_args resolved = BLOOM_SLIDER_ARGS_DEFAULT;
    if (args)
    {
        resolved = *args;
    }
    if (resolved.animation_response <= 0.0f)
    {
        resolved.animation_response = 16.0f;
    }
    resolved.show_grab = show_grab;
    return resolved;
}

bloom_bool bloom_widget_released_inside(bloom_context *ctx, bloom_id id, bloom_bool hovered)
{
    if (ctx->active_id != id)
    {
        return BLOOM_FALSE;
    }
    if (ctx->input.mouse_down[BLOOM_MOUSE_LEFT])
    {
        return BLOOM_FALSE;
    }

    ctx->active_id = 0;
    return hovered && ctx->input.mouse_released[BLOOM_MOUSE_LEFT];
}
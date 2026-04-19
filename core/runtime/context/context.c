#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "core/runtime/context/context.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <float.h>

#ifdef __cplusplus
extern "C" {
#endif
void bloom_widgets_begin_frame(void);
void bloom_widgets_end_frame(void);
#ifdef __cplusplus
}
#endif

#define BLOOM_LAYER_MAX_VERTICES 8192
#define BLOOM_LAYER_MAX_INDICES  16384
#define BLOOM_LAYER_MAX_CMDS     256

static bloom_context *g_bloom_ctx = NULL;

static bloom_f32 g_next_window_x = -FLT_MAX;
static bloom_f32 g_next_window_y = -FLT_MAX;
static bloom_f32 g_next_window_w = -FLT_MAX;
static bloom_f32 g_next_window_h = -FLT_MAX;
static bloom_i32 g_cascade_index = 0;

static bloom_f32 bloom_context_snap_pixel(bloom_f32 value)
{
    if (value >= 0.0f)
    {
        return (bloom_f32)(bloom_i32)(value + 0.5f);
    }
    return (bloom_f32)(bloom_i32)(value - 0.5f);
}

static bloom_rect bloom_context_snap_rect(bloom_rect rect)
{
    rect.x = bloom_context_snap_pixel(rect.x);
    rect.y = bloom_context_snap_pixel(rect.y);
    rect.w = bloom_context_snap_pixel(rect.w);
    rect.h = bloom_context_snap_pixel(rect.h);

    if (rect.w < 1.0f)
    {
        rect.w = 1.0f;
    }
    if (rect.h < 1.0f)
    {
        rect.h = 1.0f;
    }

    return rect;
}

static bloom_draw_list *bloom_context_layer_draw_list(bloom_context *ctx, bloom_i32 layer)
{
    if (!ctx)
    {
        return NULL;
    }

    switch (layer)
    {
    case BLOOM_DRAW_LAYER_BACKGROUND:
        return &ctx->background_draw_list;
    case BLOOM_DRAW_LAYER_FOREGROUND:
        return &ctx->foreground_draw_list;
    default:
        return &ctx->draw_list;
    }
}

static void bloom_context_push_clip_all_layers(bloom_context *ctx, bloom_rect rect)
{
    if (!ctx)
    {
        return;
    }

    bloom_draw_push_clip(&ctx->draw_list, rect);
    bloom_draw_push_clip(&ctx->background_draw_list, rect);
    bloom_draw_push_clip(&ctx->foreground_draw_list, rect);
}

static void bloom_context_pop_clip_all_layers(bloom_context *ctx)
{
    if (!ctx)
    {
        return;
    }

    bloom_draw_pop_clip(&ctx->draw_list);
    bloom_draw_pop_clip(&ctx->background_draw_list);
    bloom_draw_pop_clip(&ctx->foreground_draw_list);
}

static void bloom_window_note_content_y(bloom_window *win, bloom_f32 y_screen)
{
    bloom_f32 local_y;

    if (!win)
    {
        return;
    }

    local_y = y_screen + win->scroll_y - win->content_rect.y;
    if (local_y < 0.0f)
    {
        local_y = 0.0f;
    }
    if (local_y > win->content_extent_y)
    {
        win->content_extent_y = local_y;
    }
}

static const char *bloom_id_source(const char *str)
{
    const char *scan = str;
    while (*scan)
    {
        if (scan[0] == '#' && scan[1] == '#' && scan[2] == '#')
        {
            return scan + 3;
        }
        scan++;
    }
    return str;
}

bloom_context *bloom_create_context(void)
{
    bloom_context *ctx = (bloom_context *)calloc(1, sizeof(bloom_context));
    if (!ctx)
    {
        return NULL;
    }

    ctx->frame_arena_buffer = (bloom_u8 *)malloc(BLOOM_ARENA_SIZE);
    if (!ctx->frame_arena_buffer)
    {
        free(ctx);
        return NULL;
    }
    bloom_arena_init(&ctx->frame_arena, ctx->frame_arena_buffer, BLOOM_ARENA_SIZE);

    ctx->vertex_buffer = (bloom_vertex *)malloc(sizeof(bloom_vertex) * BLOOM_MAX_VERTICES);
    ctx->index_buffer = (bloom_draw_idx *)malloc(sizeof(bloom_draw_idx) * BLOOM_MAX_INDICES);
    ctx->cmd_buffer = (bloom_draw_cmd *)malloc(sizeof(bloom_draw_cmd) * BLOOM_MAX_DRAW_CMDS);
    ctx->background_vertex_buffer = (bloom_vertex *)malloc(sizeof(bloom_vertex) * BLOOM_LAYER_MAX_VERTICES);
    ctx->background_index_buffer = (bloom_draw_idx *)malloc(sizeof(bloom_draw_idx) * BLOOM_LAYER_MAX_INDICES);
    ctx->background_cmd_buffer = (bloom_draw_cmd *)malloc(sizeof(bloom_draw_cmd) * BLOOM_LAYER_MAX_CMDS);
    ctx->foreground_vertex_buffer = (bloom_vertex *)malloc(sizeof(bloom_vertex) * BLOOM_LAYER_MAX_VERTICES);
    ctx->foreground_index_buffer = (bloom_draw_idx *)malloc(sizeof(bloom_draw_idx) * BLOOM_LAYER_MAX_INDICES);
    ctx->foreground_cmd_buffer = (bloom_draw_cmd *)malloc(sizeof(bloom_draw_cmd) * BLOOM_LAYER_MAX_CMDS);

    if (!ctx->vertex_buffer || !ctx->index_buffer || !ctx->cmd_buffer ||
        !ctx->background_vertex_buffer || !ctx->background_index_buffer || !ctx->background_cmd_buffer ||
        !ctx->foreground_vertex_buffer || !ctx->foreground_index_buffer || !ctx->foreground_cmd_buffer)
    {
        free(ctx->vertex_buffer);
        free(ctx->index_buffer);
        free(ctx->cmd_buffer);
        free(ctx->background_vertex_buffer);
        free(ctx->background_index_buffer);
        free(ctx->background_cmd_buffer);
        free(ctx->foreground_vertex_buffer);
        free(ctx->foreground_index_buffer);
        free(ctx->foreground_cmd_buffer);
        free(ctx->frame_arena_buffer);
        free(ctx);
        return NULL;
    }

    bloom_draw_list_init(&ctx->draw_list,
                         ctx->vertex_buffer, BLOOM_MAX_VERTICES,
                         ctx->index_buffer, BLOOM_MAX_INDICES,
                         ctx->cmd_buffer, BLOOM_MAX_DRAW_CMDS);
    bloom_draw_list_init(&ctx->background_draw_list,
                         ctx->background_vertex_buffer, BLOOM_LAYER_MAX_VERTICES,
                         ctx->background_index_buffer, BLOOM_LAYER_MAX_INDICES,
                         ctx->background_cmd_buffer, BLOOM_LAYER_MAX_CMDS);
    bloom_draw_list_init(&ctx->foreground_draw_list,
                         ctx->foreground_vertex_buffer, BLOOM_LAYER_MAX_VERTICES,
                         ctx->foreground_index_buffer, BLOOM_LAYER_MAX_INDICES,
                         ctx->foreground_cmd_buffer, BLOOM_LAYER_MAX_CMDS);

    bloom_style_default(&ctx->style);

    bloom_font_init(&ctx->default_font);
    bloom_font_build_default(&ctx->default_font, ctx->style.font_size);
    ctx->current_font = &ctx->default_font;
    bloom_font_set_active(ctx->current_font);

    ctx->display_size = bloom_v2(1280, 720);
    ctx->initialized = BLOOM_TRUE;
    ctx->window_order_counter = 0;
    ctx->draw_layer = BLOOM_DRAW_LAYER_WINDOW;
    ctx->dpi_scale = 1.0f;

    if (!g_bloom_ctx)
    {
        g_bloom_ctx = ctx;
    }

    return ctx;
}

void bloom_destroy_context(bloom_context *ctx)
{
    if (!ctx)
    {
        return;
    }
    bloom_font_destroy(&ctx->default_font);
    free(ctx->vertex_buffer);
    free(ctx->index_buffer);
    free(ctx->cmd_buffer);
    free(ctx->background_vertex_buffer);
    free(ctx->background_index_buffer);
    free(ctx->background_cmd_buffer);
    free(ctx->foreground_vertex_buffer);
    free(ctx->foreground_index_buffer);
    free(ctx->foreground_cmd_buffer);
    free(ctx->frame_arena_buffer);
    if (g_bloom_ctx == ctx)
    {
        g_bloom_ctx = NULL;
    }
    free(ctx);
}

bloom_context *bloom_get_context(void)
{
    return g_bloom_ctx;
}

void bloom_set_context(bloom_context *ctx)
{
    g_bloom_ctx = ctx;
}

void bloom_begin_frame(void)
{
    bloom_context *ctx = g_bloom_ctx;
    if (!ctx)
    {
        return;
    }
    bloom_font_set_active(ctx->current_font);

    bloom_arena_reset(&ctx->frame_arena);
    bloom_draw_list_clear(&ctx->draw_list);
    bloom_draw_list_clear(&ctx->background_draw_list);
    bloom_draw_list_clear(&ctx->foreground_draw_list);
    bloom_widgets_begin_frame();

    ctx->draw_list.current_texture = 0;
    ctx->background_draw_list.current_texture = 0;
    ctx->foreground_draw_list.current_texture = 0;
    ctx->current_window = NULL;
    ctx->window_stack_depth = 0;
    ctx->frame_active = BLOOM_TRUE;
    ctx->hot_id = 0;
    ctx->draw_layer = BLOOM_DRAW_LAYER_WINDOW;
    ctx->window_order_counter = 0;
    ctx->skip_depth = 0;
    ctx->frame_count++;
    ctx->time += ctx->delta_time;

    if (ctx->last_click_id != 0 && (ctx->time - ctx->last_click_time) > 0.5)
    {
        ctx->last_click_id = 0;
    }
}

void bloom_end_frame(void)
{
    bloom_context *ctx = g_bloom_ctx;
    if (!ctx)
    {
        return;
    }

    if (ctx->debug_overlay)
    {
        ctx->debug_info.draw_calls = ctx->draw_list.cmd_count;
        ctx->debug_info.vertex_count = ctx->draw_list.vertex_count;
        ctx->debug_info.index_count = ctx->draw_list.index_count;
        ctx->debug_info.window_count = ctx->window_count;
        ctx->debug_info.arena_used = ctx->frame_arena.used;
        ctx->debug_info.arena_total = ctx->frame_arena.size;
        if (ctx->delta_time > 0.0f)
        {
            ctx->debug_info.fps = 1.0f / ctx->delta_time;
        }
        ctx->debug_info.frame_time_ms = ctx->delta_time * 1000.0f;

        char buf[256];
        bloom_f32 dy = ctx->display_size.y - 100;
        bloom_color dbg_bg = bloom_rgba(0, 0, 0, 180);
        bloom_draw_rect_rounded(&ctx->draw_list, bloom_make_rect(4, dy - 4, 260, 100), dbg_bg, 4.0f);

        snprintf(buf, sizeof(buf), "FPS: %.1f (%.2f ms)", ctx->debug_info.fps, ctx->debug_info.frame_time_ms);
        bloom_draw_text(&ctx->draw_list, bloom_v2(10, dy), buf,
                       bloom_rgba(0, 255, 100, 255), 12.0f, ctx->default_font.texture_id);
        dy += 16;
        snprintf(buf, sizeof(buf), "Draw: %u  Verts: %u  Idx: %u",
                ctx->debug_info.draw_calls, ctx->debug_info.vertex_count, ctx->debug_info.index_count);
        bloom_draw_text(&ctx->draw_list, bloom_v2(10, dy), buf,
                       bloom_rgba(0, 255, 100, 255), 12.0f, ctx->default_font.texture_id);
        dy += 16;
        snprintf(buf, sizeof(buf), "Windows: %u  Arena: %lluKB/%lluKB",
                ctx->debug_info.window_count,
                (unsigned long long)ctx->debug_info.arena_used / 1024,
                (unsigned long long)ctx->debug_info.arena_total / 1024);
        bloom_draw_text(&ctx->draw_list, bloom_v2(10, dy), buf,
                       bloom_rgba(0, 255, 100, 255), 12.0f, ctx->default_font.texture_id);
    }

    if (!ctx->input.mouse_down[BLOOM_MOUSE_LEFT])
    {
        ctx->dragging_window = 0;
        ctx->resizing_window = 0;
        ctx->resize_edges = 0;
    }

    bloom_widgets_end_frame();

    bloom_draw_list_prepend(&ctx->draw_list, &ctx->background_draw_list);
    bloom_draw_list_append(&ctx->draw_list, &ctx->foreground_draw_list);

    ctx->debug_info.draw_calls = ctx->draw_list.cmd_count;
    ctx->debug_info.vertex_count = ctx->draw_list.vertex_count;
    ctx->debug_info.index_count = ctx->draw_list.index_count;
    ctx->debug_info.window_count = ctx->window_count;
    ctx->debug_info.arena_used = ctx->frame_arena.used;
    ctx->debug_info.arena_total = ctx->frame_arena.size;
    if (ctx->delta_time > 0.0f)
    {
        ctx->debug_info.fps = 1.0f / ctx->delta_time;
    }
    ctx->debug_info.frame_time_ms = ctx->delta_time * 1000.0f;

    ctx->draw_layer = BLOOM_DRAW_LAYER_WINDOW;
    ctx->frame_active = BLOOM_FALSE;
}

void bloom_set_display_size(bloom_f32 w, bloom_f32 h)
{
    bloom_context *ctx = g_bloom_ctx;
    if (ctx)
    {
        ctx->display_size.x = w;
        ctx->display_size.y = h;
    }
}

void bloom_set_delta_time(bloom_f32 dt)
{
    bloom_context *ctx = g_bloom_ctx;
    if (ctx)
    {
        ctx->delta_time = dt;
    }
}

static bloom_window *bloom_find_window(bloom_context *ctx, bloom_id id)
{
    bloom_i32 i;
    for (i = 0; i < ctx->window_count; i++)
    {
        if (ctx->windows[i].id == id)
        {
            return &ctx->windows[i];
        }
    }
    return NULL;
}

static void bloom_window_begin_layout(bloom_context *ctx, bloom_window *win,
                                      bloom_rect body_rect, bloom_f32 padding,
                                      bloom_bool reserve_scrollbar_gutter)
{
    bloom_f32 available_width = body_rect.w - padding * 2.0f;

    if (reserve_scrollbar_gutter && !(win->flags & BLOOM_WINDOW_NO_SCROLL))
    {
        available_width -= ctx->style.scrollbar_width + ctx->style.scrollbar_inset;
    }
    if (available_width < 0.0f)
    {
        available_width = 0.0f;
    }

    win->content_rect = body_rect;
    win->content_extent_y = 0.0f;
    win->layout.type = BLOOM_LAYOUT_VERTICAL;
    win->layout.cursor = bloom_v2(body_rect.x + padding,
                                  body_rect.y + padding - win->scroll_y);
    win->layout.start = win->layout.cursor;
    win->layout.max_row_height = 0.0f;
    win->layout.available_width = available_width;
    win->layout.indent = 0.0f;
    win->layout.last_item_pos = win->layout.cursor;
    win->layout.last_item_size = bloom_v2(0.0f, 0.0f);
}

static bloom_window *bloom_create_window(bloom_context *ctx, const char *name, bloom_id id)
{
    if (ctx->window_count >= BLOOM_MAX_WINDOWS)
    {
        return NULL;
    }
    bloom_window *win = &ctx->windows[ctx->window_count++];
    memset(win, 0, sizeof(bloom_window));
    win->id = id;
    strncpy(win->name, name, sizeof(win->name) - 1);
    win->rect = bloom_make_rect(60, 60, 400, 300);
    win->active = BLOOM_TRUE;
    win->smooth_scroll = BLOOM_TRUE;
    win->appeared = BLOOM_FALSE;
    win->restore_rect = win->rect;
    return win;
}

static bloom_bool bloom_window_recently_active(const bloom_window *win, bloom_u32 frame_count)
{
    if (!win || !win->appeared)
    {
        return BLOOM_FALSE;
    }

    return win->last_frame_active != 0 && win->last_frame_active + 1 >= frame_count;
}

bloom_bool bloom_window_accepts_input(const bloom_window *win, bloom_vec2 point)
{
    bloom_context *ctx = g_bloom_ctx;
    bloom_i32 i;

    if (!ctx || !win)
    {
        return BLOOM_FALSE;
    }

    for (i = 0; i < ctx->window_count; ++i)
    {
        bloom_window *other = &ctx->windows[i];
        if (other == win)
        {
            continue;
        }
        if (!bloom_window_recently_active(other, ctx->frame_count))
        {
            continue;
        }
        if (other->order <= win->order)
        {
            continue;
        }
        if (bloom_rect_contains(other->rect, point))
        {
            return BLOOM_FALSE;
        }
    }

    return BLOOM_TRUE;
}

static bloom_bool bloom_begin_internal(const char *name, bloom_i32 flags, bloom_bool smooth_scroll);

bloom_bool bloom_begin(const char *name)
{
    return bloom_begin_internal(name, BLOOM_WINDOW_NO_BORDER, BLOOM_TRUE);
}

bloom_bool bloom_begin_window(const char *name, bloom_bool border)
{
    return bloom_begin_internal(name, border ? BLOOM_WINDOW_NONE : BLOOM_WINDOW_NO_BORDER, BLOOM_TRUE);
}

bloom_bool bloom_begin_args(const char *name, const bloom_window_args *args)
{
    bloom_window_args defaults = BLOOM_WINDOW_ARGS_DEFAULT;
    const bloom_window_args *resolved = args ? args : &defaults;
    return bloom_begin_internal(name, resolved->flags, resolved->smooth_scroll);
}

bloom_bool bloom_begin_root(const char *name)
{
    bloom_context *ctx = g_bloom_ctx;
    if (!ctx)
    {
        return BLOOM_FALSE;
    }

    return bloom_begin_root_rect(name,
                                 bloom_make_rect(0.0f, 0.0f, ctx->display_size.x, ctx->display_size.y),
                                 ctx->style.window_padding);
}

bloom_bool bloom_begin_root_rect(const char *name, bloom_rect rect, bloom_f32 padding)
{
    bloom_context *ctx = g_bloom_ctx;
    bloom_window *win;
    bloom_draw_list *dl;

    if (!ctx)
    {
        return BLOOM_FALSE;
    }

    if (ctx->skip_depth > 0)
    {
        ctx->skip_depth++;
        return BLOOM_FALSE;
    }

    if (rect.w <= 0.0f || rect.h <= 0.0f)
    {
        return BLOOM_FALSE;
    }

    if (padding < 0.0f)
    {
        padding = 0.0f;
    }

    win = &ctx->root_window;
    memset(win, 0, sizeof(*win));
    win->id = bloom_hash_str(name ? name : "##bloom_root");
    if (name)
    {
        strncpy(win->name, name, sizeof(win->name) - 1);
    }
    else
    {
        strncpy(win->name, "Root", sizeof(win->name) - 1);
    }
    win->rect = rect;
    win->flags = BLOOM_WINDOW_ROOT | BLOOM_WINDOW_NO_TITLE | BLOOM_WINDOW_NO_MOVE |
                 BLOOM_WINDOW_NO_RESIZE | BLOOM_WINDOW_NO_BORDER | BLOOM_WINDOW_NO_SCROLL;
    win->active = BLOOM_TRUE;
    win->appeared = BLOOM_TRUE;
    win->order = -1;
    win->last_frame_active = ctx->frame_count;
    win->smooth_scroll = BLOOM_FALSE;
    win->scroll_x = 0.0f;
    win->scroll_y = 0.0f;
    win->scroll_target_y = 0.0f;

    dl = bloom_get_draw_list();
    if (!dl)
    {
        return BLOOM_FALSE;
    }

    bloom_context_push_clip_all_layers(ctx, rect);
    bloom_window_begin_layout(ctx, win, rect, padding, BLOOM_FALSE);
    ctx->current_window = win;
    return BLOOM_TRUE;
}

enum
{
    BLOOM_RESIZE_EDGE_NONE = 0,
    BLOOM_RESIZE_EDGE_LEFT = (1 << 0),
    BLOOM_RESIZE_EDGE_RIGHT = (1 << 1),
    BLOOM_RESIZE_EDGE_TOP = (1 << 2),
    BLOOM_RESIZE_EDGE_BOTTOM = (1 << 3),
    BLOOM_TITLE_BUTTON_COLLAPSE = 0,
    BLOOM_TITLE_BUTTON_MINIMIZE = 1,
    BLOOM_TITLE_BUTTON_CLOSE = 2
};

static bloom_color bloom_context_color_mix(bloom_color a, bloom_color b, bloom_f32 t)
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

static bloom_color bloom_context_scale_alpha(bloom_color color, bloom_f32 scale)
{
    if (scale < 0.0f) scale = 0.0f;
    if (scale > 1.0f) scale = 1.0f;
    color.a = (bloom_u8)((bloom_f32)color.a * scale);
    return color;
}

static bloom_i32 bloom_window_resize_mask(const bloom_window *win, bloom_vec2 point, bloom_f32 grip)
{
    bloom_i32 mask = BLOOM_RESIZE_EDGE_NONE;

    if (!win || !bloom_rect_contains(win->rect, point))
    {
        return BLOOM_RESIZE_EDGE_NONE;
    }

    if (point.x <= win->rect.x + grip)
    {
        mask |= BLOOM_RESIZE_EDGE_LEFT;
    }
    else if (point.x >= win->rect.x + win->rect.w - grip)
    {
        mask |= BLOOM_RESIZE_EDGE_RIGHT;
    }

    if (point.y <= win->rect.y + grip)
    {
        mask |= BLOOM_RESIZE_EDGE_TOP;
    }
    else if (point.y >= win->rect.y + win->rect.h - grip)
    {
        mask |= BLOOM_RESIZE_EDGE_BOTTOM;
    }

    return mask;
}

static void bloom_window_apply_resize(bloom_window *win, bloom_rect start_rect,
                                      bloom_vec2 start_mouse, bloom_vec2 mouse_pos,
                                      bloom_i32 edges)
{
    bloom_f32 min_w = 140.0f;
    bloom_f32 min_h = 60.0f;
    bloom_f32 dx = mouse_pos.x - start_mouse.x;
    bloom_f32 dy = mouse_pos.y - start_mouse.y;
    bloom_rect rect = start_rect;

    if (edges & BLOOM_RESIZE_EDGE_LEFT)
    {
        rect.x += dx;
        rect.w -= dx;
    }
    if (edges & BLOOM_RESIZE_EDGE_RIGHT)
    {
        rect.w += dx;
    }
    if (edges & BLOOM_RESIZE_EDGE_TOP)
    {
        rect.y += dy;
        rect.h -= dy;
    }
    if (edges & BLOOM_RESIZE_EDGE_BOTTOM)
    {
        rect.h += dy;
    }

    if (rect.w < min_w)
    {
        if (edges & BLOOM_RESIZE_EDGE_LEFT)
        {
            rect.x = start_rect.x + start_rect.w - min_w;
        }
        rect.w = min_w;
    }
    if (rect.h < min_h)
    {
        if (edges & BLOOM_RESIZE_EDGE_TOP)
        {
            rect.y = start_rect.y + start_rect.h - min_h;
        }
        rect.h = min_h;
    }

    win->rect = rect;
}

static bloom_bool bloom_window_title_button_interaction(bloom_context *ctx, bloom_window *win,
                                                        bloom_rect rect, bloom_id id,
                                                        bloom_bool *hovered_out,
                                                        bloom_bool *held_out)
{
    bloom_bool hovered;
    bloom_bool held;
    bloom_bool clicked = BLOOM_FALSE;

    hovered = bloom_rect_contains(rect, ctx->input.mouse_pos) &&
              bloom_window_accepts_input(win, ctx->input.mouse_pos);
    if (hovered)
    {
        ctx->hot_id = id;
        if (ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT])
        {
            ctx->active_id = id;
        }
    }

    held = ctx->active_id == id && ctx->input.mouse_down[BLOOM_MOUSE_LEFT];
    if (ctx->active_id == id && !ctx->input.mouse_down[BLOOM_MOUSE_LEFT])
    {
        clicked = hovered && ctx->input.mouse_released[BLOOM_MOUSE_LEFT];
        ctx->active_id = 0;
    }

    if (hovered_out)
    {
        *hovered_out = hovered;
    }
    if (held_out)
    {
        *held_out = held;
    }
    return clicked;
}

static void bloom_window_draw_title_button_icon(bloom_draw_list *dl, bloom_rect rect,
                                                bloom_color color, bloom_i32 kind,
                                                bloom_bool toggled)
{
    bloom_f32 cx = rect.x + rect.w * 0.5f;
    bloom_f32 cy = rect.y + rect.h * 0.5f;
    bloom_f32 half = rect.w * 0.22f;

    if (kind == BLOOM_TITLE_BUTTON_MINIMIZE)
    {
        if (toggled)
        {
            bloom_draw_rect(dl,
                bloom_make_rect(cx - half, cy - half, half * 2.0f, half * 2.0f),
                color,
                1.2f);
        }
        else
        {
            bloom_draw_line(dl,
                bloom_v2(cx - half, cy + half * 0.55f),
                bloom_v2(cx + half, cy + half * 0.55f),
                color,
                1.5f);
        }
        return;
    }

    if (kind == BLOOM_TITLE_BUTTON_CLOSE)
    {
        bloom_draw_line(dl, bloom_v2(cx - half, cy - half), bloom_v2(cx + half, cy + half), color, 1.5f);
        bloom_draw_line(dl, bloom_v2(cx + half, cy - half), bloom_v2(cx - half, cy + half), color, 1.5f);
        return;
    }

    if (toggled)
    {
        bloom_draw_line(dl, bloom_v2(cx - half * 0.7f, cy - half), bloom_v2(cx + half * 0.7f, cy), color, 1.5f);
        bloom_draw_line(dl, bloom_v2(cx + half * 0.7f, cy), bloom_v2(cx - half * 0.7f, cy + half), color, 1.5f);
    }
    else
    {
        bloom_draw_line(dl, bloom_v2(cx - half, cy - half * 0.35f), bloom_v2(cx, cy + half * 0.5f), color, 1.5f);
        bloom_draw_line(dl, bloom_v2(cx, cy + half * 0.5f), bloom_v2(cx + half, cy - half * 0.35f), color, 1.5f);
    }
}

static bloom_f32 bloom_window_title_rounding(const bloom_style *style)
{
    if (!style)
    {
        return 0.0f;
    }

    if (style->title_bar_rounding < 0.0f)
    {
        return style->window_rounding;
    }

    return style->title_bar_rounding;
}

static bloom_f32 bloom_window_title_bottom_rounding(const bloom_style *style)
{
    if (!style)
    {
        return 0.0f;
    }

    if (style->title_bar_bottom_rounding < 0.0f)
    {
        return bloom_window_title_rounding(style);
    }

    return style->title_bar_bottom_rounding;
}

static bloom_corner_radii bloom_window_title_radii(const bloom_style *style, bloom_bool header_only)
{
    bloom_f32 top = bloom_window_title_rounding(style);
    bloom_f32 bottom = header_only ? bloom_window_title_bottom_rounding(style) : 0.0f;
    return bloom_make_corner_radii(top, top, bottom, bottom);
}

static void bloom_window_draw_title_fill(bloom_draw_list *dl, bloom_rect rect,
                                         bloom_color color, bloom_corner_radii radii)
{
    bloom_color transparent = bloom_rgba(0, 0, 0, 0);

    if (!dl || color.a == 0 || rect.w <= 0.0f || rect.h <= 0.0f)
    {
        return;
    }

    bloom_draw_rect_custom(dl, rect, color, transparent, 0.0f, radii);
}

static void bloom_window_draw_title_button(bloom_context *ctx, bloom_draw_list *dl,
                                           bloom_style *s, bloom_rect rect, bloom_id id,
                                           bloom_i32 window_flags,
                                           bloom_i32 kind, bloom_bool hovered,
                                           bloom_bool held, bloom_bool toggled)
{
    bloom_bool animations_enabled = !ctx || (window_flags & BLOOM_WINDOW_NO_ANIMATIONS) == 0;
    
    /* ID kind to hopefully prevent bleeds */
    bloom_id hover_id = id ^ 0x7711u ^ (bloom_id)(kind * 0x123);
    bloom_id pulse_id = id ^ 0x7713u ^ (bloom_id)(kind * 0x456);

    bloom_f32 hover_t = animations_enabled ? bloom_anim_toggle(hover_id, hovered || held || toggled, 18.0f)
                                           : ((hovered || held || toggled) ? 1.0f : 0.0f);
    bloom_f32 press_t = animations_enabled ? bloom_anim_pulse(pulse_id, held, 42.0f, 18.0f)
                                           : (held ? 1.0f : 0.0f);
    bloom_color bg = bloom_context_color_mix(s->input_bg, s->input_cursor, toggled ? 0.16f : 0.03f);
    bloom_color border = bloom_context_color_mix(s->input_border, s->input_cursor, toggled ? 0.70f : hover_t * 0.45f + press_t * 0.15f);
    bloom_color icon = bloom_context_color_mix(s->title_text, s->input_cursor, toggled ? 0.82f : hover_t * 0.52f);

    bg = bloom_context_color_mix(bg, s->input_cursor, hover_t * 0.08f + press_t * 0.06f);
    if (hover_t > 0.001f || press_t > 0.001f || toggled)
    {
        bloom_draw_rect_rounded(dl, rect, bloom_context_scale_alpha(bg, 0.98f), rect.h * 0.34f);
        bloom_draw_rect_rounded_border(dl, rect, border, rect.h * 0.34f, 1.0f);
    }

    bloom_window_draw_title_button_icon(dl, rect, icon, kind, toggled);
}

static bloom_bool bloom_begin_internal(const char *name, bloom_i32 flags, bloom_bool smooth_scroll)
{
    bloom_context *ctx = g_bloom_ctx;
    if (!ctx)
    {
        return BLOOM_FALSE;
    }

    if (ctx->skip_depth > 0)
    {
        ctx->skip_depth++;
        return BLOOM_FALSE;
    }

    bloom_id id = bloom_hash_str(name);
    bloom_window *win = bloom_find_window(ctx, id);
    bloom_bool first_frame = BLOOM_FALSE;

    if (!win)
    {
        win = bloom_create_window(ctx, name, id);
        if (!win)
        {
            return BLOOM_FALSE;
        }
        first_frame = BLOOM_TRUE;
    }

    win->flags = flags;
    win->order = ctx->window_order_counter++;
    win->active = BLOOM_TRUE;
    win->smooth_scroll = smooth_scroll;
    win->last_frame_active = ctx->frame_count;

     /* Child windows must always accept position/size from the parent
         (set_next_window_pos/size) because the parent cursor moves each frame.
         Top-level windows only accept it on the first frame (initial placement). */

    {
        bloom_bool apply_pos = (first_frame || !win->appeared || (flags & BLOOM_WINDOW_CHILD));
        if (apply_pos && g_next_window_x > -FLT_MAX && g_next_window_y > -FLT_MAX)
        {
            win->rect.x = g_next_window_x;
            win->rect.y = g_next_window_y;
        }
        else if (first_frame && !(flags & BLOOM_WINDOW_CHILD) &&
                 !(flags & BLOOM_WINDOW_TOOLTIP) && !(flags & BLOOM_WINDOW_POPUP))
        {
            bloom_f32 offset = 30.0f * (bloom_f32)(g_cascade_index % 8);
            win->rect.x = 60.0f + offset;
            win->rect.y = 60.0f + offset;
            g_cascade_index++;
        }
        if (apply_pos && g_next_window_w > -FLT_MAX && g_next_window_h > -FLT_MAX)
        {
            win->rect.w = g_next_window_w;
            win->rect.h = g_next_window_h;
        }
        win->appeared = BLOOM_TRUE;
    }

    g_next_window_x = -FLT_MAX;
    g_next_window_y = -FLT_MAX;
    g_next_window_w = -FLT_MAX;
    g_next_window_h = -FLT_MAX;

    bloom_style *s = &ctx->style;
    bloom_draw_list *dl = &ctx->draw_list;
    bloom_input *inp = &ctx->input;

    bloom_bool has_title_bar = (flags & BLOOM_WINDOW_NO_TITLE) == 0;
    bloom_bool allow_collapse = has_title_bar && !(flags & BLOOM_WINDOW_NO_COLLAPSE) &&
                                !(flags & (BLOOM_WINDOW_TOOLTIP | BLOOM_WINDOW_POPUP | BLOOM_WINDOW_CHILD | BLOOM_WINDOW_ROOT));
    bloom_bool allow_minimize = has_title_bar && !(flags & BLOOM_WINDOW_NO_MINIMIZE) &&
                                !(flags & (BLOOM_WINDOW_TOOLTIP | BLOOM_WINDOW_POPUP | BLOOM_WINDOW_CHILD | BLOOM_WINDOW_ROOT));
    bloom_bool allow_close = has_title_bar && !(flags & BLOOM_WINDOW_NO_CLOSE) &&
                             !(flags & (BLOOM_WINDOW_TOOLTIP | BLOOM_WINDOW_POPUP | BLOOM_WINDOW_CHILD | BLOOM_WINDOW_ROOT));
    bloom_bool collapse_hovered = BLOOM_FALSE;
    bloom_bool collapse_held = BLOOM_FALSE;
    bloom_bool minimize_hovered = BLOOM_FALSE;
    bloom_bool minimize_held = BLOOM_FALSE;
    bloom_bool close_hovered = BLOOM_FALSE;
    bloom_bool close_held = BLOOM_FALSE;
    bloom_bool collapse_clicked = BLOOM_FALSE;
    bloom_bool minimize_clicked = BLOOM_FALSE;
    bloom_bool close_clicked = BLOOM_FALSE;
    bloom_bool controls_hovered = BLOOM_FALSE;
    bloom_bool window_hidden;
    bloom_corner_radii header_only_radii = bloom_make_corner_radii(0.0f, 0.0f, 0.0f, 0.0f);
    bloom_f32 title_h = has_title_bar ? s->title_bar_height : 0.0f;
    bloom_f32 control_size = title_h > 14.0f ? title_h - 12.0f : 0.0f;
    bloom_f32 controls_width = 0.0f;
    bloom_f32 title_text_w;
    bloom_f32 min_window_w;
    bloom_rect title_rect = bloom_make_rect(win->rect.x, win->rect.y, win->rect.w, title_h);
    bloom_rect body_rect = bloom_make_rect(win->rect.x, win->rect.y + title_h,
                                           win->rect.w, win->rect.h - title_h);
    bloom_rect title_drag_rect = title_rect;
    bloom_rect collapse_rect = bloom_make_rect(0.0f, 0.0f, 0.0f, 0.0f);
    bloom_rect minimize_rect = bloom_make_rect(0.0f, 0.0f, 0.0f, 0.0f);
    bloom_rect close_rect = bloom_make_rect(0.0f, 0.0f, 0.0f, 0.0f);
    bloom_i32 resize_mask = BLOOM_RESIZE_EDGE_NONE;

    if (ctx->dragging_window == id && inp->mouse_down[BLOOM_MOUSE_LEFT])
    {
        if (!ctx->drag_committed)
        {
            bloom_f32 dx = inp->mouse_pos.x - ctx->drag_start_mouse.x;
            bloom_f32 dy = inp->mouse_pos.y - ctx->drag_start_mouse.y;
            if (dx * dx + dy * dy >= 16.0f) /* 4px threshold */
                ctx->drag_committed = BLOOM_TRUE;
        }
        if (ctx->drag_committed)
        {
            win->rect.x = inp->mouse_pos.x - ctx->drag_offset.x;
            win->rect.y = inp->mouse_pos.y - ctx->drag_offset.y;
        }
    }
    else if (ctx->resizing_window == id && inp->mouse_down[BLOOM_MOUSE_LEFT])
    {
        bloom_window_apply_resize(win, ctx->resize_start_rect, ctx->resize_start_mouse,
                                  inp->mouse_pos, ctx->resize_edges);
    }

    title_rect = bloom_make_rect(win->rect.x, win->rect.y, win->rect.w, title_h);
    body_rect = bloom_make_rect(win->rect.x, win->rect.y + title_h,
                                win->rect.w, win->rect.h - title_h);

    if (allow_close)
    {
        close_rect = bloom_make_rect(title_rect.x + title_rect.w - controls_width - control_size - 8.0f,
                                     title_rect.y + (title_h - control_size) * 0.5f,
                                     control_size,
                                     control_size);
        controls_width += control_size + 6.0f;
        close_clicked = bloom_window_title_button_interaction(ctx, win, close_rect,
                                                              id ^ 0xB103u,
                                                              &close_hovered,
                                                              &close_held);
    }
    if (allow_minimize)
    {
        minimize_rect = bloom_make_rect(title_rect.x + title_rect.w - controls_width - control_size - 8.0f,
                                        title_rect.y + (title_h - control_size) * 0.5f,
                                        control_size,
                                        control_size);
        controls_width += control_size + 6.0f;
        minimize_clicked = bloom_window_title_button_interaction(ctx, win, minimize_rect,
                                                                  id ^ 0xB102u,
                                                                  &minimize_hovered,
                                                                  &minimize_held);
    }
    if (allow_collapse && !win->minimized)
    {
        collapse_rect = bloom_make_rect(title_rect.x + title_rect.w - controls_width - control_size - 8.0f,
                                        title_rect.y + (title_h - control_size) * 0.5f,
                                        control_size,
                                        control_size);
        controls_width += control_size + 6.0f;
        collapse_clicked = bloom_window_title_button_interaction(ctx, win, collapse_rect,
                                                                  id ^ 0xB101u,
                                                                  &collapse_hovered,
                                                                  &collapse_held);
    }

    controls_hovered = collapse_hovered || minimize_hovered || close_hovered;

    if (close_clicked)
    {
        win->closed = BLOOM_TRUE;
    }

    if (minimize_clicked)
    {
        if (!win->minimized)
        {
            win->restore_rect = win->rect;
            win->minimized = BLOOM_TRUE;
            win->collapsed = BLOOM_FALSE;
            title_text_w = bloom_text_width(name, s->font_size);
            min_window_w = title_text_w + s->window_padding * 2.0f + controls_width + 28.0f;
            if (min_window_w < 180.0f) min_window_w = 180.0f;
            if (win->rect.w > min_window_w) win->rect.w = min_window_w;
            if (win->rect.h > title_h) win->rect.h = title_h;
        }
        else
        {
            win->minimized = BLOOM_FALSE;
            if (win->restore_rect.w > 0.0f && win->restore_rect.h > 0.0f)
            {
                win->rect = win->restore_rect;
            }
        }
    }

    if (collapse_clicked)
    {
        if (!win->collapsed)
        {
            win->restore_rect = win->rect;
            win->collapsed = BLOOM_TRUE;
            if (win->rect.h > title_h) win->rect.h = title_h;
        }
        else
        {
            win->collapsed = BLOOM_FALSE;
            if (win->restore_rect.h > title_h)
            {
                win->rect.w = win->restore_rect.w;
                win->rect.h = win->restore_rect.h;
            }
        }
    }

    window_hidden = win->collapsed || win->minimized;

    if (win->closed)
    {
        ctx->current_window = NULL;
        ctx->skip_depth = 1;
        return BLOOM_FALSE;
    }

    if (has_title_bar)
    {
        header_only_radii = bloom_window_title_radii(s, BLOOM_TRUE);
    }

    title_rect = bloom_make_rect(win->rect.x, win->rect.y, win->rect.w, title_h);
    body_rect = bloom_make_rect(win->rect.x, win->rect.y + title_h,
                                win->rect.w, win->rect.h - title_h);
    title_drag_rect = title_rect;
    if (body_rect.h < 0.0f)
    {
        body_rect.h = 0.0f;
    }

    controls_width = 0.0f;
    if (allow_close)
    {
        close_rect = bloom_make_rect(title_rect.x + title_rect.w - controls_width - control_size - 8.0f,
                                     title_rect.y + (title_h - control_size) * 0.5f,
                                     control_size,
                                     control_size);
        controls_width += control_size + 6.0f;
    }
    if (allow_minimize)
    {
        minimize_rect = bloom_make_rect(title_rect.x + title_rect.w - controls_width - control_size - 8.0f,
                                        title_rect.y + (title_h - control_size) * 0.5f,
                                        control_size,
                                        control_size);
        controls_width += control_size + 6.0f;
    }
    if (allow_collapse && !win->minimized)
    {
        collapse_rect = bloom_make_rect(title_rect.x + title_rect.w - controls_width - control_size - 8.0f,
                                        title_rect.y + (title_h - control_size) * 0.5f,
                                        control_size,
                                        control_size);
        controls_width += control_size + 6.0f;
    }
    if (controls_width > 0.0f)
    {
        title_drag_rect.w -= controls_width + 8.0f;
    }

    if (!(flags & BLOOM_WINDOW_NO_MOVE) && !(flags & BLOOM_WINDOW_HOST_MOVE))
    {
        bloom_rect drag_zone = has_title_bar ? title_drag_rect : win->rect;
        if (inp->mouse_pressed[BLOOM_MOUSE_LEFT] && bloom_rect_contains(drag_zone, inp->mouse_pos) &&
            !controls_hovered &&
            bloom_window_accepts_input(win, inp->mouse_pos))
        {
            ctx->dragging_window = id;
            ctx->drag_committed = BLOOM_FALSE;
            ctx->drag_start_mouse = inp->mouse_pos;
            ctx->drag_offset.x = inp->mouse_pos.x - win->rect.x;
            ctx->drag_offset.y = inp->mouse_pos.y - win->rect.y;
        }
    }

    if (!(flags & BLOOM_WINDOW_NO_RESIZE) && !window_hidden)
    {
        resize_mask = bloom_window_resize_mask(win, inp->mouse_pos, 8.0f);
        if (inp->mouse_pressed[BLOOM_MOUSE_LEFT] && resize_mask != BLOOM_RESIZE_EDGE_NONE &&
            bloom_window_accepts_input(win, inp->mouse_pos))
        {
            ctx->resizing_window = id;
            ctx->resize_edges = resize_mask;
            ctx->resize_start_mouse = inp->mouse_pos;
            ctx->resize_start_rect = win->rect;
        }
    }

    if (!window_hidden)
    {
        win->restore_rect = win->rect;
    }

    if (!(flags & BLOOM_WINDOW_NO_BACKGROUND) && s->shadow_offset > 0)
    {
        bloom_color shadow_col = s->shadow;
        bloom_color transparent = bloom_rgba(0, 0, 0, 0);
        bloom_f32 r = (flags & BLOOM_WINDOW_CHILD) ? s->child_rounding : s->window_rounding;
        shadow_col.a = (bloom_u8)(s->shadow_alpha * 255);
        if (window_hidden && has_title_bar)
        {
            bloom_draw_rect_custom(dl,
                bloom_make_rect(win->rect.x + s->shadow_offset, win->rect.y + s->shadow_offset,
                                win->rect.w, win->rect.h),
                shadow_col, transparent, 0.0f, header_only_radii);
        }
        else
        {
            bloom_draw_rect_rounded(dl,
                bloom_make_rect(win->rect.x + s->shadow_offset, win->rect.y + s->shadow_offset,
                               win->rect.w, win->rect.h),
                shadow_col, r);
        }
    }

    if (!(flags & BLOOM_WINDOW_NO_BACKGROUND))
    {
        bloom_color transparent = bloom_rgba(0, 0, 0, 0);
        bloom_f32 r = (flags & BLOOM_WINDOW_CHILD) ? s->child_rounding : s->window_rounding;
        if (window_hidden && has_title_bar)
        {
            bloom_draw_rect_custom(dl, win->rect, s->window_bg, transparent, 0.0f, header_only_radii);
        }
        else
        {
            bloom_draw_rect_rounded(dl, win->rect, s->window_bg, r);
        }
    }

    if (!(flags & BLOOM_WINDOW_NO_BORDER))
    {
        bloom_color transparent = bloom_rgba(0, 0, 0, 0);
        bloom_f32 r = (flags & BLOOM_WINDOW_CHILD) ? s->child_rounding : s->window_rounding;
        if (window_hidden && has_title_bar)
        {
            bloom_draw_rect_custom(dl, win->rect, transparent, s->window_border, s->border_width, header_only_radii);
        }
        else
        {
            bloom_draw_rect_rounded_border(dl, win->rect, s->window_border, r, s->border_width);
        }
    }

    if (has_title_bar)
    {
        bloom_bool is_focused = (ctx->dragging_window == id || ctx->resizing_window == id ||
                                 bloom_rect_contains(win->rect, inp->mouse_pos));
        bloom_color title_bg = is_focused ? s->title_bg_active : s->title_bg;
        bloom_color title_col = is_focused ? s->title_text : bloom_context_color_mix(s->title_text, s->text_disabled, 0.35f);
        bloom_corner_radii title_radii = bloom_window_title_radii(s, window_hidden);
        bloom_f32 scaled_line_h = s->font_size;
        bloom_f32 text_right = title_rect.x + title_rect.w - controls_width - s->window_padding - 4.0f;
        bloom_rect accent_rect;

        if (ctx->default_font.valid && ctx->default_font.size > 0.0f)
        {
            scaled_line_h = ctx->default_font.line_height * (s->font_size / ctx->default_font.size);
        }

        bloom_window_draw_title_fill(dl, title_rect, title_bg, title_radii);
    accent_rect = bloom_make_rect(title_rect.x + s->window_padding,
                      title_rect.y + (title_h - 18.0f) * 0.5f,
                      4.0f,
                      18.0f);
    bloom_draw_rect_rounded(dl,
                accent_rect,
                bloom_context_scale_alpha(s->input_cursor, is_focused ? 0.95f : 0.32f),
                accent_rect.w * 0.5f);

    bloom_f32 text_x = accent_rect.x + accent_rect.w + 12.0f;
        bloom_f32 text_y = title_rect.y + (title_h - scaled_line_h) * 0.5f;
        if (text_x < text_right)
        {
        bloom_draw_text(dl, bloom_v2(text_x, text_y), name, title_col, s->font_size,
                       ctx->default_font.texture_id);
        }
        if (!window_hidden)
        {
            bloom_draw_rect_filled(dl,
                bloom_make_rect(win->rect.x + s->window_padding,
                                win->rect.y + title_h - 1.0f,
                                win->rect.w - s->window_padding * 2.0f,
                                1.0f),
                bloom_context_scale_alpha(s->separator, 0.95f));
        }

        if (allow_collapse && !win->minimized)
        {
            bloom_window_draw_title_button(ctx, dl, s, collapse_rect, id ^ 0xB101u,
                                           flags,
                                           BLOOM_TITLE_BUTTON_COLLAPSE,
                                           collapse_hovered, collapse_held, win->collapsed);
        }
        if (allow_minimize)
        {
            bloom_window_draw_title_button(ctx, dl, s, minimize_rect, id ^ 0xB102u,
                                           flags,
                                           BLOOM_TITLE_BUTTON_MINIMIZE,
                                           minimize_hovered, minimize_held, win->minimized);
        }
        if (allow_close)
        {
            bloom_window_draw_title_button(ctx, dl, s, close_rect, id ^ 0xB103u,
                                           flags,
                                           BLOOM_TITLE_BUTTON_CLOSE,
                                           close_hovered, close_held, win->closed);
        }
    }

    if (window_hidden)
    {
        ctx->current_window = NULL;
        ctx->skip_depth = 1;
        return BLOOM_FALSE;
    }

    bloom_context_push_clip_all_layers(ctx, body_rect);

    bloom_window_begin_layout(ctx, win, body_rect, s->window_padding, BLOOM_TRUE);

    if (ctx->current_window && ctx->window_stack_depth < 16)
    {
        ctx->window_stack[ctx->window_stack_depth++] = ctx->current_window;
    }
    ctx->current_window = win;
    return BLOOM_TRUE;
}

bloom_bool bloom_begin_ex(const char *name, bloom_i32 flags)
{
    return bloom_begin_internal(name, flags, BLOOM_TRUE);
}

void bloom_end(void)
{
    bloom_context *ctx = g_bloom_ctx;
    if (!ctx)
    {
        return;
    }

    if (!ctx->current_window)
    {
        if (ctx->skip_depth > 0)
        {
            ctx->skip_depth--;
        }
        return;
    }

    bloom_window *win = ctx->current_window;
    bloom_style *s = &ctx->style;
    bloom_draw_list *dl = &ctx->draw_list;
    bloom_input *inp = &ctx->input;

    if (!dl)
    {
        ctx->current_window = (ctx->window_stack_depth > 0)
            ? ctx->window_stack[--ctx->window_stack_depth] : NULL;
        return;
    }

    bloom_context_pop_clip_all_layers(ctx);

    {
        bloom_f32 content_bottom = 0.0f;
        bloom_f32 fallback_bottom = win->content_extent_y;
        if (win->layout.last_item_size.y > 0.0f || win->layout.max_row_height > 0.0f)
        {
            content_bottom = win->layout.last_item_pos.y + win->layout.last_item_size.y;
            if (win->layout.max_row_height > win->layout.last_item_size.y)
            {
                content_bottom = win->layout.last_item_pos.y + win->layout.max_row_height;
            }
            bloom_window_note_content_y(win, content_bottom);
            fallback_bottom = win->layout.last_item_pos.y + win->scroll_y - win->content_rect.y +
                              (win->layout.max_row_height > win->layout.last_item_size.y ? win->layout.max_row_height
                                                                                         : win->layout.last_item_size.y);
        }

        if (fallback_bottom < 0.0f)
        {
            fallback_bottom = 0.0f;
        }

        win->content_height = fallback_bottom + s->window_padding;
        if (win->content_height < win->content_rect.h)
        {
            win->content_height = win->content_rect.h;
        }
    }

    if (win->flags & BLOOM_WINDOW_ROOT)
    {
        ctx->current_window = (ctx->window_stack_depth > 0)
            ? ctx->window_stack[--ctx->window_stack_depth] : NULL;
        return;
    }

    if (!(win->flags & BLOOM_WINDOW_NO_SCROLL))
    {
        bloom_f32 max_scroll = win->content_height - win->content_rect.h;
        if (max_scroll < 0.0f)
        {
            max_scroll = 0.0f;
        }

        if (win->scroll_target_y < 0.0f)
        {
            win->scroll_target_y = 0.0f;
        }
        if (win->scroll_target_y > max_scroll)
        {
            win->scroll_target_y = max_scroll;
        }

        if (max_scroll <= 0.0f)
        {
            win->scroll_y = 0.0f;
            win->scroll_target_y = 0.0f;
            bloom_anim_reset(win->id ^ 0x6BC3A511u);
        }
        else
        {
            bloom_f32 corner_clear = s->scrollbar_inset;
            bloom_f32 sb_top_inset = s->scrollbar_inset;
            bloom_f32 sb_bottom_inset = s->scrollbar_inset;
            bloom_f32 sb_right_inset = s->scrollbar_inset;
            bloom_f32 sb_x;
            bloom_f32 sb_y;
            bloom_f32 sb_h;
            bloom_f32 ratio;
            bloom_f32 grab_h;
            bloom_f32 scroll_ratio;
            bloom_f32 grab_y;
            bloom_id scrollbar_id = win->id ^ 0x6BC3A511u;
            bloom_rect scrollbar_clip_rect;
            bloom_rect track_rect;
            bloom_rect grab_rect;
            bloom_rect inner_grab_rect;
            bloom_bool window_accepts_pointer = bloom_window_accepts_input(win, inp->mouse_pos);
            bloom_bool track_hovered;
            bloom_bool grab_hovered;

            if (s->window_rounding > 0.5f)
            {
                corner_clear = s->window_rounding * 0.4f;
                if (corner_clear < s->scrollbar_inset)
                {
                    corner_clear = s->scrollbar_inset;
                }

                sb_right_inset = corner_clear;
                sb_bottom_inset = corner_clear;

                if (win->content_rect.y <= win->rect.y + 0.5f)
                {
                    sb_top_inset = corner_clear;
                }
            }

            sb_x = win->content_rect.x + win->content_rect.w - s->scrollbar_width - sb_right_inset;
            sb_y = win->content_rect.y + sb_top_inset;
            sb_h = win->content_rect.h - sb_top_inset - sb_bottom_inset;

            if (sb_h < 1.0f)
            {
                sb_h = 1.0f;
            }

            if (win->smooth_scroll && (win->flags & BLOOM_WINDOW_NO_ANIMATIONS) == 0 && ctx->active_id != scrollbar_id)
            {
                win->scroll_y = bloom_anim_state(scrollbar_id, win->scroll_target_y, 17.0f);
            }
            else
            {
                win->scroll_y = win->scroll_target_y;
            }

            if (win->scroll_y < 0.0f)
            {
                win->scroll_y = 0.0f;
            }
            if (win->scroll_y > max_scroll)
            {
                win->scroll_y = max_scroll;
            }

            ratio = sb_h / win->content_height;
            if (ratio > 1.0f)
            {
                ratio = 1.0f;
            }
            grab_h = sb_h * ratio;
            if (grab_h < 20.0f)
            {
                grab_h = 20.0f;
            }

            scroll_ratio = (max_scroll > 0.0f) ? (win->scroll_y / max_scroll) : 0.0f;
            grab_y = sb_y + scroll_ratio * (sb_h - grab_h);
            scrollbar_clip_rect = bloom_context_snap_rect(bloom_make_rect(sb_x, sb_y, s->scrollbar_width, sb_h));
            track_rect = scrollbar_clip_rect;
            grab_rect = bloom_context_snap_rect(bloom_make_rect(sb_x, grab_y, s->scrollbar_width, grab_h));
            if (grab_rect.h > track_rect.h)
            {
                grab_rect.h = track_rect.h;
            }
            if (grab_rect.y < track_rect.y)
            {
                grab_rect.y = track_rect.y;
            }
            if (grab_rect.y + grab_rect.h > track_rect.y + track_rect.h)
            {
                grab_rect.y = track_rect.y + track_rect.h - grab_rect.h;
            }
            inner_grab_rect = bloom_context_snap_rect(
                bloom_make_rect(grab_rect.x + 2.0f, grab_rect.y, grab_rect.w - 4.0f, grab_rect.h));
            track_hovered = window_accepts_pointer && bloom_rect_contains(track_rect, inp->mouse_pos);
            grab_hovered = window_accepts_pointer && bloom_rect_contains(grab_rect, inp->mouse_pos);

            bloom_draw_push_clip(dl, scrollbar_clip_rect);
            bloom_draw_rect_rounded(dl, track_rect, s->scrollbar_bg, s->scrollbar_rounding);
            bloom_draw_rect_rounded_border(dl, track_rect,
                bloom_context_scale_alpha(s->window_border, 0.65f),
                s->scrollbar_rounding,
                1.0f);

            if (inp->mouse_pressed[BLOOM_MOUSE_LEFT] && track_hovered)
            {
                ctx->active_id = scrollbar_id;
                if (grab_hovered)
                {
                    ctx->drag_offset.y = inp->mouse_pos.y - grab_rect.y;
                }
                else
                {
                    ctx->drag_offset.y = grab_h * 0.5f;
                }
            }

            if (ctx->active_id == scrollbar_id)
            {
                if (inp->mouse_down[BLOOM_MOUSE_LEFT])
                {
                    bloom_f32 new_grab_y = inp->mouse_pos.y - ctx->drag_offset.y;
                    if (new_grab_y < sb_y) new_grab_y = sb_y;
                    if (new_grab_y > sb_y + sb_h - grab_h) new_grab_y = sb_y + sb_h - grab_h;
                    scroll_ratio = (sb_h - grab_h) > 0.0f ? ((new_grab_y - sb_y) / (sb_h - grab_h)) : 0.0f;
                    win->scroll_y = scroll_ratio * max_scroll;
                    win->scroll_target_y = win->scroll_y;
                    grab_y = bloom_context_snap_pixel(new_grab_y);
                    grab_rect.y = grab_y;
                    inner_grab_rect.y = grab_y;
                }
                else
                {
                    ctx->active_id = 0;
                }
            }

            bloom_draw_rect_rounded(dl,
                inner_grab_rect,
                (grab_hovered || ctx->active_id == scrollbar_id) ? s->scrollbar_grab_hovered : s->scrollbar_grab,
                s->scrollbar_rounding);
            bloom_draw_rect_rounded_border(dl,
                inner_grab_rect,
                bloom_context_scale_alpha(s->window_bg, 0.55f),
                s->scrollbar_rounding,
                1.0f);
            bloom_draw_pop_clip(dl);

            if (window_accepts_pointer && bloom_rect_contains(win->content_rect, inp->mouse_pos) && inp->mouse_wheel != 0.0f)
            {
                bloom_f32 scroll_step = win->content_rect.h * 0.18f;
                if (scroll_step < 28.0f)
                {
                    scroll_step = 28.0f;
                }
                win->scroll_target_y -= inp->mouse_wheel * scroll_step;
                if (win->scroll_target_y < 0.0f) win->scroll_target_y = 0.0f;
                if (win->scroll_target_y > max_scroll) win->scroll_target_y = max_scroll;
                if (!win->smooth_scroll)
                {
                    win->scroll_y = win->scroll_target_y;
                }
                inp->mouse_wheel = 0.0f;
            }
        }
    }

    ctx->current_window = (ctx->window_stack_depth > 0)
        ? ctx->window_stack[--ctx->window_stack_depth] : NULL;
}

bloom_bool bloom_begin_child(const char *name, bloom_f32 w, bloom_f32 h)
{
    return bloom_begin_child_args(name, w, h, NULL);
}

bloom_bool bloom_begin_child_args(const char *name, bloom_f32 w, bloom_f32 h, const bloom_window_args *args)
{
    bloom_context *ctx = g_bloom_ctx;
    bloom_window_args defaults = BLOOM_WINDOW_ARGS_DEFAULT;
    const bloom_window_args *resolved = args ? args : &defaults;
    if (!ctx || !ctx->current_window)
    {
        return BLOOM_FALSE;
    }

    bloom_window *parent = ctx->current_window;
    bloom_vec2 pos = parent->layout.cursor;

    if (w <= 0) w = parent->layout.available_width;
    if (h <= 0)
    {
        bloom_f32 bottom = parent->content_rect.y + parent->content_rect.h;
        h = bottom - pos.y;
        if (h < 1.0f) h = 1.0f;
    }

    bloom_set_next_window_pos(pos.x, pos.y);
    bloom_set_next_window_size(w, h);

    parent->layout.last_item_pos = pos;
    parent->layout.last_item_size = bloom_v2(w, h);
    parent->layout.cursor.y += h + ctx->style.item_spacing;

    return bloom_begin_internal(name,
                                resolved->flags | BLOOM_WINDOW_NO_TITLE | BLOOM_WINDOW_NO_MOVE |
                                BLOOM_WINDOW_NO_RESIZE | BLOOM_WINDOW_NO_BORDER | BLOOM_WINDOW_CHILD,
                                resolved->smooth_scroll);
}

void bloom_end_child(void)
{
    bloom_end();
}

bloom_id bloom_get_id(const char *str)
{
    bloom_context *ctx = g_bloom_ctx;
    bloom_id seed = BLOOM_HASH_SEED;
    const char *id_str = bloom_id_source(str);
    if (ctx && ctx->id_stack_depth > 0)
    {
        seed = ctx->id_stack[ctx->id_stack_depth - 1];
    }
    return bloom_hash_str_seed(id_str, seed);
}

void bloom_push_id(const char *str)
{
    bloom_context *ctx = g_bloom_ctx;
    if (!ctx || ctx->id_stack_depth >= BLOOM_MAX_ID_STACK)
    {
        return;
    }
    ctx->id_stack[ctx->id_stack_depth++] = bloom_get_id(str);
}

void bloom_push_id_int(bloom_i32 n)
{
    bloom_context *ctx = g_bloom_ctx;
    if (!ctx || ctx->id_stack_depth >= BLOOM_MAX_ID_STACK)
    {
        return;
    }
    bloom_id seed = BLOOM_HASH_SEED;
    if (ctx->id_stack_depth > 0)
    {
        seed = ctx->id_stack[ctx->id_stack_depth - 1];
    }
    ctx->id_stack[ctx->id_stack_depth++] = bloom_hash_bytes(&n, sizeof(n), seed);
}

void bloom_push_id_ptr(const void *ptr)
{
    bloom_context *ctx = g_bloom_ctx;
    if (!ctx || ctx->id_stack_depth >= BLOOM_MAX_ID_STACK)
    {
        return;
    }
    bloom_id seed = BLOOM_HASH_SEED;
    if (ctx->id_stack_depth > 0)
    {
        seed = ctx->id_stack[ctx->id_stack_depth - 1];
    }
    ctx->id_stack[ctx->id_stack_depth++] = bloom_hash_bytes(&ptr, sizeof(ptr), seed);
}

void bloom_pop_id(void)
{
    bloom_context *ctx = g_bloom_ctx;
    if (ctx && ctx->id_stack_depth > 0)
    {
        ctx->id_stack_depth--;
    }
}

void bloom_set_next_window_pos(bloom_f32 x, bloom_f32 y)
{
    g_next_window_x = x;
    g_next_window_y = y;
}

void bloom_set_next_window_size(bloom_f32 w, bloom_f32 h)
{
    g_next_window_w = w;
    g_next_window_h = h;
}

bloom_vec2 bloom_get_cursor_pos(void)
{
    bloom_context *ctx = g_bloom_ctx;
    if (ctx && ctx->current_window)
    {
        return ctx->current_window->layout.cursor;
    }
    return bloom_v2(0, 0);
}

void bloom_set_cursor_pos(bloom_f32 x, bloom_f32 y)
{
    bloom_context *ctx = g_bloom_ctx;
    if (ctx && ctx->current_window)
    {
        ctx->current_window->layout.cursor.x = x;
        ctx->current_window->layout.cursor.y = y;
    }
}

bloom_f32 bloom_get_content_width(void)
{
    bloom_context *ctx = g_bloom_ctx;
    if (ctx && ctx->current_window)
    {
        return ctx->current_window->layout.available_width;
    }
    return 0;
}

void bloom_same_line(void)
{
    bloom_context *ctx = g_bloom_ctx;
    if (!ctx || !ctx->current_window)
    {
        return;
    }
    bloom_window *win = ctx->current_window;
    if (win->layout.last_item_size.x <= 0.0f && win->layout.last_item_size.y <= 0.0f)
    {
        return;
    }
    win->layout.cursor.x = win->layout.last_item_pos.x + win->layout.last_item_size.x + ctx->style.item_inner_spacing;
    win->layout.cursor.y = win->layout.last_item_pos.y;
    if (win->layout.max_row_height < win->layout.last_item_size.y)
    {
        win->layout.max_row_height = win->layout.last_item_size.y;
    }
    win->layout.type = BLOOM_LAYOUT_HORIZONTAL;
}

void bloom_new_line(void)
{
    bloom_context *ctx = g_bloom_ctx;
    if (!ctx || !ctx->current_window)
    {
        return;
    }
    bloom_window *win = ctx->current_window;
    win->layout.cursor.x = win->layout.start.x + win->layout.indent;
    if (win->layout.max_row_height > 0.0f)
    {
        win->layout.cursor.y = win->layout.last_item_pos.y + win->layout.max_row_height + ctx->style.item_spacing;
    }
    win->layout.max_row_height = 0;
    win->layout.type = BLOOM_LAYOUT_VERTICAL;
}

void bloom_begin_flow(void)
{
    bloom_context *ctx = g_bloom_ctx;
    if (!ctx || !ctx->current_window)
    {
        return;
    }
    ctx->current_window->layout.type = BLOOM_LAYOUT_FLOW;
    ctx->current_window->layout.max_row_height = 0.0f;
}

void bloom_end_flow(void)
{
    bloom_context *ctx = g_bloom_ctx;
    bloom_window *win;
    if (!ctx || !ctx->current_window)
    {
        return;
    }
    win = ctx->current_window;
    if (win->layout.max_row_height > 0.0f)
    {
        win->layout.cursor.x = win->layout.start.x + win->layout.indent;
        win->layout.cursor.y += win->layout.max_row_height + ctx->style.item_spacing;
        win->layout.max_row_height = 0.0f;
    }
    win->layout.type = BLOOM_LAYOUT_VERTICAL;
}

void bloom_indent(bloom_f32 amount)
{
    bloom_context *ctx = g_bloom_ctx;
    if (!ctx || !ctx->current_window)
    {
        return;
    }
    if (amount <= 0) amount = ctx->style.indent_spacing;
    ctx->current_window->layout.indent += amount;
    ctx->current_window->layout.cursor.x += amount;
    ctx->current_window->layout.available_width -= amount;
}

void bloom_unindent(bloom_f32 amount)
{
    bloom_context *ctx = g_bloom_ctx;
    if (!ctx || !ctx->current_window)
    {
        return;
    }
    if (amount <= 0) amount = ctx->style.indent_spacing;
    ctx->current_window->layout.indent -= amount;
    ctx->current_window->layout.cursor.x -= amount;
    ctx->current_window->layout.available_width += amount;
}

void bloom_separator(void)
{
    bloom_context *ctx = g_bloom_ctx;
    if (!ctx || !ctx->current_window)
    {
        return;
    }
    bloom_window *win = ctx->current_window;
    if (win->layout.max_row_height > 0.0f)
    {
        win->layout.cursor.x = win->layout.start.x + win->layout.indent;
        win->layout.cursor.y = win->layout.last_item_pos.y + win->layout.max_row_height;
        win->layout.max_row_height = 0.0f;
        win->layout.type = BLOOM_LAYOUT_VERTICAL;
    }
    bloom_f32 y = win->layout.cursor.y + ctx->style.item_spacing * 0.5f;
    bloom_f32 x0 = win->layout.cursor.x;
    bloom_f32 x1 = x0 + win->layout.available_width;
    bloom_draw_line(&ctx->draw_list,
                    bloom_v2(x0, y),
                    bloom_v2(x1, y),
                    ctx->style.separator, 1.0f);
    bloom_window_note_content_y(win, y + 1.0f);
    win->layout.cursor.x = win->layout.start.x + win->layout.indent;
    win->layout.cursor.y += ctx->style.item_spacing;
}

void bloom_spacing(void)
{
    bloom_context *ctx = g_bloom_ctx;
    if (ctx && ctx->current_window)
    {
        bloom_window *win = ctx->current_window;
        if (win->layout.max_row_height > 0.0f)
        {
            win->layout.cursor.x = win->layout.start.x + win->layout.indent;
            win->layout.cursor.y = win->layout.last_item_pos.y + win->layout.max_row_height;
            win->layout.max_row_height = 0.0f;
            win->layout.type = BLOOM_LAYOUT_VERTICAL;
        }
        win->layout.cursor.y += ctx->style.item_spacing;
    }
}

void bloom_set_layout(bloom_i32 type)
{
    bloom_context *ctx = g_bloom_ctx;
    if (ctx && ctx->current_window)
    {
        ctx->current_window->layout.type = type;
    }
}

bloom_style *bloom_get_style(void)
{
    bloom_context *ctx = g_bloom_ctx;
    return ctx ? &ctx->style : NULL;
}

bloom_input *bloom_get_input(void)
{
    bloom_context *ctx = g_bloom_ctx;
    return ctx ? &ctx->input : NULL;
}

bloom_draw_list *bloom_get_draw_list(void)
{
    bloom_context *ctx = g_bloom_ctx;
    if (!ctx)
    {
        return NULL;
    }

    if (ctx->frame_active)
    {
        if (ctx->skip_depth > 0)
        {
            return NULL;
        }
        return bloom_context_layer_draw_list(ctx, ctx->draw_layer);
    }

    return &ctx->draw_list;
}

bloom_debug_info *bloom_get_debug_info(void)
{
    bloom_context *ctx = g_bloom_ctx;
    return ctx ? &ctx->debug_info : NULL;
}

void bloom_show_debug_overlay(bloom_bool show)
{
    bloom_context *ctx = g_bloom_ctx;
    if (ctx)
    {
        ctx->debug_overlay = show;
    }
}

bloom_font *bloom_get_default_font(void)
{
    bloom_context *ctx = g_bloom_ctx;
    return ctx ? &ctx->default_font : NULL;
}

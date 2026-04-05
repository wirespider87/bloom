#ifndef BLOOM_CORE_RUNTIME_CONTEXT_H
#define BLOOM_CORE_RUNTIME_CONTEXT_H

#include "core/base/types/types.h"
#include "core/base/memory/memory.h"
#include "core/runtime/input/input.h"
#include "core/runtime/style/style.h"
#include "core/graphics/draw/draw.h"
#include "core/graphics/font/font.h"
#include "core/base/hash/hash.h"
#include "core/runtime/animation/animation.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    BLOOM_LAYOUT_VERTICAL = 0,
    BLOOM_LAYOUT_HORIZONTAL,
    BLOOM_LAYOUT_MANUAL,
    BLOOM_LAYOUT_FLOW
};

enum
{
    BLOOM_WINDOW_NONE       = 0,
    BLOOM_WINDOW_NO_TITLE   = (1 << 0),
    BLOOM_WINDOW_NO_RESIZE  = (1 << 1),
    BLOOM_WINDOW_NO_MOVE    = (1 << 2),
    BLOOM_WINDOW_NO_SCROLL  = (1 << 3),
    BLOOM_WINDOW_AUTO_SIZE  = (1 << 4),
    BLOOM_WINDOW_NO_BORDER  = (1 << 5),
    BLOOM_WINDOW_TOOLTIP    = (1 << 6),
    BLOOM_WINDOW_POPUP      = (1 << 7),
    BLOOM_WINDOW_CHILD      = (1 << 8),
    BLOOM_WINDOW_ROOT       = (1 << 9),
    BLOOM_WINDOW_NO_COLLAPSE = (1 << 10),
    BLOOM_WINDOW_NO_MINIMIZE = (1 << 11),
    BLOOM_WINDOW_NO_BACKGROUND = (1 << 12),
    BLOOM_WINDOW_NO_ANIMATIONS = (1 << 13),
    BLOOM_WINDOW_HOST_MOVE = (1 << 14),
    BLOOM_WINDOW_NO_CLOSE = (1 << 15)
};

#define BLOOM_WINDOW_NO_TITLEBAR BLOOM_WINDOW_NO_TITLE
#define BLOOM_WINDOW_NO_DECORATION \
    (BLOOM_WINDOW_NO_TITLE | BLOOM_WINDOW_NO_RESIZE | BLOOM_WINDOW_NO_BORDER | \
     BLOOM_WINDOW_NO_COLLAPSE | BLOOM_WINDOW_NO_MINIMIZE | BLOOM_WINDOW_NO_CLOSE)

typedef struct bloom_layout
{
    bloom_i32   type;
    bloom_vec2  cursor;
    bloom_vec2  start;
    bloom_f32   max_row_height;
    bloom_f32   available_width;
    bloom_f32   indent;
    bloom_vec2  last_item_pos;
    bloom_vec2  last_item_size;
} bloom_layout;

typedef struct bloom_window
{
    bloom_id    id;
    char        name[128];
    bloom_rect  rect;
    bloom_rect  content_rect;
    bloom_f32   scroll_y;
    bloom_f32   scroll_target_y;
    bloom_f32   scroll_x;
    bloom_f32   content_height;
    bloom_f32   content_extent_y;
    bloom_f32   content_width;
    bloom_i32   flags;
    bloom_bool  active;
    bloom_bool  smooth_scroll;
    bloom_bool  collapsed;
    bloom_bool  minimized;
    bloom_bool  closed;
    bloom_bool  appeared;
    bloom_i32   order;
    bloom_u32   last_frame_active;
    bloom_rect  restore_rect;
    bloom_layout layout;
} bloom_window;

typedef struct bloom_popup
{
    bloom_id    id;
    bloom_bool  open;
    bloom_rect  rect;
} bloom_popup;

typedef struct bloom_window_args
{
    bloom_i32  flags;
    bloom_bool smooth_scroll;
} bloom_window_args;

#define BLOOM_WINDOW_ARGS_DEFAULT { BLOOM_WINDOW_NONE, BLOOM_TRUE }

typedef struct bloom_debug_info
{
    bloom_f32   fps;
    bloom_f32   frame_time_ms;
    bloom_u32   draw_calls;
    bloom_u32   vertex_count;
    bloom_u32   index_count;
    bloom_u32   window_count;
    bloom_u64   arena_used;
    bloom_u64   arena_total;
} bloom_debug_info;

typedef struct bloom_context
{
    bloom_arena     frame_arena;
    bloom_u8       *frame_arena_buffer;

    bloom_input     input;
    bloom_style     style;
    bloom_draw_list draw_list;
    bloom_draw_list background_draw_list;
    bloom_draw_list foreground_draw_list;

    bloom_vertex   *vertex_buffer;
    bloom_draw_idx *index_buffer;
    bloom_draw_cmd *cmd_buffer;
    bloom_vertex   *background_vertex_buffer;
    bloom_draw_idx *background_index_buffer;
    bloom_draw_cmd *background_cmd_buffer;
    bloom_vertex   *foreground_vertex_buffer;
    bloom_draw_idx *foreground_index_buffer;
    bloom_draw_cmd *foreground_cmd_buffer;

    bloom_window    windows[BLOOM_MAX_WINDOWS];
    bloom_window    root_window;
    bloom_i32       window_count;
    bloom_window   *current_window;
    bloom_window   *window_stack[16];
    bloom_i32       window_stack_depth;
    bloom_i32       window_order_counter;
    bloom_i32       skip_depth;

    bloom_id        id_stack[BLOOM_MAX_ID_STACK];
    bloom_i32       id_stack_depth;

    bloom_id        hot_id;
    bloom_id        active_id;
    bloom_id        focus_id;

    bloom_popup     popups[BLOOM_MAX_POPUPS];
    bloom_i32       popup_count;

    bloom_anim_track anim_tracks[BLOOM_MAX_ANIM_STATES];

    bloom_font      default_font;
    bloom_font     *current_font;

    bloom_vec2      display_size;
    bloom_f32       delta_time;
    bloom_f64       time;
    bloom_u32       frame_count;

    bloom_bool      initialized;
    bloom_bool      frame_active;

    bloom_bool      debug_overlay;
    bloom_debug_info debug_info;

    bloom_id        dragging_window;
    bloom_bool      drag_committed;
    bloom_id        resizing_window;
    bloom_vec2      drag_offset;
    bloom_vec2      drag_start_mouse;
    bloom_vec2      resize_start_mouse;
    bloom_rect      resize_start_rect;
    bloom_i32       resize_edges;

    bloom_f32       tooltip_timer;
    bloom_id        tooltip_id;

    bloom_i32       table_column_count;
    bloom_f32       table_column_widths[BLOOM_MAX_COLUMNS];
    bloom_i32       table_current_column;
    bloom_f32       table_row_y;
    bloom_i32       table_row_index;

    bloom_i32       draw_layer;
} bloom_context;

#if defined(BLOOM_INTERNAL_BUILD) || defined(BLOOM_ENABLE_LEGACY_API)
bloom_context *bloom_create_context(void);
void bloom_destroy_context(bloom_context *ctx);
bloom_context *bloom_get_context(void);
void bloom_set_context(bloom_context *ctx);

void bloom_begin_frame(void);
void bloom_end_frame(void);

void bloom_set_display_size(bloom_f32 w, bloom_f32 h);
void bloom_set_delta_time(bloom_f32 dt);

bloom_bool bloom_begin(const char *name);
bloom_bool bloom_begin_window(const char *name, bloom_bool border);
bloom_bool bloom_begin_args(const char *name, const bloom_window_args *args);
bloom_bool bloom_begin_ex(const char *name, bloom_i32 flags);
bloom_bool bloom_begin_root(const char *name);
bloom_bool bloom_begin_root_rect(const char *name, bloom_rect rect, bloom_f32 padding);
void bloom_end(void);

bloom_bool bloom_begin_child(const char *name, bloom_f32 w, bloom_f32 h);
bloom_bool bloom_begin_child_args(const char *name, bloom_f32 w, bloom_f32 h, const bloom_window_args *args);
void bloom_end_child(void);

bloom_id bloom_get_id(const char *str);
void bloom_push_id(const char *str);
void bloom_push_id_int(bloom_i32 n);
void bloom_push_id_ptr(const void *ptr);
void bloom_pop_id(void);

void bloom_set_next_window_pos(bloom_f32 x, bloom_f32 y);
void bloom_set_next_window_size(bloom_f32 w, bloom_f32 h);

bloom_vec2 bloom_get_cursor_pos(void);
void bloom_set_cursor_pos(bloom_f32 x, bloom_f32 y);
bloom_f32 bloom_get_content_width(void);

void bloom_same_line(void);
void bloom_new_line(void);
void bloom_indent(bloom_f32 amount);
void bloom_unindent(bloom_f32 amount);
void bloom_separator(void);
void bloom_spacing(void);

void bloom_set_layout(bloom_i32 type);

bloom_style *bloom_get_style(void);
bloom_input *bloom_get_input(void);
bloom_draw_list *bloom_get_draw_list(void);
bloom_debug_info *bloom_get_debug_info(void);

void bloom_show_debug_overlay(bloom_bool show);
bloom_font *bloom_get_default_font(void);
bloom_bool bloom_window_accepts_input(const bloom_window *win, bloom_vec2 point);
#endif

#ifdef __cplusplus
}
#endif

#endif

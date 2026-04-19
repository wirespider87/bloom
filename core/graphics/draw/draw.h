#ifndef BLOOM_CORE_GRAPHICS_DRAW_H
#define BLOOM_CORE_GRAPHICS_DRAW_H

#include "core/base/types/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bloom_vertex
{
    bloom_vec2  pos;
    bloom_vec2  center;
    bloom_vec2  half_size;
    bloom_vec2  uv;
    bloom_u32   col;
    bloom_vec4  corner_radii;
    bloom_f32   border_thickness;
    bloom_u32   elem_type;
} bloom_vertex;

typedef bloom_u16 bloom_draw_idx;

enum
{
    BLOOM_DRAW_CMD_TRIANGLES = 0,
    BLOOM_DRAW_CMD_CLIP
};

typedef struct bloom_draw_cmd
{
    bloom_i32   type;
    bloom_u32   elem_count;
    bloom_rect  clip_rect;
    bloom_u32   texture_id;
} bloom_draw_cmd;

typedef struct bloom_draw_list
{
    bloom_vertex   *vertices;
    bloom_u32       vertex_count;
    bloom_u32       vertex_capacity;

    bloom_draw_idx *indices;
    bloom_u32       index_count;
    bloom_u32       index_capacity;

    bloom_draw_cmd *commands;
    bloom_u32       cmd_count;
    bloom_u32       cmd_capacity;

    bloom_rect      clip_stack[BLOOM_MAX_CLIP_STACK];
    bloom_i32       clip_depth;

    bloom_u32       current_texture;
} bloom_draw_list;

typedef enum bloom_draw_layer
{
    BLOOM_DRAW_LAYER_WINDOW = 0,
    BLOOM_DRAW_LAYER_BACKGROUND,
    BLOOM_DRAW_LAYER_FOREGROUND
} bloom_draw_layer;

typedef struct bloom_corner_radii
{
    bloom_f32 top_left;
    bloom_f32 top_right;
    bloom_f32 bottom_right;
    bloom_f32 bottom_left;
} bloom_corner_radii;

static inline bloom_corner_radii bloom_make_corner_radii(bloom_f32 top_left, bloom_f32 top_right,
                                                         bloom_f32 bottom_right, bloom_f32 bottom_left)
{
    bloom_corner_radii radii;
    radii.top_left = top_left;
    radii.top_right = top_right;
    radii.bottom_right = bottom_right;
    radii.bottom_left = bottom_left;
    return radii;
}

static inline bloom_corner_radii bloom_make_corner_radii_all(bloom_f32 radius)
{
    return bloom_make_corner_radii(radius, radius, radius, radius);
}

static inline bloom_corner_radii bloom_make_corner_radii_top(bloom_f32 radius)
{
    return bloom_make_corner_radii(radius, radius, 0.0f, 0.0f);
}

static inline bloom_corner_radii bloom_make_corner_radii_bottom(bloom_f32 radius)
{
    return bloom_make_corner_radii(0.0f, 0.0f, radius, radius);
}

static inline bloom_corner_radii bloom_make_corner_radii_first(bloom_f32 radius)
{
    return bloom_make_corner_radii(radius, 0.0f, 0.0f, 0.0f);
}

static inline bloom_corner_radii bloom_make_corner_radii_second(bloom_f32 radius)
{
    return bloom_make_corner_radii(0.0f, radius, 0.0f, 0.0f);
}

static inline bloom_corner_radii bloom_make_corner_radii_third(bloom_f32 radius)
{
    return bloom_make_corner_radii(0.0f, 0.0f, radius, 0.0f);
}

static inline bloom_corner_radii bloom_make_corner_radii_fourth(bloom_f32 radius)
{
    return bloom_make_corner_radii(0.0f, 0.0f, 0.0f, radius);
}

#if defined(BLOOM_INTERNAL_BUILD) || defined(BLOOM_ENABLE_LEGACY_API)
void bloom_draw_list_init(bloom_draw_list *dl, bloom_vertex *verts, bloom_u32 vert_cap,
                          bloom_draw_idx *idxs, bloom_u32 idx_cap,
                          bloom_draw_cmd *cmds, bloom_u32 cmd_cap);
void bloom_draw_list_clear(bloom_draw_list *dl);
void bloom_draw_list_append(bloom_draw_list *dst, const bloom_draw_list *src);
void bloom_draw_list_prepend(bloom_draw_list *dst, const bloom_draw_list *src);

void bloom_draw_push_clip(bloom_draw_list *dl, bloom_rect rect);
void bloom_draw_pop_clip(bloom_draw_list *dl);

void bloom_draw_rect_filled(bloom_draw_list *dl, bloom_rect rect, bloom_color col);
void bloom_draw_rect(bloom_draw_list *dl, bloom_rect rect, bloom_color col, bloom_f32 thickness);
void bloom_draw_rect_rounded(bloom_draw_list *dl, bloom_rect rect, bloom_color col, bloom_f32 radius);
void bloom_draw_rect_rounded_border(bloom_draw_list *dl, bloom_rect rect, bloom_color col, bloom_f32 radius, bloom_f32 thickness);
void bloom_draw_rect_custom(bloom_draw_list *dl, bloom_rect rect, bloom_color fill_color,
                            bloom_color border_color, bloom_f32 border_thickness,
                            bloom_corner_radii radii);
void bloom_draw_triangle(bloom_draw_list *dl, bloom_vec2 a, bloom_vec2 b, bloom_vec2 c, bloom_color col);
void bloom_draw_line(bloom_draw_list *dl, bloom_vec2 a, bloom_vec2 b, bloom_color col, bloom_f32 thickness);
void bloom_draw_circle_filled(bloom_draw_list *dl, bloom_vec2 center, bloom_f32 radius, bloom_color col, int segments);

void bloom_draw_text(bloom_draw_list *dl, bloom_vec2 pos, const char *text,
                     bloom_color col, bloom_f32 font_size, bloom_u32 font_texture);
/*
 * UTF-8: text_len is bytes. Layout uses bloom_text_shape_visual (simplified RTL run order for
 * LTR paragraphs). Atlas-backed codepoints draw; others use '?'. Monochrome emoji only if cmap
 * maps a glyf; color emoji (COLR/CBDT) not supported.
 */
void bloom_draw_text_n(bloom_draw_list *dl, bloom_vec2 pos, const char *text, bloom_u32 text_len,
                       bloom_color col, bloom_f32 font_size, bloom_u32 font_texture);
bloom_f32 bloom_text_width(const char *text, bloom_f32 font_size);
/* Same UTF-8 byte semantics as bloom_draw_text_n */
bloom_f32 bloom_text_width_n(const char *text, bloom_u32 text_len, bloom_f32 font_size);
#endif

#ifdef __cplusplus
}
#endif

#endif

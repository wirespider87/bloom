#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "core/graphics/draw/draw.h"
#include "core/graphics/font/font.h"
#include "core/runtime/context/context.h"
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define BLOOM_ELEM_SDF_RECT   0
#define BLOOM_ELEM_MSDF_TEXT  1
#define BLOOM_ELEM_PLAIN      2
#define BLOOM_ELEM_SDF_BORDER 3
#define BLOOM_ELEM_SDF_LINE   4

void bloom_draw_list_init(bloom_draw_list *dl, bloom_vertex *verts, bloom_u32 vert_cap,
                          bloom_draw_idx *idxs, bloom_u32 idx_cap,
                          bloom_draw_cmd *cmds, bloom_u32 cmd_cap)
{
    dl->vertices = verts;
    dl->vertex_count = 0;
    dl->vertex_capacity = vert_cap;
    dl->indices = idxs;
    dl->index_count = 0;
    dl->index_capacity = idx_cap;
    dl->commands = cmds;
    dl->cmd_count = 0;
    dl->cmd_capacity = cmd_cap;
    dl->clip_depth = 0;
    dl->current_texture = 0;
}

void bloom_draw_list_clear(bloom_draw_list *dl)
{
    dl->vertex_count = 0;
    dl->index_count = 0;
    dl->cmd_count = 0;
    dl->clip_depth = 0;
}

void bloom_draw_list_append(bloom_draw_list *dst, const bloom_draw_list *src)
{
    bloom_u32 vertex_base;
    bloom_u32 index_base;
    bloom_u32 i;

    if (!dst || !src || src->vertex_count == 0 || src->index_count == 0 || src->cmd_count == 0)
    {
        return;
    }

    if (dst->vertex_count + src->vertex_count > dst->vertex_capacity ||
        dst->index_count + src->index_count > dst->index_capacity ||
        dst->cmd_count + src->cmd_count > dst->cmd_capacity)
    {
        return;
    }

    vertex_base = dst->vertex_count;
    index_base = dst->index_count;

    memcpy(&dst->vertices[dst->vertex_count], src->vertices, sizeof(bloom_vertex) * src->vertex_count);
    dst->vertex_count += src->vertex_count;

    for (i = 0; i < src->index_count; ++i)
    {
        dst->indices[index_base + i] = (bloom_draw_idx)(src->indices[i] + vertex_base);
    }
    dst->index_count += src->index_count;

    memcpy(&dst->commands[dst->cmd_count], src->commands, sizeof(bloom_draw_cmd) * src->cmd_count);
    dst->cmd_count += src->cmd_count;
}

void bloom_draw_list_prepend(bloom_draw_list *dst, const bloom_draw_list *src)
{
    bloom_u32 i;

    if (!dst || !src || src->vertex_count == 0 || src->index_count == 0 || src->cmd_count == 0)
    {
        return;
    }

    if (dst->vertex_count + src->vertex_count > dst->vertex_capacity ||
        dst->index_count + src->index_count > dst->index_capacity ||
        dst->cmd_count + src->cmd_count > dst->cmd_capacity)
    {
        return;
    }

    memmove(&dst->vertices[src->vertex_count], dst->vertices, sizeof(bloom_vertex) * dst->vertex_count);

    for (i = dst->index_count; i > 0; --i)
    {
        dst->indices[src->index_count + i - 1] = (bloom_draw_idx)(dst->indices[i - 1] + src->vertex_count);
    }

    memmove(&dst->commands[src->cmd_count], dst->commands, sizeof(bloom_draw_cmd) * dst->cmd_count);

    memcpy(dst->vertices, src->vertices, sizeof(bloom_vertex) * src->vertex_count);
    memcpy(dst->indices, src->indices, sizeof(bloom_draw_idx) * src->index_count);
    memcpy(dst->commands, src->commands, sizeof(bloom_draw_cmd) * src->cmd_count);

    dst->vertex_count += src->vertex_count;
    dst->index_count += src->index_count;
    dst->cmd_count += src->cmd_count;
}

static bloom_bool bloom_rect_exact_equal(bloom_rect a, bloom_rect b)
{
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}

static void bloom_draw_add_cmd(bloom_draw_list *dl, bloom_u32 elem_count)
{
    bloom_rect clip_rect;

    if (dl->clip_depth > 0)
    {
        clip_rect = dl->clip_stack[dl->clip_depth - 1];
    }
    else
    {
        clip_rect = bloom_make_rect(-1, -1, -1, -1);
    }

    if (dl->cmd_count > 0)
    {
        bloom_draw_cmd *prev = &dl->commands[dl->cmd_count - 1];
        if (prev->type == BLOOM_DRAW_CMD_TRIANGLES &&
            prev->texture_id == dl->current_texture &&
            bloom_rect_exact_equal(prev->clip_rect, clip_rect))
        {
            prev->elem_count += elem_count;
            return;
        }
    }

    if (dl->cmd_count >= dl->cmd_capacity)
    {
        return;
    }

    {
        bloom_draw_cmd *cmd = &dl->commands[dl->cmd_count++];
        cmd->type = BLOOM_DRAW_CMD_TRIANGLES;
        cmd->elem_count = elem_count;
        cmd->texture_id = dl->current_texture;
        cmd->clip_rect = clip_rect;
    }
}

void bloom_draw_push_clip(bloom_draw_list *dl, bloom_rect rect)
{
    if (dl->clip_depth < BLOOM_MAX_CLIP_STACK)
    {
        if (dl->clip_depth > 0)
        {
            bloom_rect parent = dl->clip_stack[dl->clip_depth - 1];
            if (parent.w >= 0.0f && parent.h >= 0.0f)
            {
                bloom_f32 x0 = rect.x > parent.x ? rect.x : parent.x;
                bloom_f32 y0 = rect.y > parent.y ? rect.y : parent.y;
                bloom_f32 x1 = (rect.x + rect.w) < (parent.x + parent.w) ? (rect.x + rect.w) : (parent.x + parent.w);
                bloom_f32 y1 = (rect.y + rect.h) < (parent.y + parent.h) ? (rect.y + rect.h) : (parent.y + parent.h);
                if (x1 < x0) x1 = x0;
                if (y1 < y0) y1 = y0;
                rect = bloom_make_rect(x0, y0, x1 - x0, y1 - y0);
            }
        }
        dl->clip_stack[dl->clip_depth++] = rect;
    }
}

void bloom_draw_pop_clip(bloom_draw_list *dl)
{
    if (dl->clip_depth > 0)
    {
        dl->clip_depth--;
    }
}

static bloom_bool bloom_draw_reserve(bloom_draw_list *dl, bloom_u32 vtx_count, bloom_u32 idx_count)
{
    if (dl->vertex_count + vtx_count > dl->vertex_capacity)
    {
        return BLOOM_FALSE;
    }
    if (dl->index_count + idx_count > dl->index_capacity)
    {
        return BLOOM_FALSE;
    }
    return BLOOM_TRUE;
}

static bloom_vertex bloom_make_sdf_vertex(bloom_vec2 pos, bloom_vec2 center, bloom_vec2 half_size,
                                          bloom_vec2 uv, bloom_u32 col, bloom_f32 corner_radius,
                                          bloom_f32 border_thickness, bloom_u32 elem_type)
{
    bloom_vertex v;
    v.pos = pos;
    v.center = center;
    v.half_size = half_size;
    v.uv = uv;
    v.col = col;
    v.corner_radius = corner_radius;
    v.border_thickness = border_thickness;
    v.elem_type = elem_type;
    return v;
}

static bloom_vertex bloom_make_plain_vertex(bloom_vec2 pos, bloom_u32 col)
{
    bloom_vertex v;
    v.pos = pos;
    v.center = bloom_v2(0, 0);
    v.half_size = bloom_v2(0, 0);
    v.uv = bloom_v2(0, 0);
    v.col = col;
    v.corner_radius = 0.0f;
    v.border_thickness = 0.0f;
    v.elem_type = BLOOM_ELEM_PLAIN;
    return v;
}

static void bloom_draw_sdf_quad(bloom_draw_list *dl, bloom_rect rect, bloom_u32 col,
                                bloom_f32 corner_radius, bloom_f32 border_thickness,
                                bloom_u32 elem_type)
{
    bloom_f32 pad = 1.5f;
    bloom_vec2 center = bloom_v2(rect.x + rect.w * 0.5f, rect.y + rect.h * 0.5f);
    bloom_vec2 half_size = bloom_v2(rect.w * 0.5f, rect.h * 0.5f);
    bloom_vec2 uv = bloom_v2(0, 0);
    bloom_u32 base;

    if (!bloom_draw_reserve(dl, 4, 6))
    {
        return;
    }

    base = dl->vertex_count;

    dl->vertices[dl->vertex_count++] = bloom_make_sdf_vertex(
        bloom_v2(rect.x - pad, rect.y - pad), center, half_size, uv, col, corner_radius, border_thickness, elem_type);
    dl->vertices[dl->vertex_count++] = bloom_make_sdf_vertex(
        bloom_v2(rect.x + rect.w + pad, rect.y - pad), center, half_size, uv, col, corner_radius, border_thickness, elem_type);
    dl->vertices[dl->vertex_count++] = bloom_make_sdf_vertex(
        bloom_v2(rect.x + rect.w + pad, rect.y + rect.h + pad), center, half_size, uv, col, corner_radius, border_thickness, elem_type);
    dl->vertices[dl->vertex_count++] = bloom_make_sdf_vertex(
        bloom_v2(rect.x - pad, rect.y + rect.h + pad), center, half_size, uv, col, corner_radius, border_thickness, elem_type);

    dl->indices[dl->index_count++] = (bloom_draw_idx)(base + 0);
    dl->indices[dl->index_count++] = (bloom_draw_idx)(base + 1);
    dl->indices[dl->index_count++] = (bloom_draw_idx)(base + 2);
    dl->indices[dl->index_count++] = (bloom_draw_idx)(base + 0);
    dl->indices[dl->index_count++] = (bloom_draw_idx)(base + 2);
    dl->indices[dl->index_count++] = (bloom_draw_idx)(base + 3);

    bloom_draw_add_cmd(dl, 6);
}

static bloom_f32 bloom_draw_clamp_f32(bloom_f32 value, bloom_f32 min_value, bloom_f32 max_value)
{
    if (value < min_value)
    {
        return min_value;
    }
    if (value > max_value)
    {
        return max_value;
    }
    return value;
}

static bloom_corner_radii bloom_draw_normalize_corner_radii(bloom_rect rect, bloom_corner_radii radii)
{
    bloom_f32 scale = 1.0f;
    bloom_f32 edge_sum;

    radii.top_left = bloom_draw_clamp_f32(radii.top_left, 0.0f, rect.w * 0.5f);
    radii.top_right = bloom_draw_clamp_f32(radii.top_right, 0.0f, rect.w * 0.5f);
    radii.bottom_right = bloom_draw_clamp_f32(radii.bottom_right, 0.0f, rect.w * 0.5f);
    radii.bottom_left = bloom_draw_clamp_f32(radii.bottom_left, 0.0f, rect.w * 0.5f);

    radii.top_left = bloom_draw_clamp_f32(radii.top_left, 0.0f, rect.h * 0.5f);
    radii.top_right = bloom_draw_clamp_f32(radii.top_right, 0.0f, rect.h * 0.5f);
    radii.bottom_right = bloom_draw_clamp_f32(radii.bottom_right, 0.0f, rect.h * 0.5f);
    radii.bottom_left = bloom_draw_clamp_f32(radii.bottom_left, 0.0f, rect.h * 0.5f);

    edge_sum = radii.top_left + radii.top_right;
    if (edge_sum > rect.w && edge_sum > 0.0f)
    {
        bloom_f32 edge_scale = rect.w / edge_sum;
        if (edge_scale < scale) scale = edge_scale;
    }
    edge_sum = radii.bottom_left + radii.bottom_right;
    if (edge_sum > rect.w && edge_sum > 0.0f)
    {
        bloom_f32 edge_scale = rect.w / edge_sum;
        if (edge_scale < scale) scale = edge_scale;
    }
    edge_sum = radii.top_left + radii.bottom_left;
    if (edge_sum > rect.h && edge_sum > 0.0f)
    {
        bloom_f32 edge_scale = rect.h / edge_sum;
        if (edge_scale < scale) scale = edge_scale;
    }
    edge_sum = radii.top_right + radii.bottom_right;
    if (edge_sum > rect.h && edge_sum > 0.0f)
    {
        bloom_f32 edge_scale = rect.h / edge_sum;
        if (edge_scale < scale) scale = edge_scale;
    }

    if (scale < 1.0f)
    {
        radii.top_left *= scale;
        radii.top_right *= scale;
        radii.bottom_right *= scale;
        radii.bottom_left *= scale;
    }

    return radii;
}

void bloom_draw_rect_filled(bloom_draw_list *dl, bloom_rect rect, bloom_color col)
{
    if (rect.w <= 0.0f || rect.h <= 0.0f || col.a == 0)
    {
        return;
    }

    bloom_draw_sdf_quad(dl, rect, bloom_color_to_u32(col), 0.0f, 0.0f, BLOOM_ELEM_SDF_RECT);
}

void bloom_draw_rect(bloom_draw_list *dl, bloom_rect rect, bloom_color col, bloom_f32 thickness)
{
    if (rect.w <= 0.0f || rect.h <= 0.0f || col.a == 0 || thickness <= 0.0f)
    {
        return;
    }

    bloom_draw_sdf_quad(dl, rect, bloom_color_to_u32(col), 0.0f, thickness, BLOOM_ELEM_SDF_BORDER);
}

void bloom_draw_rect_rounded(bloom_draw_list *dl, bloom_rect rect, bloom_color col, bloom_f32 radius)
{
    if (rect.w <= 0.0f || rect.h <= 0.0f || col.a == 0)
    {
        return;
    }

    if (radius <= 0.5f)
    {
        bloom_draw_rect_filled(dl, rect, col);
        return;
    }

    {
        bloom_f32 r = radius;
        if (r > rect.w * 0.5f) r = rect.w * 0.5f;
        if (r > rect.h * 0.5f) r = rect.h * 0.5f;
        bloom_draw_sdf_quad(dl, rect, bloom_color_to_u32(col), r, 0.0f, BLOOM_ELEM_SDF_RECT);
    }
}

void bloom_draw_rect_rounded_border(bloom_draw_list *dl, bloom_rect rect, bloom_color col, bloom_f32 radius, bloom_f32 thickness)
{
    if (rect.w <= 0.0f || rect.h <= 0.0f || col.a == 0 || thickness <= 0.0f)
    {
        return;
    }

    if (radius <= 0.5f)
    {
        bloom_draw_rect(dl, rect, col, thickness);
        return;
    }

    {
        bloom_f32 r = radius;
        if (r > rect.w * 0.5f) r = rect.w * 0.5f;
        if (r > rect.h * 0.5f) r = rect.h * 0.5f;
        bloom_draw_sdf_quad(dl, rect, bloom_color_to_u32(col), r, thickness, BLOOM_ELEM_SDF_BORDER);
    }
}

void bloom_draw_rect_custom(bloom_draw_list *dl, bloom_rect rect, bloom_color fill_color,
                            bloom_color border_color, bloom_f32 border_thickness,
                            bloom_corner_radii radii)
{
    bloom_corner_radii nr;
    bloom_f32 max_r;

    if (!dl || rect.w <= 0.0f || rect.h <= 0.0f)
    {
        return;
    }

    nr = bloom_draw_normalize_corner_radii(rect, radii);
    max_r = nr.top_left;
    if (nr.top_right > max_r) max_r = nr.top_right;
    if (nr.bottom_right > max_r) max_r = nr.bottom_right;
    if (nr.bottom_left > max_r) max_r = nr.bottom_left;

    if (border_color.a > 0 && border_thickness > 0.0f)
    {
        bloom_draw_sdf_quad(dl, rect, bloom_color_to_u32(border_color), max_r, border_thickness, BLOOM_ELEM_SDF_BORDER);
    }

    if (fill_color.a > 0)
    {
        bloom_rect inner_rect = rect;
        bloom_f32 inner_r = max_r;

        if (border_color.a > 0 && border_thickness > 0.0f)
        {
            inner_rect = bloom_make_rect(rect.x + border_thickness, rect.y + border_thickness,
                                         rect.w - border_thickness * 2.0f, rect.h - border_thickness * 2.0f);
            inner_r = bloom_draw_clamp_f32(max_r - border_thickness, 0.0f, inner_rect.w * 0.5f);
            if (inner_r > inner_rect.h * 0.5f) inner_r = inner_rect.h * 0.5f;
        }

        if (inner_rect.w > 0.0f && inner_rect.h > 0.0f)
        {
            bloom_draw_sdf_quad(dl, inner_rect, bloom_color_to_u32(fill_color), inner_r, 0.0f, BLOOM_ELEM_SDF_RECT);
        }
    }
}

void bloom_draw_triangle(bloom_draw_list *dl, bloom_vec2 a, bloom_vec2 b, bloom_vec2 c, bloom_color col)
{
    bloom_u32 base;
    bloom_u32 cv;

    if (!bloom_draw_reserve(dl, 3, 3))
    {
        return;
    }

    base = dl->vertex_count;
    cv = bloom_color_to_u32(col);

    dl->vertices[dl->vertex_count++] = bloom_make_plain_vertex(a, cv);
    dl->vertices[dl->vertex_count++] = bloom_make_plain_vertex(b, cv);
    dl->vertices[dl->vertex_count++] = bloom_make_plain_vertex(c, cv);

    dl->indices[dl->index_count++] = (bloom_draw_idx)(base + 0);
    dl->indices[dl->index_count++] = (bloom_draw_idx)(base + 1);
    dl->indices[dl->index_count++] = (bloom_draw_idx)(base + 2);

    bloom_draw_add_cmd(dl, 3);
}

void bloom_draw_line(bloom_draw_list *dl, bloom_vec2 a, bloom_vec2 b, bloom_color col, bloom_f32 thickness)
{
    bloom_f32 dx = b.x - a.x;
    bloom_f32 dy = b.y - a.y;
    bloom_f32 len = sqrtf(dx * dx + dy * dy);
    bloom_f32 half_t = thickness * 0.5f;
    bloom_f32 pad = 1.5f;
    bloom_f32 nx;
    bloom_f32 ny;
    bloom_f32 tx;
    bloom_f32 ty;
    bloom_vec2 p0;
    bloom_vec2 p1;
    bloom_vec2 p2;
    bloom_vec2 p3;
    bloom_u32 base;
    bloom_u32 c;

    if (len < 0.001f || col.a == 0)
    {
        return;
    }

    tx = dx / len;
    ty = dy / len;
    nx = -ty;
    ny = tx;

    p0 = bloom_v2(a.x + nx * (half_t + pad) - tx * pad, a.y + ny * (half_t + pad) - ty * pad);
    p1 = bloom_v2(b.x + nx * (half_t + pad) + tx * pad, b.y + ny * (half_t + pad) + ty * pad);
    p2 = bloom_v2(b.x - nx * (half_t + pad) + tx * pad, b.y - ny * (half_t + pad) + ty * pad);
    p3 = bloom_v2(a.x - nx * (half_t + pad) - tx * pad, a.y - ny * (half_t + pad) - ty * pad);

    if (!bloom_draw_reserve(dl, 4, 6))
    {
        return;
    }

    base = dl->vertex_count;
    c = bloom_color_to_u32(col);

    dl->vertices[dl->vertex_count++] = bloom_make_sdf_vertex(p0, a, b, bloom_v2(0, 0), c, half_t, 0.0f, BLOOM_ELEM_SDF_LINE);
    dl->vertices[dl->vertex_count++] = bloom_make_sdf_vertex(p1, a, b, bloom_v2(0, 0), c, half_t, 0.0f, BLOOM_ELEM_SDF_LINE);
    dl->vertices[dl->vertex_count++] = bloom_make_sdf_vertex(p2, a, b, bloom_v2(0, 0), c, half_t, 0.0f, BLOOM_ELEM_SDF_LINE);
    dl->vertices[dl->vertex_count++] = bloom_make_sdf_vertex(p3, a, b, bloom_v2(0, 0), c, half_t, 0.0f, BLOOM_ELEM_SDF_LINE);

    dl->indices[dl->index_count++] = (bloom_draw_idx)(base + 0);
    dl->indices[dl->index_count++] = (bloom_draw_idx)(base + 1);
    dl->indices[dl->index_count++] = (bloom_draw_idx)(base + 2);
    dl->indices[dl->index_count++] = (bloom_draw_idx)(base + 0);
    dl->indices[dl->index_count++] = (bloom_draw_idx)(base + 2);
    dl->indices[dl->index_count++] = (bloom_draw_idx)(base + 3);

    bloom_draw_add_cmd(dl, 6);
}

void bloom_draw_circle_filled(bloom_draw_list *dl, bloom_vec2 center, bloom_f32 radius, bloom_color col, int segments)
{
    bloom_rect rect;

    (void)segments;

    if (radius <= 0.0f || col.a == 0)
    {
        return;
    }

    rect = bloom_make_rect(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f);
    bloom_draw_sdf_quad(dl, rect, bloom_color_to_u32(col), radius, 0.0f, BLOOM_ELEM_SDF_RECT);
}

void bloom_draw_text(bloom_draw_list *dl, bloom_vec2 pos, const char *text,
                     bloom_color col, bloom_f32 font_size, bloom_u32 font_texture)
{
    bloom_draw_text_n(dl, pos, text, (bloom_u32)strlen(text), col, font_size, font_texture);
}

void bloom_draw_text_n(bloom_draw_list *dl, bloom_vec2 pos, const char *text, bloom_u32 text_len,
                      bloom_color col, bloom_f32 font_size, bloom_u32 font_texture)
{
    bloom_font *font = bloom_font_get_active();
    bloom_f32 x = floorf(pos.x + 0.5f);
    bloom_f32 y = floorf(pos.y + 0.5f);
    bloom_f32 scale = 1.0f;
    bloom_u32 old_tex = dl->current_texture;
    bloom_u32 idx = 0;

    if (font && font->valid && font->size > 0.0f)
    {
        scale = font_size / font->size;
    }

    dl->current_texture = font_texture;

    while (idx < text_len)
    {
        bloom_u8 ch_u8 = (bloom_u8)text[idx];

        if (ch_u8 == '\n')
        {
            x = floorf(pos.x + 0.5f);
            y += font ? font->line_height * scale : font_size;
            idx++;
            continue;
        }

        if (ch_u8 < 32)
        {
            idx++;
            continue;
        }

        {
            bloom_f32 char_w = font_size * 0.55f;
            bloom_f32 char_h = font_size;
            bloom_f32 gx0 = 0.0f;
            bloom_f32 gy0 = 0.0f;
            bloom_f32 draw_x0;
            bloom_f32 draw_y0;
            bloom_f32 draw_x1;
            bloom_f32 draw_y1;
            bloom_f32 u0;
            bloom_f32 v0;
            bloom_f32 u1;
            bloom_f32 v1;

            if (font && font->valid && ch_u8 >= 32 && ch_u8 < 128)
            {
                bloom_glyph *glyph = &font->glyphs[ch_u8 - 32];
                char_w = (glyph->x1 - glyph->x0) * scale;
                char_h = (glyph->y1 - glyph->y0) * scale;
                gx0 = glyph->x0 * scale;
                gy0 = glyph->y0 * scale;
                u0 = glyph->u0;
                v0 = glyph->v0;
                u1 = glyph->u1;
                v1 = glyph->v1;
            }
            else
            {
                int ch = ch_u8 - 32;
                bloom_f32 atlas_w = 16.0f;
                bloom_f32 atlas_h = 8.0f;
                u0 = (bloom_f32)(ch % 16) / atlas_w;
                v0 = (bloom_f32)(ch / 16) / atlas_h;
                u1 = u0 + 1.0f / atlas_w;
                v1 = v0 + 1.0f / atlas_h;
            }

            if (bloom_draw_reserve(dl, 4, 6))
            {
                bloom_u32 base = dl->vertex_count;
                bloom_u32 c = bloom_color_to_u32(col);
                bloom_vec2 center;
                bloom_vec2 half_size;

                draw_x0 = x + gx0;
                draw_y0 = y + gy0;
                draw_x1 = draw_x0 + char_w;
                draw_y1 = draw_y0 + char_h;

                if (draw_x1 <= draw_x0)
                {
                    draw_x1 = draw_x0 + 1.0f;
                }
                if (draw_y1 <= draw_y0)
                {
                    draw_y1 = draw_y0 + 1.0f;
                }

                center = bloom_v2((draw_x0 + draw_x1) * 0.5f, (draw_y0 + draw_y1) * 0.5f);
                half_size = bloom_v2((draw_x1 - draw_x0) * 0.5f, (draw_y1 - draw_y0) * 0.5f);

                dl->vertices[dl->vertex_count++] = bloom_make_sdf_vertex(
                    bloom_v2(draw_x0, draw_y0), center, half_size, bloom_v2(u0, v0), c, 0.0f, 0.0f, BLOOM_ELEM_MSDF_TEXT);
                dl->vertices[dl->vertex_count++] = bloom_make_sdf_vertex(
                    bloom_v2(draw_x1, draw_y0), center, half_size, bloom_v2(u1, v0), c, 0.0f, 0.0f, BLOOM_ELEM_MSDF_TEXT);
                dl->vertices[dl->vertex_count++] = bloom_make_sdf_vertex(
                    bloom_v2(draw_x1, draw_y1), center, half_size, bloom_v2(u1, v1), c, 0.0f, 0.0f, BLOOM_ELEM_MSDF_TEXT);
                dl->vertices[dl->vertex_count++] = bloom_make_sdf_vertex(
                    bloom_v2(draw_x0, draw_y1), center, half_size, bloom_v2(u0, v1), c, 0.0f, 0.0f, BLOOM_ELEM_MSDF_TEXT);

                dl->indices[dl->index_count++] = (bloom_draw_idx)(base + 0);
                dl->indices[dl->index_count++] = (bloom_draw_idx)(base + 1);
                dl->indices[dl->index_count++] = (bloom_draw_idx)(base + 2);
                dl->indices[dl->index_count++] = (bloom_draw_idx)(base + 0);
                dl->indices[dl->index_count++] = (bloom_draw_idx)(base + 2);
                dl->indices[dl->index_count++] = (bloom_draw_idx)(base + 3);

                bloom_draw_add_cmd(dl, 6);
            }

            if (font && font->valid)
            {
                x += bloom_font_char_width(font, (bloom_u32)ch_u8) * scale;
            }
            else
            {
                x += char_w;
            }
        }
        idx++;
    }

    dl->current_texture = old_tex;
}

bloom_f32 bloom_text_width(const char *text, bloom_f32 font_size)
{
    return bloom_text_width_n(text, (bloom_u32)strlen(text), font_size);
}

bloom_f32 bloom_text_width_n(const char *text, bloom_u32 text_len, bloom_f32 font_size)
{
    bloom_font *font = bloom_font_get_active();
    if (font && font->valid)
    {
        bloom_f32 scale = 1.0f;
        if (font->size > 0.0f)
        {
            scale = font_size / font->size;
        }
        {
            bloom_f32 w = 0;
            bloom_f32 line_w = 0;
            bloom_u32 i;
            for (i = 0; i < text_len; ++i)
            {
                if (text[i] == '\n')
                {
                    if (line_w > w) w = line_w;
                    line_w = 0;
                }
                else
                {
                    bloom_u32 codepoint = (bloom_u32)(bloom_u8)text[i];
                    if (codepoint >= 32)
                    {
                        line_w += bloom_font_char_width(font, codepoint) * scale;
                    }
                }
            }
            if (line_w > w) w = line_w;
            return w;
        }
    }

    {
        bloom_f32 w = 0;
        bloom_f32 line_w = 0;
        bloom_f32 char_w = font_size * 0.55f;
        bloom_u32 i;
        for (i = 0; i < text_len; ++i)
        {
            if (text[i] == '\n')
            {
                if (line_w > w) w = line_w;
                line_w = 0;
            }
            else
            {
                line_w += char_w;
            }
        }
        if (line_w > w) w = line_w;
        return w;
    }
}

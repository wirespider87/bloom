#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "../bloom.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static bloom_vec2 bloom_api_v2(bloom_f32 x, bloom_f32 y)
{
    return bloom_v2(x, y);
}

static bloom_rect bloom_api_rect_make(bloom_f32 x, bloom_f32 y, bloom_f32 w, bloom_f32 h)
{
    return bloom_make_rect(x, y, w, h);
}

static bloom_color bloom_api_rgba(bloom_u8 r, bloom_u8 g, bloom_u8 b, bloom_u8 a)
{
    return bloom_rgba(r, g, b, a);
}

static bloom_color bloom_api_rgb(bloom_u8 r, bloom_u8 g, bloom_u8 b)
{
    return bloom_rgb(r, g, b);
}

static bloom_corner_radii bloom_api_corner_radii(bloom_f32 top_left, bloom_f32 top_right,
                                                 bloom_f32 bottom_right, bloom_f32 bottom_left)
{
    return bloom_make_corner_radii(top_left, top_right, bottom_right, bottom_left);
}

static bloom_corner_radii bloom_api_corner_radii_all(bloom_f32 radius)
{
    return bloom_make_corner_radii_all(radius);
}

static bloom_corner_radii bloom_api_corner_radii_top(bloom_f32 radius)
{
    return bloom_make_corner_radii_top(radius);
}

static bloom_corner_radii bloom_api_corner_radii_bottom(bloom_f32 radius)
{
    return bloom_make_corner_radii_bottom(radius);
}

static bloom_corner_radii bloom_api_corner_radii_first(bloom_f32 radius)
{
    return bloom_make_corner_radii_first(radius);
}

static bloom_corner_radii bloom_api_corner_radii_second(bloom_f32 radius)
{
    return bloom_make_corner_radii_second(radius);
}

static bloom_corner_radii bloom_api_corner_radii_third(bloom_f32 radius)
{
    return bloom_make_corner_radii_third(radius);
}

static bloom_corner_radii bloom_api_corner_radii_fourth(bloom_f32 radius)
{
    return bloom_make_corner_radii_fourth(radius);
}

static bloom_draw_list *bloom_api_draw_list(void)
{
    return bloom_get_draw_list();
}

static bloom_draw_list *bloom_api_draw_list_for_layer_internal(bloom_i32 layer)
{
    bloom_context *ctx = bloom_get_context();

    if (!ctx || ctx->skip_depth > 0)
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

static bloom_draw_list *bloom_api_draw_list_for_layer(bloom_i32 layer)
{
    return bloom_api_draw_list_for_layer_internal(layer);
}

static bloom_i32 bloom_api_draw_get_layer(void)
{
    bloom_context *ctx = bloom_get_context();
    return ctx ? ctx->draw_layer : BLOOM_DRAW_LAYER_WINDOW;
}

static void bloom_api_draw_set_layer(bloom_i32 layer)
{
    bloom_context *ctx = bloom_get_context();
    if (!ctx)
    {
        return;
    }

    if (layer != BLOOM_DRAW_LAYER_BACKGROUND && layer != BLOOM_DRAW_LAYER_FOREGROUND)
    {
        layer = BLOOM_DRAW_LAYER_WINDOW;
    }
    ctx->draw_layer = layer;
}

static void bloom_api_draw_reset_layer(void)
{
    bloom_api_draw_set_layer(BLOOM_DRAW_LAYER_WINDOW);
}

static void bloom_api_draw_line(bloom_vec2 a, bloom_vec2 b, bloom_color col, bloom_f32 thickness)
{
    bloom_draw_list *dl = bloom_get_draw_list();
    if (dl)
    {
        bloom_draw_line(dl, a, b, col, thickness);
    }
}

static void bloom_api_draw_triangle(bloom_vec2 a, bloom_vec2 b, bloom_vec2 c, bloom_color col)
{
    bloom_draw_list *dl = bloom_get_draw_list();
    if (dl)
    {
        bloom_draw_triangle(dl, a, b, c, col);
    }
}

static void bloom_api_draw_rect(bloom_rect rect, bloom_color col, bloom_f32 thickness)
{
    bloom_draw_list *dl = bloom_get_draw_list();
    if (dl)
    {
        bloom_draw_rect(dl, rect, col, thickness);
    }
}

static void bloom_api_draw_rect_filled(bloom_rect rect, bloom_color col)
{
    bloom_draw_list *dl = bloom_get_draw_list();
    if (dl)
    {
        bloom_draw_rect_filled(dl, rect, col);
    }
}

static void bloom_api_draw_rect_rounded(bloom_rect rect, bloom_color col, bloom_f32 radius)
{
    bloom_draw_list *dl = bloom_get_draw_list();
    if (dl)
    {
        bloom_draw_rect_rounded(dl, rect, col, radius);
    }
}

static void bloom_api_draw_rect_rounded_border(bloom_rect rect, bloom_color col,
                                               bloom_f32 radius, bloom_f32 thickness)
{
    bloom_draw_list *dl = bloom_get_draw_list();
    if (dl)
    {
        bloom_draw_rect_rounded_border(dl, rect, col, radius, thickness);
    }
}

static void bloom_api_draw_circle_filled(bloom_vec2 center, bloom_f32 radius, bloom_color col, int segments)
{
    bloom_draw_list *dl = bloom_get_draw_list();
    if (dl)
    {
        bloom_draw_circle_filled(dl, center, radius, col, segments);
    }
}

static void bloom_api_draw_text(bloom_vec2 pos, const char *text, bloom_color col, bloom_f32 font_size)
{
    bloom_context *ctx = bloom_get_context();
    bloom_draw_list *dl = bloom_get_draw_list();
    if (ctx && dl)
    {
        bloom_draw_text(dl, pos, text, col, font_size, ctx->default_font.texture_id);
    }
}

static bloom_id bloom_api_anim_key_id(const char *key)
{
    return bloom_hash_str(key ? key : "##bloom_anim");
}

static bloom_f32 bloom_api_anim_clamp01(bloom_f32 t)
{
    return bloom_anim_clamp01(t);
}

static bloom_f32 bloom_api_anim_ease(bloom_f32 t, bloom_anim_ease ease)
{
    return bloom_anim_ease_apply(t, ease);
}

static bloom_f32 bloom_api_anim_value(const char *key, bloom_f32 target, bloom_f32 response)
{
    return bloom_anim_state(bloom_api_anim_key_id(key), target, response);
}

static bloom_f32 bloom_api_anim_toggle(const char *key, bloom_bool on, bloom_f32 response)
{
    return bloom_anim_toggle(bloom_api_anim_key_id(key), on, response);
}

static bloom_f32 bloom_api_anim_range(const char *key, bloom_f32 from, bloom_f32 to,
                                      bloom_f32 target_t, bloom_f32 response,
                                      bloom_anim_ease ease)
{
    return bloom_anim_range(bloom_api_anim_key_id(key), from, to, target_t, response, ease);
}

static bloom_f32 bloom_api_anim_pulse(const char *key, bloom_bool active,
                                      bloom_f32 attack, bloom_f32 decay)
{
    return bloom_anim_pulse(bloom_api_anim_key_id(key), active, attack, decay);
}

static bloom_f32 bloom_api_anim_spring(const char *key, bloom_f32 target,
                                       bloom_f32 stiffness, bloom_f32 damping)
{
    return bloom_anim_spring(bloom_api_anim_key_id(key), target, stiffness, damping);
}

static bloom_f32 bloom_api_anim_delay(const char *key, bloom_bool active,
                                      bloom_f32 delay, bloom_f32 response)
{
    return bloom_anim_delay(bloom_api_anim_key_id(key), active, delay, response);
}

static bloom_f32 bloom_api_anim_stagger(const char *key, bloom_bool active, bloom_i32 index,
                                        bloom_f32 step_delay, bloom_f32 response)
{
    return bloom_anim_stagger(bloom_api_anim_key_id(key), active, index, step_delay, response);
}

static bloom_f32 bloom_api_anim_loop(bloom_f32 speed, bloom_f32 offset)
{
    return bloom_anim_loop(speed, offset);
}

static bloom_f32 bloom_api_anim_ping_pong(bloom_f32 speed, bloom_f32 offset)
{
    return bloom_anim_ping_pong(speed, offset);
}

static bloom_f32 bloom_api_anim_sine(bloom_f32 speed, bloom_f32 offset)
{
    return bloom_anim_sine(speed, offset);
}

static void bloom_api_anim_reset(const char *key)
{
    bloom_anim_reset(bloom_api_anim_key_id(key));
}

static void bloom_api_anim_reset_all(void)
{
    bloom_anim_reset_all();
}

static bloom_f32 bloom_api_clamp_f32(bloom_f32 value, bloom_f32 min_value, bloom_f32 max_value)
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

static bloom_corner_radii bloom_api_normalize_corner_radii(bloom_rect rect, bloom_corner_radii radii)
{
    bloom_f32 scale = 1.0f;
    bloom_f32 edge_sum;

    radii.top_left = bloom_api_clamp_f32(radii.top_left, 0.0f, rect.w * 0.5f);
    radii.top_right = bloom_api_clamp_f32(radii.top_right, 0.0f, rect.w * 0.5f);
    radii.bottom_right = bloom_api_clamp_f32(radii.bottom_right, 0.0f, rect.w * 0.5f);
    radii.bottom_left = bloom_api_clamp_f32(radii.bottom_left, 0.0f, rect.w * 0.5f);

    radii.top_left = bloom_api_clamp_f32(radii.top_left, 0.0f, rect.h * 0.5f);
    radii.top_right = bloom_api_clamp_f32(radii.top_right, 0.0f, rect.h * 0.5f);
    radii.bottom_right = bloom_api_clamp_f32(radii.bottom_right, 0.0f, rect.h * 0.5f);
    radii.bottom_left = bloom_api_clamp_f32(radii.bottom_left, 0.0f, rect.h * 0.5f);

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

static void bloom_api_build_corner_points(bloom_rect rect, bloom_corner_radii radii,
                                          int segments, bloom_vec2 *points, int *count)
{
    bloom_vec2 centers[4];
    bloom_f32 corner_radii[4];
    bloom_f32 start_angles[4];
    int corner;
    int index = 0;

    centers[0] = bloom_v2(rect.x + radii.top_left, rect.y + radii.top_left);
    centers[1] = bloom_v2(rect.x + rect.w - radii.top_right, rect.y + radii.top_right);
    centers[2] = bloom_v2(rect.x + rect.w - radii.bottom_right, rect.y + rect.h - radii.bottom_right);
    centers[3] = bloom_v2(rect.x + radii.bottom_left, rect.y + rect.h - radii.bottom_left);

    corner_radii[0] = radii.top_left;
    corner_radii[1] = radii.top_right;
    corner_radii[2] = radii.bottom_right;
    corner_radii[3] = radii.bottom_left;

    start_angles[0] = (bloom_f32)M_PI;
    start_angles[1] = (bloom_f32)(M_PI * 1.5);
    start_angles[2] = 0.0f;
    start_angles[3] = (bloom_f32)(M_PI * 0.5);

    for (corner = 0; corner < 4; ++corner)
    {
        int step;
        for (step = 0; step <= segments; ++step)
        {
            bloom_f32 angle = start_angles[corner] + ((bloom_f32)step / (bloom_f32)segments) * ((bloom_f32)M_PI * 0.5f);
            bloom_f32 radius = corner_radii[corner];
            if (radius <= 0.0f)
            {
                switch (corner)
                {
                case 0: points[index] = bloom_v2(rect.x, rect.y); break;
                case 1: points[index] = bloom_v2(rect.x + rect.w, rect.y); break;
                case 2: points[index] = bloom_v2(rect.x + rect.w, rect.y + rect.h); break;
                default: points[index] = bloom_v2(rect.x, rect.y + rect.h); break;
                }
            }
            else
            {
                points[index] = bloom_v2(centers[corner].x + cosf(angle) * radius,
                                         centers[corner].y + sinf(angle) * radius);
            }
            index++;
        }
    }

    *count = index;
}

static int bloom_api_draw_segments(void)
{
    bloom_context *ctx = bloom_get_context();
    if (ctx && ctx->style.curve_segments > 2)
    {
        return ctx->style.curve_segments;
    }
    return 16;
}

static bloom_f32 bloom_api_polygon_area(const bloom_vec2 *points, bloom_i32 count)
{
    bloom_f32 area = 0.0f;
    bloom_i32 i;

    for (i = 0; i < count; ++i)
    {
        bloom_i32 next = (i + 1) % count;
        area += points[i].x * points[next].y - points[next].x * points[i].y;
    }

    return area * 0.5f;
}

static void bloom_api_draw_convex_points(bloom_draw_list *dl, const bloom_vec2 *points,
                                         bloom_i32 count, bloom_color col)
{
    bloom_context *ctx = bloom_get_context();
    bloom_i32 i;

    if (!dl || !points || count < 3 || col.a == 0)
    {
        return;
    }

    for (i = 1; i < count - 1; ++i)
    {
        bloom_draw_triangle(dl, points[0], points[i], points[i + 1], col);
    }

    if (ctx && ctx->style.antialias_shapes)
    {
        bloom_f32 area = bloom_api_polygon_area(points, count);

        for (i = 0; i < count; ++i)
        {
            bloom_i32 next = (i + 1) % count;
            bloom_f32 dx = points[next].x - points[i].x;
            bloom_f32 dy = points[next].y - points[i].y;
            bloom_f32 len = sqrtf(dx * dx + dy * dy);
            bloom_vec2 normal;

            if (len <= 0.0001f)
            {
                continue;
            }

            if (area >= 0.0f)
            {
                normal = bloom_v2(dy / len, -dx / len);
            }
            else
            {
                normal = bloom_v2(-dy / len, dx / len);
            }

            bloom_draw_line(dl,
                            bloom_v2(points[i].x + normal.x, points[i].y + normal.y),
                            bloom_v2(points[next].x + normal.x, points[next].y + normal.y),
                            col,
                            1.0f);
        }
    }
}

static void bloom_api_draw_rounded_fill(bloom_draw_list *dl, bloom_rect rect,
                                        bloom_corner_radii radii, bloom_color col)
{
    bloom_vec2 points[260];
    int count = 0;
    int segments = bloom_api_draw_segments();

    if (!dl || col.a == 0 || rect.w <= 0.0f || rect.h <= 0.0f)
    {
        return;
    }

    radii = bloom_api_normalize_corner_radii(rect, radii);
    bloom_api_build_corner_points(rect, radii, segments, points, &count);
    bloom_api_draw_convex_points(dl, points, count, col);
}

static void bloom_api_draw_rounded_ring(bloom_draw_list *dl, bloom_rect rect,
                                        bloom_corner_radii radii, bloom_color col,
                                        bloom_f32 thickness)
{
    bloom_vec2 outer_points[260];
    bloom_vec2 inner_points[260];
    bloom_rect inner_rect;
    bloom_corner_radii inner_radii;
    int segments = bloom_api_draw_segments();
    int count = 0;
    int i;

    if (!dl || col.a == 0 || thickness <= 0.0f || rect.w <= 0.0f || rect.h <= 0.0f)
    {
        return;
    }

    radii = bloom_api_normalize_corner_radii(rect, radii);
    inner_rect = bloom_make_rect(rect.x + thickness, rect.y + thickness,
                                 rect.w - thickness * 2.0f, rect.h - thickness * 2.0f);

    if (inner_rect.w <= 0.0f || inner_rect.h <= 0.0f)
    {
        bloom_api_draw_rounded_fill(dl, rect, radii, col);
        return;
    }

    inner_radii = radii;
    inner_radii.top_left = bloom_api_clamp_f32(inner_radii.top_left - thickness, 0.0f, inner_rect.w * 0.5f);
    inner_radii.top_right = bloom_api_clamp_f32(inner_radii.top_right - thickness, 0.0f, inner_rect.w * 0.5f);
    inner_radii.bottom_right = bloom_api_clamp_f32(inner_radii.bottom_right - thickness, 0.0f, inner_rect.w * 0.5f);
    inner_radii.bottom_left = bloom_api_clamp_f32(inner_radii.bottom_left - thickness, 0.0f, inner_rect.w * 0.5f);
    inner_radii = bloom_api_normalize_corner_radii(inner_rect, inner_radii);

    bloom_api_build_corner_points(rect, radii, segments, outer_points, &count);
    bloom_api_build_corner_points(inner_rect, inner_radii, segments, inner_points, &count);

    for (i = 0; i < count; ++i)
    {
        int next = (i + 1) % count;
        bloom_draw_triangle(dl, outer_points[i], outer_points[next], inner_points[next], col);
        bloom_draw_triangle(dl, outer_points[i], inner_points[next], inner_points[i], col);
    }
}

static void bloom_api_draw_rect_custom(bloom_rect rect, bloom_color fill_color,
                                       bloom_color border_color, bloom_f32 border_thickness,
                                       bloom_corner_radii radii)
{
    bloom_draw_list *dl = bloom_get_draw_list();

    if (!dl)
    {
        return;
    }

    bloom_draw_rect_custom(dl, rect, fill_color, border_color, border_thickness, radii);
}

static void bloom_api_draw_circle(bloom_vec2 center, bloom_f32 radius, bloom_color col,
                                  bloom_f32 thickness, int segments)
{
    bloom_draw_list *dl = bloom_get_draw_list();
    int i;
    bloom_vec2 prev;

    if (!dl || radius <= 0.0f || thickness <= 0.0f || col.a == 0)
    {
        return;
    }

    segments = segments > 0 ? segments : bloom_api_draw_segments() * 2;
    if (segments < 12) segments = 12;
    if (segments > 128) segments = 128;

    prev = bloom_v2(center.x + radius, center.y);
    for (i = 1; i <= segments; ++i)
    {
        bloom_f32 angle = ((bloom_f32)i / (bloom_f32)segments) * ((bloom_f32)M_PI * 2.0f);
        bloom_vec2 curr = bloom_v2(center.x + cosf(angle) * radius,
                                   center.y + sinf(angle) * radius);
        bloom_draw_line(dl, prev, curr, col, thickness);
        prev = curr;
    }
}

static void bloom_api_draw_polyline(const bloom_vec2 *points, bloom_i32 count,
                                    bloom_color col, bloom_f32 thickness, bloom_bool closed)
{
    bloom_draw_list *dl = bloom_get_draw_list();
    bloom_i32 i;

    if (!dl || !points || count < 2 || thickness <= 0.0f || col.a == 0)
    {
        return;
    }

    for (i = 0; i < count - 1; ++i)
    {
        bloom_draw_line(dl, points[i], points[i + 1], col, thickness);
    }

    if (closed && count > 2)
    {
        bloom_draw_line(dl, points[count - 1], points[0], col, thickness);
    }
}

static bloom_vec2 bloom_api_bezier_cubic_point(bloom_vec2 p0, bloom_vec2 p1,
                                               bloom_vec2 p2, bloom_vec2 p3,
                                               bloom_f32 t)
{
    bloom_f32 u = 1.0f - t;
    bloom_f32 tt = t * t;
    bloom_f32 uu = u * u;
    bloom_f32 uuu = uu * u;
    bloom_f32 ttt = tt * t;

    return bloom_v2(uuu * p0.x + 3.0f * uu * t * p1.x + 3.0f * u * tt * p2.x + ttt * p3.x,
                    uuu * p0.y + 3.0f * uu * t * p1.y + 3.0f * u * tt * p2.y + ttt * p3.y);
}

static void bloom_api_draw_bezier_cubic(bloom_vec2 p0, bloom_vec2 p1,
                                        bloom_vec2 p2, bloom_vec2 p3,
                                        bloom_color col, bloom_f32 thickness,
                                        bloom_i32 segments)
{
    bloom_draw_list *dl = bloom_get_draw_list();
    bloom_i32 i;
    bloom_vec2 prev;

    if (!dl || thickness <= 0.0f || col.a == 0)
    {
        return;
    }

    segments = segments > 0 ? segments : bloom_api_draw_segments() * 2;
    if (segments < 4) segments = 4;
    if (segments > 128) segments = 128;

    prev = p0;
    for (i = 1; i <= segments; ++i)
    {
        bloom_f32 t = (bloom_f32)i / (bloom_f32)segments;
        bloom_vec2 curr = bloom_api_bezier_cubic_point(p0, p1, p2, p3, t);
        bloom_draw_line(dl, prev, curr, col, thickness);
        prev = curr;
    }
}

static void bloom_api_draw_convex_fill_polygon(const bloom_vec2 *points, bloom_i32 count,
                                               bloom_color col)
{
    bloom_draw_list *dl = bloom_get_draw_list();
    if (dl)
    {
        bloom_api_draw_convex_points(dl, points, count, col);
    }
}

static const bloom_api g_bloom_api = {
    bloom_api_v2,
    bloom_api_rect_make,
    bloom_api_rgba,
    bloom_api_rgb,
    bloom_api_corner_radii,
    bloom_api_corner_radii_all,
    bloom_api_corner_radii_top,
    bloom_api_corner_radii_bottom,
    bloom_api_corner_radii_first,
    bloom_api_corner_radii_second,
    bloom_api_corner_radii_third,
    bloom_api_corner_radii_fourth,

    bloom_create_context,
    bloom_destroy_context,
    bloom_get_context,
    bloom_set_context,
    bloom_begin_frame,
    bloom_end_frame,
    bloom_set_display_size,
    bloom_set_delta_time,
    bloom_begin,
    bloom_begin_window,
    bloom_begin_args,
    bloom_begin_ex,
    bloom_begin_root,
    bloom_begin_root_rect,
    bloom_end,
    bloom_begin_child,
    bloom_begin_child_args,
    bloom_end_child,
    bloom_get_id,
    bloom_push_id,
    bloom_push_id_int,
    bloom_push_id_ptr,
    bloom_pop_id,
    bloom_set_next_window_pos,
    bloom_set_next_window_size,
    bloom_get_cursor_pos,
    bloom_set_cursor_pos,
    bloom_get_content_width,
    bloom_same_line,
    bloom_new_line,
    bloom_indent,
    bloom_unindent,
    bloom_separator,
    bloom_spacing,
    bloom_set_layout,
    bloom_get_style,
    bloom_get_input,
    bloom_get_draw_list,
    bloom_get_debug_info,
    bloom_show_debug_overlay,
    bloom_get_default_font,

    bloom_text,
    bloom_text_wrapped,
    bloom_text_colored,
    bloom_text_disabled,
    bloom_button,
    bloom_button_sized,
    bloom_button_mini,
    bloom_button_ghost,
    bloom_button_direction,
    bloom_choice_strip,
    bloom_list_bullet,
    bloom_checkbox,
    bloom_toggle,
    bloom_toggle_ex,
    bloom_radio_button,
    bloom_slider_float,
    bloom_slider_int,
    bloom_slider_float_ex,
    bloom_slider_int_ex,
    bloom_slider_float_bar,
    bloom_slider_int_bar,
    bloom_slider_float_tall,
    bloom_slider_int_tall,
    bloom_text_input,
    bloom_text_area,
    bloom_int_field,
    bloom_float_field,
    bloom_precise_field,
    bloom_value_field,
    bloom_int_scrub,
    bloom_float_scrub,
    bloom_int_span,
    bloom_float_span,
    bloom_combo_begin,
    bloom_combo_begin_ex,
    bloom_combo_begin_args,
    bloom_combo_end,
    bloom_combo_item,
    bloom_action_split_begin,
    bloom_action_split_end,
    bloom_action_split_item,
    bloom_filter_select_begin,
    bloom_filter_select_begin_args,
    bloom_filter_select_end,
    bloom_filter_select_item,
    bloom_multi_select_begin,
    bloom_multi_select_begin_args,
    bloom_multi_select_end,
    bloom_multi_select_item,
    bloom_color_edit3,
    bloom_color_edit4,
    bloom_color_edit_rgb,
    bloom_color_edit_rgba,
    bloom_color_pick_rgb,
    bloom_color_pick_rgba,
    bloom_color_swatch,
    bloom_begin_table,
    bloom_end_table,
    bloom_table_next_column,
    bloom_table_next_row,
    bloom_table_header,
    bloom_tooltip,
    bloom_set_tooltip,
    bloom_tree_node,
    bloom_tree_pop,
    bloom_collapsing_header,
    bloom_progress_bar,

    bloom_hyperlink,
    bloom_spinner,
    bloom_image,
    bloom_image_button,
    bloom_splitter,
    bloom_text_selectable,
    bloom_begin_flow,
    bloom_end_flow,

    {
        bloom_arena_init,
        bloom_arena_alloc,
        bloom_arena_reset,
        bloom_arena_remaining,
        bloom_pool_init,
        bloom_pool_alloc,
        bloom_pool_free
    },
    {
        bloom_hash_str,
        bloom_hash_str_seed,
        bloom_hash_bytes
    },
    {
        bloom_input_begin,
        bloom_input_set_mouse_pos,
        bloom_input_set_mouse_button,
        bloom_input_set_mouse_wheel,
        bloom_input_set_key,
        bloom_input_add_char
    },
    {
        bloom_style_default,
        bloom_style_material_dark,
        bloom_style_material_light
    },
    {
        bloom_font_init,
        bloom_font_build_default,
        bloom_font_load_from_memory,
        bloom_font_destroy,
        bloom_font_set_active,
        bloom_font_get_active,
        bloom_font_text_width,
        bloom_font_char_width
    },
    {
        bloom_create_opengl_backend,
        bloom_destroy_opengl_backend
    },
    {
        bloom_platform_create,
        bloom_platform_destroy,
        bloom_platform_poll,
        bloom_platform_swap,
        bloom_platform_get_size,
        bloom_platform_get_opacity,
        bloom_platform_set_opacity,
        bloom_platform_get_time,
        bloom_platform_get_clipboard_text,
        bloom_platform_set_clipboard_text
    },
    {
        bloom_api_draw_list,
        bloom_api_draw_list_for_layer,
        bloom_api_draw_get_layer,
        bloom_api_draw_set_layer,
        bloom_api_draw_reset_layer,
        bloom_api_draw_line,
        bloom_api_draw_triangle,
        bloom_api_draw_rect,
        bloom_api_draw_rect_filled,
        bloom_api_draw_rect_rounded,
        bloom_api_draw_rect_rounded_border,
        bloom_api_draw_rect_custom,
        bloom_api_draw_circle,
        bloom_api_draw_circle_filled,
        bloom_api_draw_polyline,
        bloom_api_draw_bezier_cubic,
        bloom_api_draw_convex_fill_polygon,
        bloom_api_draw_text
    },
    {
        bloom_api_anim_clamp01,
        bloom_api_anim_ease,
        bloom_api_anim_value,
        bloom_api_anim_toggle,
        bloom_api_anim_range,
        bloom_api_anim_pulse,
        bloom_api_anim_spring,
        bloom_api_anim_delay,
        bloom_api_anim_stagger,
        bloom_api_anim_loop,
        bloom_api_anim_ping_pong,
        bloom_api_anim_sine,
        bloom_api_anim_reset,
        bloom_api_anim_reset_all
    }
};

const bloom_api *bloom = &g_bloom_api;
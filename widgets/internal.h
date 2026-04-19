#ifndef BLOOM_WIDGETS_INTERNAL_H
#define BLOOM_WIDGETS_INTERNAL_H

#include "widgets/api.h"
#include "core/runtime/context/context.h"
#include "platform/api.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

void bloom_advance_layout(bloom_f32 w, bloom_f32 h);

bloom_bool bloom_widget_hovered(bloom_rect rect);
bloom_color bloom_color_mix(bloom_color a, bloom_color b, bloom_f32 t);
bloom_color bloom_apply_state_layer(bloom_color base, bloom_color state, bloom_f32 amount);
bloom_f32 bloom_lerp_f32(bloom_f32 a, bloom_f32 b, bloom_f32 t);
bloom_f32 bloom_table_row_height(bloom_context *ctx);
bloom_u32 bloom_label_visible_length(const char *label);
bloom_f32 bloom_scaled_line_height(bloom_context *ctx, bloom_f32 font_size);
bloom_f32 bloom_centered_text_y(bloom_context *ctx, bloom_f32 top, bloom_f32 height, bloom_f32 font_size);
bloom_bool bloom_window_animations_enabled(bloom_context *ctx);
bloom_f32 bloom_window_anim_toggle(bloom_context *ctx, bloom_id id, bloom_bool on, bloom_f32 response);
bloom_f32 bloom_window_anim_pulse(bloom_context *ctx, bloom_id id, bloom_bool active, bloom_f32 attack, bloom_f32 decay);
bloom_f32 bloom_window_anim_spring(bloom_context *ctx, bloom_id id, bloom_f32 target, bloom_f32 stiffness, bloom_f32 damping);
bloom_f32 bloom_window_anim_state(bloom_context *ctx, bloom_id id, bloom_f32 target, bloom_f32 response);
void bloom_draw_wrapped_text_lines(bloom_context *ctx, bloom_vec2 pos, const char *text,
                                   bloom_color col, bloom_f32 font_size,
                                   bloom_f32 max_width, bloom_f32 *out_width,
                                   bloom_f32 *out_height);
bloom_f32 bloom_label_width(bloom_context *ctx, const char *label, bloom_f32 font_size);
void bloom_draw_label(bloom_context *ctx, bloom_vec2 pos, const char *label, bloom_color col, bloom_f32 font_size);
bloom_bool bloom_numeric_input(const char *label, void *value_ptr, bloom_value_kind kind);

void bloom_buffer_clear(char *buf, bloom_u32 buf_size);
bloom_bool bloom_buffer_append(char *buf, bloom_u32 buf_size, const char *text);
bloom_color bloom_scale_alpha(bloom_color color, bloom_f32 scale);
void bloom_draw_soft_circle(bloom_context *ctx, bloom_vec2 center, bloom_f32 radius, bloom_color color);
void bloom_draw_soft_line(bloom_context *ctx, bloom_vec2 a, bloom_vec2 b, bloom_color color, bloom_f32 thickness);

bloom_toggle_args bloom_resolve_toggle_args(const bloom_toggle_args *args);
bloom_slider_args bloom_resolve_slider_args(const bloom_slider_args *args, bloom_bool show_grab);
bloom_bool bloom_widget_released_inside(bloom_context *ctx, bloom_id id, bloom_bool hovered);
bloom_bool bloom_widget_double_clicked(bloom_context *ctx, bloom_id id, bloom_bool hovered);

void bloom_popup_begin_deferred_draw(bloom_context *ctx);
void bloom_popup_end_deferred_draw(bloom_context *ctx);
void bloom_widgets_begin_frame(void);
void bloom_widgets_end_frame(void);
void bloom_tooltip_begin_frame(void);
void bloom_tooltip_flush_deferred(void);

extern bloom_bool g_popup_input_block;
extern bloom_rect g_popup_input_rect;
extern bloom_bool g_popup_persist_open;
extern bloom_rect g_popup_persist_rect;
extern bloom_bool g_popup_draw_redirect_active;

#ifdef __cplusplus
}
#endif

#endif

#ifndef BLOOM_H
#define BLOOM_H

#include "core/base/api.h"
#include "core/graphics/api.h"
#include "core/runtime/api.h"
#include "widgets/api.h"
#include "rendering/api.h"
#include "platform/api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bloom_memory_api
{
	void  (*arena_init)(bloom_arena *a, void *buffer, bloom_u64 size);
	void *(*arena_alloc)(bloom_arena *a, bloom_u64 size);
	void  (*arena_reset)(bloom_arena *a);
	bloom_u64 (*arena_remaining)(bloom_arena *a);

	void  (*pool_init)(bloom_pool *p, void *buffer, bloom_u64 chunk_size, bloom_u64 count);
	void *(*pool_alloc)(bloom_pool *p);
	void  (*pool_free)(bloom_pool *p, void *ptr);
} bloom_memory_api;

typedef struct bloom_hash_api
{
	bloom_id (*str)(const char *str);
	bloom_id (*str_seed)(const char *str, bloom_id seed);
	bloom_id (*bytes)(const void *data, bloom_u64 len, bloom_id seed);
} bloom_hash_api;

typedef struct bloom_input_api
{
	void (*begin)(bloom_input *input);
	void (*set_mouse_pos)(bloom_input *input, float x, float y);
	void (*set_mouse_button)(bloom_input *input, int button, bool down);
	void (*set_mouse_wheel)(bloom_input *input, float delta);
	void (*set_key)(bloom_input *input, int key, bool down);
	void (*add_char)(bloom_input *input, char c);
} bloom_input_api;

typedef struct bloom_style_module_api
{
	void (*default_theme)(bloom_style *style);
	void (*material_dark)(bloom_style *style);
	void (*material_light)(bloom_style *style);
} bloom_style_module_api;

typedef struct bloom_font_api
{
	void (*init)(bloom_font *font);
	bloom_bool (*build_default)(bloom_font *font, bloom_f32 size);
	bloom_bool (*load_from_memory)(bloom_font *font, const bloom_u8 *data, bloom_u64 size, bloom_f32 pixel_size);
	void (*destroy)(bloom_font *font);
	void (*set_active)(bloom_font *font);
	bloom_font *(*get_active)(void);
	bloom_f32 (*text_width)(bloom_font *font, const char *text);
	bloom_f32 (*char_width)(bloom_font *font, bloom_u32 codepoint);
} bloom_font_api;

typedef struct bloom_render_module_api
{
	bloom_render_backend *(*create_opengl_backend)(void);
	void (*destroy_opengl_backend)(bloom_render_backend *backend);
} bloom_render_module_api;

typedef struct bloom_platform_api
{
	bloom_platform_window *(*create)(bloom_platform_config *config);
	void (*destroy)(bloom_platform_window *win);
	bloom_bool (*poll)(bloom_platform_window *win);
	void (*swap)(bloom_platform_window *win);
	void (*get_size)(bloom_platform_window *win, bloom_i32 *w, bloom_i32 *h);
	bloom_f32 (*get_opacity)(bloom_platform_window *win);
	void (*set_opacity)(bloom_platform_window *win, bloom_f32 opacity);
	bloom_bool (*get_auto_dpi_awareness)(void);
	void (*set_auto_dpi_awareness)(bloom_bool enabled);
	bloom_f64 (*get_time)(void);
	bloom_bool (*get_clipboard_text)(char *buffer, bloom_u32 buffer_size);
	bloom_bool (*set_clipboard_text)(const char *text);
} bloom_platform_api;

typedef struct bloom_draw_utility_api
{
	bloom_draw_list *(*list)(void);
	bloom_draw_list *(*list_for_layer)(bloom_i32 layer);
	bloom_i32 (*get_layer)(void);
	void (*set_layer)(bloom_i32 layer);
	void (*reset_layer)(void);
	void (*line)(bloom_vec2 a, bloom_vec2 b, bloom_color col, bloom_f32 thickness);
	void (*triangle)(bloom_vec2 a, bloom_vec2 b, bloom_vec2 c, bloom_color col);
	void (*rect)(bloom_rect rect, bloom_color col, bloom_f32 thickness);
	void (*rect_filled)(bloom_rect rect, bloom_color col);
	void (*rect_rounded)(bloom_rect rect, bloom_color col, bloom_f32 radius);
	void (*rect_rounded_border)(bloom_rect rect, bloom_color col, bloom_f32 radius, bloom_f32 thickness);
	void (*rect_custom)(bloom_rect rect, bloom_color fill_color, bloom_color border_color, bloom_f32 border_thickness, bloom_corner_radii radii);
	void (*circle)(bloom_vec2 center, bloom_f32 radius, bloom_color col, bloom_f32 thickness, int segments);
	void (*circle_filled)(bloom_vec2 center, bloom_f32 radius, bloom_color col, int segments);
	void (*polyline)(const bloom_vec2 *points, bloom_i32 count, bloom_color col, bloom_f32 thickness, bloom_bool closed);
	void (*bezier_cubic)(bloom_vec2 p0, bloom_vec2 p1, bloom_vec2 p2, bloom_vec2 p3, bloom_color col, bloom_f32 thickness, bloom_i32 segments);
	void (*convex_fill)(const bloom_vec2 *points, bloom_i32 count, bloom_color col);
	void (*text)(bloom_vec2 pos, const char *text, bloom_color col, bloom_f32 font_size);
} bloom_draw_utility_api;

typedef struct bloom_animation_api
{
	bloom_f32 (*clamp01)(bloom_f32 t);
	bloom_f32 (*ease)(bloom_f32 t, bloom_anim_ease ease);
	bloom_f32 (*value)(const char *key, bloom_f32 target, bloom_f32 response);
	bloom_f32 (*toggle)(const char *key, bloom_bool on, bloom_f32 response);
	bloom_f32 (*range)(const char *key, bloom_f32 from, bloom_f32 to,
	                  bloom_f32 target_t, bloom_f32 response,
	                  bloom_anim_ease ease);
	bloom_f32 (*pulse)(const char *key, bloom_bool active,
	                  bloom_f32 attack, bloom_f32 decay);
	bloom_f32 (*spring)(const char *key, bloom_f32 target,
	                   bloom_f32 stiffness, bloom_f32 damping);
	bloom_f32 (*delay)(const char *key, bloom_bool active,
	                  bloom_f32 delay, bloom_f32 response);
	bloom_f32 (*stagger)(const char *key, bloom_bool active, bloom_i32 index,
	                    bloom_f32 step_delay, bloom_f32 response);
	bloom_f32 (*loop)(bloom_f32 speed, bloom_f32 offset);
	bloom_f32 (*ping_pong)(bloom_f32 speed, bloom_f32 offset);
	bloom_f32 (*sine)(bloom_f32 speed, bloom_f32 offset);
	void (*reset)(const char *key);
	void (*reset_all)(void);
} bloom_animation_api;

typedef struct bloom_api
{
	bloom_vec2 (*v2)(bloom_f32 x, bloom_f32 y);
	bloom_rect (*rect_make)(bloom_f32 x, bloom_f32 y, bloom_f32 w, bloom_f32 h);
	bloom_color (*rgba)(bloom_u8 r, bloom_u8 g, bloom_u8 b, bloom_u8 a);
	bloom_color (*rgb)(bloom_u8 r, bloom_u8 g, bloom_u8 b);
	bloom_corner_radii (*corner_radii)(bloom_f32 top_left, bloom_f32 top_right, bloom_f32 bottom_right, bloom_f32 bottom_left);
	bloom_corner_radii (*corner_radii_all)(bloom_f32 radius);
	bloom_corner_radii (*corner_radii_top)(bloom_f32 radius);
	bloom_corner_radii (*corner_radii_bottom)(bloom_f32 radius);
	bloom_corner_radii (*corner_radii_first)(bloom_f32 radius);
	bloom_corner_radii (*corner_radii_second)(bloom_f32 radius);
	bloom_corner_radii (*corner_radii_third)(bloom_f32 radius);
	bloom_corner_radii (*corner_radii_fourth)(bloom_f32 radius);

	bloom_context *(*create_context)(void);
	void (*destroy_context)(bloom_context *ctx);
	bloom_context *(*get_context)(void);
	void (*set_context)(bloom_context *ctx);
	void (*begin_frame)(void);
	void (*end_frame)(void);
	void (*set_display_size)(bloom_f32 w, bloom_f32 h);
	void (*set_delta_time)(bloom_f32 dt);
	bloom_bool (*begin)(const char *name);
	bloom_bool (*begin_window)(const char *name, bloom_bool border);
	bloom_bool (*begin_args)(const char *name, const bloom_window_args *args);
	bloom_bool (*begin_ex)(const char *name, bloom_i32 flags);
	bloom_bool (*begin_root)(const char *name);
	bloom_bool (*begin_root_rect)(const char *name, bloom_rect rect, bloom_f32 padding);
	void (*end)(void);
	bloom_bool (*begin_child)(const char *name, bloom_f32 w, bloom_f32 h);
	bloom_bool (*begin_child_args)(const char *name, bloom_f32 w, bloom_f32 h, const bloom_window_args *args);
	void (*end_child)(void);
	bloom_id (*get_id)(const char *str);
	void (*push_id)(const char *str);
	void (*push_id_int)(bloom_i32 n);
	void (*push_id_ptr)(const void *ptr);
	void (*pop_id)(void);
	void (*set_next_window_pos)(bloom_f32 x, bloom_f32 y);
	void (*set_next_window_size)(bloom_f32 w, bloom_f32 h);
	bloom_vec2 (*get_cursor_pos)(void);
	void (*set_cursor_pos)(bloom_f32 x, bloom_f32 y);
	bloom_f32 (*get_content_width)(void);
	void (*same_line)(void);
	void (*new_line)(void);
	void (*indent)(bloom_f32 amount);
	void (*unindent)(bloom_f32 amount);
	void (*separator)(void);
	void (*spacing)(void);
	void (*set_layout)(bloom_i32 type);
	bloom_style *(*get_style)(void);
	bloom_input *(*get_input)(void);
	bloom_draw_list *(*get_draw_list)(void);
	bloom_debug_info *(*get_debug_info)(void);
	void (*show_debug_overlay)(bloom_bool show);
	bloom_font *(*get_default_font)(void);

	void (*text)(const char *text);
	void (*text_wrapped)(const char *text);
	void (*text_colored)(const char *text, bloom_color col);
	void (*text_disabled)(const char *text);
	bloom_bool (*button)(const char *label);
	bloom_bool (*button_sized)(const char *label, bloom_f32 w, bloom_f32 h);
	bloom_bool (*button_mini)(const char *label);
	bloom_bool (*button_ghost)(const char *id, bloom_f32 w, bloom_f32 h);
	bloom_bool (*button_direction)(const char *id, bloom_direction direction);
	bloom_bool (*choice_strip)(const char *label, const char *const *items, bloom_i32 item_count, bloom_i32 *selected_index);
	void (*list_bullet)(const char *text);
	bloom_bool (*checkbox)(const char *label, bloom_bool *value);
	bloom_bool (*toggle)(const char *label, bloom_bool *value);
	bloom_bool (*toggle_ex)(const char *label, bloom_bool *value, const bloom_toggle_args *args);
	bloom_bool (*radio_button)(const char *label, bloom_bool active);
	bloom_bool (*slider_float)(const char *label, bloom_f32 *value, bloom_f32 min_val, bloom_f32 max_val);
	bloom_bool (*slider_int)(const char *label, bloom_i32 *value, bloom_i32 min_val, bloom_i32 max_val);
	bloom_bool (*slider_float_ex)(const char *label, bloom_f32 *value, bloom_f32 min_val, bloom_f32 max_val,
	                           const bloom_slider_args *args);
	bloom_bool (*slider_int_ex)(const char *label, bloom_i32 *value, bloom_i32 min_val, bloom_i32 max_val,
	                         const bloom_slider_args *args);
	bloom_bool (*slider_float_bar)(const char *label, bloom_f32 *value, bloom_f32 min_val, bloom_f32 max_val,
	                            const bloom_slider_args *args);
	bloom_bool (*slider_int_bar)(const char *label, bloom_i32 *value, bloom_i32 min_val, bloom_i32 max_val,
	                          const bloom_slider_args *args);
	bloom_bool (*slider_float_tall)(const char *label, bloom_f32 *value, bloom_f32 min_val, bloom_f32 max_val,
	                             bloom_f32 height);
	bloom_bool (*slider_int_tall)(const char *label, bloom_i32 *value, bloom_i32 min_val, bloom_i32 max_val,
	                           bloom_f32 height);
	bloom_bool (*text_input)(const char *label, char *buf, bloom_u32 buf_size);
	bloom_bool (*text_area)(const char *label, char *buf, bloom_u32 buf_size, bloom_i32 lines);
	bloom_bool (*int_field)(const char *label, bloom_i32 *value, bloom_i32 step);
	bloom_bool (*float_field)(const char *label, bloom_f32 *value, bloom_f32 step);
	bloom_bool (*precise_field)(const char *label, bloom_f64 *value, bloom_f64 step);
	bloom_bool (*value_field)(const char *label, void *value, bloom_value_kind kind, bloom_f64 step);
	bloom_bool (*int_scrub)(const char *label, bloom_i32 *value, bloom_i32 min_val, bloom_i32 max_val,
	                    bloom_f32 step_per_pixel);
	bloom_bool (*float_scrub)(const char *label, bloom_f32 *value, bloom_f32 min_val, bloom_f32 max_val,
	                      bloom_f32 step_per_pixel);
	bloom_bool (*int_span)(const char *label, bloom_i32 *min_value, bloom_i32 *max_value, bloom_i32 step);
	bloom_bool (*float_span)(const char *label, bloom_f32 *min_value, bloom_f32 *max_value, bloom_f32 step);
	bloom_bool (*combo_begin)(const char *label, const char *preview);
	bloom_bool (*combo_begin_ex)(const char *label, const char *preview, bloom_i32 visible_items);
	bloom_bool (*combo_begin_args)(const char *label, const char *preview, const bloom_combo_args *args);
	void (*combo_end)(void);
	bloom_bool (*combo_item)(const char *label, bloom_bool selected);
	bloom_bool (*action_split_begin)(const char *label, bloom_bool *primary_pressed, bloom_i32 visible_items);
	void (*action_split_end)(void);
	bloom_bool (*action_split_item)(const char *label);
	bloom_bool (*filter_select_begin)(const char *label, const char *preview,
	                               char *filter_buf, bloom_u32 filter_buf_size,
	                               bloom_i32 visible_items);
	bloom_bool (*filter_select_begin_args)(const char *label, const char *preview,
	                                    char *filter_buf, bloom_u32 filter_buf_size,
	                                    const bloom_combo_args *args);
	void (*filter_select_end)(void);
	bloom_bool (*filter_select_item)(const char *label, bloom_bool selected);
	bloom_bool (*multi_select_begin)(const char *label, const char *preview, bloom_i32 visible_items);
	bloom_bool (*multi_select_begin_args)(const char *label, const char *preview, const bloom_combo_args *args);
	void (*multi_select_end)(void);
	bloom_bool (*multi_select_item)(const char *label, bloom_bool selected);
	bloom_bool (*color_edit3)(const char *label, bloom_f32 col[3]);
	bloom_bool (*color_edit4)(const char *label, bloom_f32 col[4]);
	bloom_bool (*color_edit_rgb)(const char *label, bloom_f32 col[3], bloom_u32 flags);
	bloom_bool (*color_edit_rgba)(const char *label, bloom_f32 col[4], bloom_u32 flags);
	bloom_bool (*color_pick_rgb)(const char *label, bloom_f32 col[3], bloom_u32 flags);
	bloom_bool (*color_pick_rgba)(const char *label, bloom_f32 col[4], bloom_u32 flags);
	bloom_bool (*color_swatch)(const char *label, const bloom_f32 col[4], bloom_f32 w, bloom_f32 h);
	void (*begin_table)(const char *label, bloom_i32 columns);
	void (*end_table)(void);
	void (*table_next_column)(void);
	void (*table_next_row)(void);
	void (*table_header)(const char *label);
	void (*tooltip)(const char *text);
	void (*set_tooltip)(const char *text);
	bloom_bool (*tree_node)(const char *label);
	void (*tree_pop)(void);
	bloom_bool (*collapsing_header)(const char *label);
	void (*progress_bar)(bloom_f32 fraction, bloom_f32 w, bloom_f32 h);

	bloom_bool (*hyperlink)(const char *label);
	void (*spinner)(const char *label, bloom_f32 radius);
	void (*image)(bloom_u32 texture_id, bloom_f32 w, bloom_f32 h);
	bloom_bool (*image_button)(const char *label, bloom_u32 texture_id, bloom_f32 w, bloom_f32 h);
	bloom_bool (*splitter)(bloom_bool vertical, bloom_f32 thickness, bloom_f32 *size1, bloom_f32 *size2,
	                       bloom_f32 min_size1, bloom_f32 min_size2);
	void (*text_selectable)(const char *text);
	void (*begin_flow)(void);
	void (*end_flow)(void);

	bloom_memory_api memory;
	bloom_hash_api hash;
	bloom_input_api input;
	bloom_style_module_api style;
	bloom_font_api font;
	bloom_render_module_api render;
	bloom_platform_api platform;
	bloom_draw_utility_api draw;
	bloom_animation_api anim;
} bloom_api;

extern const bloom_api *bloom;

#ifdef __cplusplus
}
#endif

#endif

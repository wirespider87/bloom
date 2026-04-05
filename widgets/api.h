#ifndef BLOOM_WIDGETS_API_H
#define BLOOM_WIDGETS_API_H

#include "core/base/api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bloom_slider_args
{
	bloom_bool show_grab;
	bloom_f32  animation_response;
} bloom_slider_args;

typedef struct bloom_toggle_args
{
	bloom_f32 animation_response;
} bloom_toggle_args;

typedef struct bloom_combo_args
{
	bloom_i32  visible_items;
	bloom_bool smooth_scroll;
} bloom_combo_args;

typedef enum bloom_direction
{
	BLOOM_DIRECTION_LEFT = 0,
	BLOOM_DIRECTION_RIGHT,
	BLOOM_DIRECTION_UP,
	BLOOM_DIRECTION_DOWN
} bloom_direction;

typedef enum bloom_value_kind
{
	BLOOM_VALUE_KIND_INT = 0,
	BLOOM_VALUE_KIND_FLOAT,
	BLOOM_VALUE_KIND_DOUBLE
} bloom_value_kind;

enum
{
	BLOOM_COLOR_FLAGS_NONE = 0,
	BLOOM_COLOR_FLAGS_NO_RGB = (1 << 0),
	BLOOM_COLOR_FLAGS_NO_HEX = (1 << 1),
	BLOOM_COLOR_FLAGS_NO_HSV = (1 << 2),
	BLOOM_COLOR_FLAGS_NO_LAB = (1 << 3),
	BLOOM_COLOR_FLAGS_NO_ALPHA = (1 << 4),
	BLOOM_COLOR_FLAGS_NO_PICKER = (1 << 5)
};

#define BLOOM_SLIDER_ARGS_DEFAULT { BLOOM_TRUE, 16.0f }
#define BLOOM_TOGGLE_ARGS_DEFAULT { 14.0f }
#define BLOOM_COMBO_ARGS_DEFAULT  { 6, BLOOM_TRUE }

#if defined(BLOOM_INTERNAL_BUILD) || defined(BLOOM_ENABLE_LEGACY_API)
void bloom_text(const char *text);
void bloom_text_wrapped(const char *text);
void bloom_text_colored(const char *text, bloom_color col);
void bloom_text_disabled(const char *text);

bloom_bool bloom_button(const char *label);
bloom_bool bloom_button_sized(const char *label, bloom_f32 w, bloom_f32 h);
bloom_bool bloom_button_mini(const char *label);
bloom_bool bloom_button_ghost(const char *id, bloom_f32 w, bloom_f32 h);
bloom_bool bloom_button_direction(const char *id, bloom_direction direction);
bloom_bool bloom_choice_strip(const char *label, const char *const *items, bloom_i32 item_count, bloom_i32 *selected_index);

void bloom_list_bullet(const char *text);

bloom_bool bloom_checkbox(const char *label, bloom_bool *value);
bloom_bool bloom_toggle(const char *label, bloom_bool *value);
bloom_bool bloom_toggle_ex(const char *label, bloom_bool *value, const bloom_toggle_args *args);
bloom_bool bloom_radio_button(const char *label, bloom_bool active);

bloom_bool bloom_slider_float(const char *label, bloom_f32 *value, bloom_f32 min_val, bloom_f32 max_val);
bloom_bool bloom_slider_int(const char *label, bloom_i32 *value, bloom_i32 min_val, bloom_i32 max_val);
bloom_bool bloom_slider_float_ex(const char *label, bloom_f32 *value, bloom_f32 min_val, bloom_f32 max_val,
								 const bloom_slider_args *args);
bloom_bool bloom_slider_int_ex(const char *label, bloom_i32 *value, bloom_i32 min_val, bloom_i32 max_val,
							   const bloom_slider_args *args);
bloom_bool bloom_slider_float_bar(const char *label, bloom_f32 *value, bloom_f32 min_val, bloom_f32 max_val,
								  const bloom_slider_args *args);
bloom_bool bloom_slider_int_bar(const char *label, bloom_i32 *value, bloom_i32 min_val, bloom_i32 max_val,
								const bloom_slider_args *args);
bloom_bool bloom_slider_float_tall(const char *label, bloom_f32 *value, bloom_f32 min_val, bloom_f32 max_val,
							   bloom_f32 height);
bloom_bool bloom_slider_int_tall(const char *label, bloom_i32 *value, bloom_i32 min_val, bloom_i32 max_val,
							 bloom_f32 height);

bloom_bool bloom_text_input(const char *label, char *buf, bloom_u32 buf_size);
bloom_bool bloom_text_area(const char *label, char *buf, bloom_u32 buf_size, bloom_i32 lines);

bloom_bool bloom_int_field(const char *label, bloom_i32 *value, bloom_i32 step);
bloom_bool bloom_float_field(const char *label, bloom_f32 *value, bloom_f32 step);
bloom_bool bloom_precise_field(const char *label, bloom_f64 *value, bloom_f64 step);
bloom_bool bloom_value_field(const char *label, void *value, bloom_value_kind kind, bloom_f64 step);

bloom_bool bloom_int_scrub(const char *label, bloom_i32 *value, bloom_i32 min_val, bloom_i32 max_val,
							 bloom_f32 step_per_pixel);
bloom_bool bloom_float_scrub(const char *label, bloom_f32 *value, bloom_f32 min_val, bloom_f32 max_val,
							   bloom_f32 step_per_pixel);

bloom_bool bloom_int_span(const char *label, bloom_i32 *min_value, bloom_i32 *max_value, bloom_i32 step);
bloom_bool bloom_float_span(const char *label, bloom_f32 *min_value, bloom_f32 *max_value, bloom_f32 step);

bloom_bool bloom_combo_begin(const char *label, const char *preview);
bloom_bool bloom_combo_begin_ex(const char *label, const char *preview, bloom_i32 visible_items);
bloom_bool bloom_combo_begin_args(const char *label, const char *preview, const bloom_combo_args *args);
void bloom_combo_end(void);
bloom_bool bloom_combo_item(const char *label, bloom_bool selected);

bloom_bool bloom_action_split_begin(const char *label, bloom_bool *primary_pressed, bloom_i32 visible_items);
void bloom_action_split_end(void);
bloom_bool bloom_action_split_item(const char *label);

bloom_bool bloom_filter_select_begin(const char *label, const char *preview,
							   char *filter_buf, bloom_u32 filter_buf_size,
							   bloom_i32 visible_items);
bloom_bool bloom_filter_select_begin_args(const char *label, const char *preview,
								char *filter_buf, bloom_u32 filter_buf_size,
								const bloom_combo_args *args);
void bloom_filter_select_end(void);
bloom_bool bloom_filter_select_item(const char *label, bloom_bool selected);

bloom_bool bloom_multi_select_begin(const char *label, const char *preview, bloom_i32 visible_items);
bloom_bool bloom_multi_select_begin_args(const char *label, const char *preview, const bloom_combo_args *args);
void bloom_multi_select_end(void);
bloom_bool bloom_multi_select_item(const char *label, bloom_bool selected);

bloom_bool bloom_color_edit3(const char *label, bloom_f32 col[3]);
bloom_bool bloom_color_edit4(const char *label, bloom_f32 col[4]);
bloom_bool bloom_color_edit_rgb(const char *label, bloom_f32 col[3], bloom_u32 flags);
bloom_bool bloom_color_edit_rgba(const char *label, bloom_f32 col[4], bloom_u32 flags);
bloom_bool bloom_color_pick_rgb(const char *label, bloom_f32 col[3], bloom_u32 flags);
bloom_bool bloom_color_pick_rgba(const char *label, bloom_f32 col[4], bloom_u32 flags);
bloom_bool bloom_color_swatch(const char *label, const bloom_f32 col[4], bloom_f32 w, bloom_f32 h);

void bloom_begin_table(const char *label, bloom_i32 columns);
void bloom_end_table(void);
void bloom_table_next_column(void);
void bloom_table_next_row(void);
void bloom_table_header(const char *label);

void bloom_tooltip(const char *text);
void bloom_set_tooltip(const char *text);

bloom_bool bloom_tree_node(const char *label);
void bloom_tree_pop(void);

bloom_bool bloom_collapsing_header(const char *label);

void bloom_progress_bar(bloom_f32 fraction, bloom_f32 w, bloom_f32 h);

bloom_bool bloom_hyperlink(const char *label);
void bloom_spinner(const char *label, bloom_f32 radius);
void bloom_image(bloom_u32 texture_id, bloom_f32 w, bloom_f32 h);
bloom_bool bloom_image_button(const char *label, bloom_u32 texture_id, bloom_f32 w, bloom_f32 h);
bloom_bool bloom_splitter(bloom_bool vertical, bloom_f32 thickness, bloom_f32 *size1, bloom_f32 *size2,
                          bloom_f32 min_size1, bloom_f32 min_size2);
void bloom_text_selectable(const char *text);

void bloom_begin_flow(void);
void bloom_end_flow(void);
#endif

#ifdef __cplusplus
}
#endif

#endif

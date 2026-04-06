#ifndef BLOOM_CORE_RUNTIME_STYLE_H
#define BLOOM_CORE_RUNTIME_STYLE_H

#include "core/base/types/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLOOM_STYLE_ROUNDING_INHERIT (-1.0f)

typedef struct bloom_style
{
    bloom_color window_bg;
    bloom_color window_border;
    bloom_color title_bg;
    bloom_color title_bg_active;
    bloom_color title_text;

    bloom_color button_bg;
    bloom_color button_bg_hovered;
    bloom_color button_bg_active;
    bloom_color button_text;

    bloom_color slider_bg;
    bloom_color slider_grab;
    bloom_color slider_grab_active;

    bloom_color checkbox_bg;
    bloom_color checkbox_mark;
    bloom_color checkbox_border;

    bloom_color input_bg;
    bloom_color input_border;
    bloom_color input_text;
    bloom_color input_cursor;
    bloom_color input_selection;

    bloom_color dropdown_bg;
    bloom_color dropdown_border;
    bloom_color dropdown_item_hovered;

    bloom_color tooltip_bg;
    bloom_color tooltip_text;
    bloom_color tooltip_border;

    bloom_color text_default;
    bloom_color text_disabled;

    bloom_color scrollbar_bg;
    bloom_color scrollbar_grab;
    bloom_color scrollbar_grab_hovered;

    bloom_color separator;
    bloom_color shadow;

    bloom_color table_header_bg;
    bloom_color table_row_bg;
    bloom_color table_row_bg_alt;
    bloom_color table_border;

    bloom_f32   window_rounding;
    bloom_f32   button_rounding;
    bloom_f32   slider_rounding;
    bloom_f32   input_rounding;
    bloom_f32   scrollbar_rounding;

    bloom_f32   window_padding;
    bloom_f32   item_spacing;
    bloom_f32   item_inner_spacing;
    bloom_f32   indent_spacing;
    bloom_f32   scrollbar_width;
    bloom_f32   scrollbar_inset;
    bloom_f32   title_bar_height;
    bloom_f32   title_bar_rounding;
    bloom_f32   title_bar_bottom_rounding;
    bloom_f32   border_width;
    bloom_f32   shadow_offset;
    bloom_f32   shadow_alpha;
    bloom_f32   color_preview_rounding;
    bloom_f32   color_preview_border_width;

    bloom_f32   font_size;

    bloom_bool  antialias_shapes;
    bloom_bool  antialias_lines;
    bloom_bool  antialias_text;
    bloom_i32   curve_segments;
    bloom_i32   circle_segments;

    bloom_f32   control_padding_x;
    bloom_f32   control_padding_y;
    bloom_f32   field_padding_x;
    bloom_f32   field_padding_y;
    bloom_f32   label_gap;
    bloom_f32   touch_padding;
} bloom_style;

#if defined(BLOOM_INTERNAL_BUILD) || defined(BLOOM_ENABLE_LEGACY_API)
void bloom_style_default(bloom_style *style);
void bloom_style_material_dark(bloom_style *style);
void bloom_style_material_light(bloom_style *style);
void bloom_style_scale(bloom_style *style, bloom_f32 scale);
#endif

#ifdef __cplusplus
}
#endif

#endif

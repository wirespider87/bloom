#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "core/runtime/style/style.h"

void bloom_style_scale(bloom_style *style, bloom_f32 scale)
{
    if (!style)
    {
        return;
    }

    if (scale <= 0.0f || scale == 1.0f)
    {
        return;
    }

    style->window_rounding *= scale;
    style->button_rounding *= scale;
    style->slider_rounding *= scale;
    style->input_rounding *= scale;
    style->scrollbar_rounding *= scale;

    style->window_padding *= scale;
    style->item_spacing *= scale;
    style->item_inner_spacing *= scale;
    style->indent_spacing *= scale;
    style->scrollbar_width *= scale;
    style->scrollbar_inset *= scale;
    style->title_bar_height *= scale;
    if (style->title_bar_rounding != BLOOM_STYLE_ROUNDING_INHERIT)
    {
        style->title_bar_rounding *= scale;
    }
    if (style->title_bar_bottom_rounding != BLOOM_STYLE_ROUNDING_INHERIT)
    {
        style->title_bar_bottom_rounding *= scale;
    }
    style->border_width *= scale;
    style->shadow_offset *= scale;
    style->color_preview_rounding *= scale;
    style->color_preview_border_width *= scale;

    style->font_size *= scale;

    style->control_padding_x *= scale;
    style->control_padding_y *= scale;
    style->field_padding_x *= scale;
    style->field_padding_y *= scale;
    style->label_gap *= scale;
    style->touch_padding *= scale;
}

void bloom_style_default(bloom_style *style)
{
    bloom_style_material_dark(style);
}

void bloom_style_material_dark(bloom_style *style)
{
    style->window_bg          = bloom_rgba(28, 27, 31, 246);
    style->window_border      = bloom_rgba(73, 69, 79, 220);
    style->title_bg           = bloom_rgba(33, 31, 38, 255);
    style->title_bg_active    = bloom_rgba(43, 41, 48, 255);
    style->title_text         = bloom_rgba(230, 224, 233, 255);

    style->button_bg          = bloom_rgba(103, 80, 164, 255);
    style->button_bg_hovered  = bloom_rgba(118, 94, 185, 255);
    style->button_bg_active   = bloom_rgba(88, 70, 140, 255);
    style->button_text        = bloom_rgba(255, 251, 254, 255);

    style->slider_bg          = bloom_rgba(73, 69, 79, 255);
    style->slider_grab        = bloom_rgba(208, 188, 255, 255);
    style->slider_grab_active = bloom_rgba(234, 221, 255, 255);

    style->checkbox_bg        = bloom_rgba(33, 31, 38, 255);
    style->checkbox_mark      = bloom_rgba(208, 188, 255, 255);
    style->checkbox_border    = bloom_rgba(147, 143, 153, 255);

    style->input_bg           = bloom_rgba(33, 31, 38, 255);
    style->input_border       = bloom_rgba(147, 143, 153, 255);
    style->input_text         = bloom_rgba(230, 224, 233, 255);
    style->input_cursor       = bloom_rgba(208, 188, 255, 255);
    style->input_selection    = bloom_rgba(208, 188, 255, 72);

    style->dropdown_bg           = bloom_rgba(43, 41, 48, 255);
    style->dropdown_border       = bloom_rgba(73, 69, 79, 255);
    style->dropdown_item_hovered = bloom_rgba(208, 188, 255, 52);

    style->tooltip_bg         = bloom_rgba(54, 52, 60, 248);
    style->tooltip_text       = bloom_rgba(230, 224, 233, 255);
    style->tooltip_border     = bloom_rgba(73, 69, 79, 255);

    style->text_default       = bloom_rgba(230, 224, 233, 255);
    style->text_disabled      = bloom_rgba(147, 143, 153, 255);

    style->scrollbar_bg           = bloom_rgba(24, 24, 28, 144);
    style->scrollbar_grab         = bloom_rgba(121, 116, 126, 255);
    style->scrollbar_grab_hovered = bloom_rgba(147, 143, 153, 255);

    style->separator          = bloom_rgba(73, 69, 79, 180);
    style->shadow             = bloom_rgba(0, 0, 0, 120);

    style->table_header_bg    = bloom_rgba(43, 41, 48, 255);
    style->table_row_bg       = bloom_rgba(31, 30, 34, 255);
    style->table_row_bg_alt   = bloom_rgba(35, 34, 39, 255);
    style->table_border       = bloom_rgba(73, 69, 79, 255);

    style->window_rounding    = 24.0f;
    style->button_rounding    = 20.0f;
    style->slider_rounding    = 999.0f;
    style->input_rounding     = 16.0f;
    style->scrollbar_rounding = 999.0f;

    style->window_padding     = 24.0f;
    style->item_spacing       = 14.0f;
    style->item_inner_spacing = 8.0f;
    style->indent_spacing     = 20.0f;
    style->scrollbar_width    = 12.0f;
    style->scrollbar_inset    = 8.0f;
    style->title_bar_height   = 52.0f;
    style->title_bar_rounding = BLOOM_STYLE_ROUNDING_INHERIT;
    style->title_bar_bottom_rounding = BLOOM_STYLE_ROUNDING_INHERIT;
    style->border_width       = 1.0f;
    style->shadow_offset      = 16.0f;
    style->shadow_alpha       = 0.18f;
    style->color_preview_rounding = 12.0f;
    style->color_preview_border_width = 1.0f;

    style->font_size          = 16.0f;

    style->antialias_shapes   = BLOOM_TRUE;
    style->antialias_lines    = BLOOM_TRUE;
    style->antialias_text     = BLOOM_TRUE;
    style->curve_segments     = 28;
    style->circle_segments    = 64;

    style->control_padding_x  = 20.0f;
    style->control_padding_y  = 10.0f;
    style->field_padding_x    = 16.0f;
    style->field_padding_y    = 10.0f;
    style->label_gap          = 8.0f;
    style->touch_padding      = 8.0f;
}

void bloom_style_material_light(bloom_style *style)
{
    style->window_bg          = bloom_rgba(254, 247, 255, 250);
    style->window_border      = bloom_rgba(202, 196, 208, 220);
    style->title_bg           = bloom_rgba(247, 242, 250, 255);
    style->title_bg_active    = bloom_rgba(236, 230, 240, 255);
    style->title_text         = bloom_rgba(29, 27, 32, 255);

    style->button_bg          = bloom_rgba(101, 85, 143, 255);
    style->button_bg_hovered  = bloom_rgba(116, 92, 177, 255);
    style->button_bg_active   = bloom_rgba(88, 70, 140, 255);
    style->button_text        = bloom_rgba(255, 255, 255, 255);

    style->slider_bg          = bloom_rgba(202, 196, 208, 255);
    style->slider_grab        = bloom_rgba(101, 85, 143, 255);
    style->slider_grab_active = bloom_rgba(79, 55, 139, 255);

    style->checkbox_bg        = bloom_rgba(254, 247, 255, 255);
    style->checkbox_mark      = bloom_rgba(101, 85, 143, 255);
    style->checkbox_border    = bloom_rgba(121, 116, 126, 255);

    style->input_bg           = bloom_rgba(247, 242, 250, 255);
    style->input_border       = bloom_rgba(121, 116, 126, 255);
    style->input_text         = bloom_rgba(29, 27, 32, 255);
    style->input_cursor       = bloom_rgba(101, 85, 143, 255);
    style->input_selection    = bloom_rgba(101, 85, 143, 64);

    style->dropdown_bg           = bloom_rgba(255, 251, 254, 255);
    style->dropdown_border       = bloom_rgba(202, 196, 208, 255);
    style->dropdown_item_hovered = bloom_rgba(103, 80, 164, 34);

    style->tooltip_bg         = bloom_rgba(49, 48, 51, 240);
    style->tooltip_text       = bloom_rgba(244, 239, 244, 255);
    style->tooltip_border     = bloom_rgba(202, 196, 208, 255);

    style->text_default       = bloom_rgba(29, 27, 32, 255);
    style->text_disabled      = bloom_rgba(121, 116, 126, 255);

    style->scrollbar_bg           = bloom_rgba(247, 242, 250, 144);
    style->scrollbar_grab         = bloom_rgba(174, 169, 180, 255);
    style->scrollbar_grab_hovered = bloom_rgba(121, 116, 126, 255);

    style->separator          = bloom_rgba(202, 196, 208, 180);
    style->shadow             = bloom_rgba(0, 0, 0, 42);

    style->table_header_bg    = bloom_rgba(247, 242, 250, 255);
    style->table_row_bg       = bloom_rgba(255, 251, 254, 255);
    style->table_row_bg_alt   = bloom_rgba(250, 244, 252, 255);
    style->table_border       = bloom_rgba(202, 196, 208, 255);

    style->window_rounding    = 24.0f;
    style->button_rounding    = 20.0f;
    style->slider_rounding    = 999.0f;
    style->input_rounding     = 16.0f;
    style->scrollbar_rounding = 999.0f;

    style->window_padding     = 24.0f;
    style->item_spacing       = 14.0f;
    style->item_inner_spacing = 8.0f;
    style->indent_spacing     = 20.0f;
    style->scrollbar_width    = 12.0f;
    style->scrollbar_inset    = 8.0f;
    style->title_bar_height   = 52.0f;
    style->title_bar_rounding = BLOOM_STYLE_ROUNDING_INHERIT;
    style->title_bar_bottom_rounding = BLOOM_STYLE_ROUNDING_INHERIT;
    style->border_width       = 1.0f;
    style->shadow_offset      = 10.0f;
    style->shadow_alpha       = 0.12f;
    style->color_preview_rounding = 12.0f;
    style->color_preview_border_width = 1.0f;

    style->font_size          = 16.0f;

    style->antialias_shapes   = BLOOM_TRUE;
    style->antialias_lines    = BLOOM_TRUE;
    style->antialias_text     = BLOOM_TRUE;
    style->curve_segments     = 28;
    style->circle_segments    = 64;

    style->control_padding_x  = 20.0f;
    style->control_padding_y  = 10.0f;
    style->field_padding_x    = 16.0f;
    style->field_padding_y    = 10.0f;
    style->label_gap          = 8.0f;
    style->touch_padding      = 8.0f;
}

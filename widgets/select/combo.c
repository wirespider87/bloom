#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "widgets/internal.h"
typedef struct bloom_combo_popup_state
{
    bloom_bool open;
    bloom_bool multi_select;
    bloom_bool smooth_scroll;
    bloom_bool redraw_control_on_change;
    bloom_bool search_enabled;
    bloom_bool clip_active;
    bloom_bool parent_clip_active;
    bloom_bool close_requested;
    bloom_id id;
    bloom_u32 open_frame;
    bloom_u32 filter_buf_size;
    bloom_i32 visible_items;
    bloom_i32 item_count;
    bloom_i32 last_item_count;
    bloom_f32 item_height;
    bloom_f32 width;
    bloom_f32 scroll;
    bloom_f32 scroll_target;
    bloom_f32 max_scroll;
    bloom_f32 search_height;
    bloom_rect control_rect;
    bloom_rect popup_rect;
    bloom_rect viewport_rect;
    bloom_rect parent_clip_rect;
    bloom_vec2 item_origin;
    bloom_i32 preview_selected_count;
    bloom_i32 preview_selected_shown;
    bloom_bool preview_from_selection;
    bloom_bool selection_changed;
    char *filter_buf;
    char label_text[96];
    char preview_text[256];
} bloom_combo_popup_state;

static bloom_combo_popup_state g_combo_popup;

static bloom_combo_args bloom_resolve_combo_args(const bloom_combo_args *args, bloom_i32 fallback_visible_items)
{
    bloom_combo_args resolved = BLOOM_COMBO_ARGS_DEFAULT;
    if (args)
    {
        resolved = *args;
    }
    if (fallback_visible_items > 0)
    {
        resolved.visible_items = fallback_visible_items;
    }
    if (resolved.visible_items < 1)
    {
        resolved.visible_items = 1;
    }
    return resolved;
}

static bloom_f32 bloom_clamp_f32(bloom_f32 value, bloom_f32 min_value, bloom_f32 max_value)
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

static char bloom_ascii_lower(char ch)
{
    if (ch >= 'A' && ch <= 'Z')
    {
        return (char)(ch - 'A' + 'a');
    }
    return ch;
}

static bloom_bool bloom_visible_label_contains(const char *label, const char *filter)
{
    bloom_u32 visible_len;
    bloom_u32 filter_len;
    bloom_u32 i;

    if (!filter || filter[0] == '\0')
    {
        return BLOOM_TRUE;
    }
    if (!label)
    {
        return BLOOM_FALSE;
    }

    visible_len = bloom_label_visible_length(label);
    filter_len = (bloom_u32)strlen(filter);
    if (filter_len > visible_len)
    {
        return BLOOM_FALSE;
    }

    for (i = 0; i + filter_len <= visible_len; ++i)
    {
        bloom_u32 j;
        bloom_bool match = BLOOM_TRUE;

        for (j = 0; j < filter_len; ++j)
        {
            if (bloom_ascii_lower(label[i + j]) != bloom_ascii_lower(filter[j]))
            {
                match = BLOOM_FALSE;
                break;
            }
        }

        if (match)
        {
            return BLOOM_TRUE;
        }
    }

    return BLOOM_FALSE;
}

static void bloom_buffer_copy(char *buf, bloom_u32 buf_size, const char *text)
{
    bloom_buffer_clear(buf, buf_size);
    if (text && text[0] != '\0')
    {
        bloom_buffer_append(buf, buf_size, text);
    }
}

static void bloom_buffer_append_visible_label(char *buf, bloom_u32 buf_size, const char *label)
{
    bloom_u32 len;
    bloom_u32 avail;
    bloom_u32 visible_len;
    bloom_u32 i;

    if (!buf || !label || buf_size == 0)
    {
        return;
    }

    len = (bloom_u32)strlen(buf);
    if (len >= buf_size - 1)
    {
        return;
    }

    avail = (buf_size - 1) - len;
    visible_len = bloom_label_visible_length(label);
    for (i = 0; i < visible_len && i < avail; ++i)
    {
        if ((unsigned char)label[i] < 32)
        {
            continue;
        }
        buf[len++] = label[i];
    }
    buf[len] = '\0';
}

static bloom_bool bloom_popup_control_hit(bloom_context *ctx, bloom_id id, bloom_rect rect,
                                          bloom_bool *hovered_out, bloom_bool *held_out)
{
    bloom_bool hovered = bloom_widget_hovered(rect);
    bloom_bool held = (ctx->active_id == id);
    bloom_bool pressed = BLOOM_FALSE;

    if (hovered)
    {
        ctx->hot_id = id;
        if (ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT])
        {
            ctx->active_id = id;
            held = BLOOM_TRUE;
        }
    }

    if (ctx->active_id == id)
    {
        held = BLOOM_TRUE;
        if (ctx->input.mouse_released[BLOOM_MOUSE_LEFT])
        {
            if (hovered)
            {
                pressed = BLOOM_TRUE;
            }
            ctx->active_id = 0;
            held = BLOOM_FALSE;
        }
    }

    if (hovered_out)
    {
        *hovered_out = hovered;
    }
    if (held_out)
    {
        *held_out = held;
    }
    return pressed;
}

static void bloom_combo_draw_control(bloom_context *ctx, const char *label, const char *preview,
                                     bloom_rect combo_rect, bloom_bool popup_open)
{
    bloom_style *s;
    bloom_f32 label_h;
    bloom_f32 preview_y;
    bloom_vec2 arrow_center;

    if (!ctx)
    {
        return;
    }

    s = &ctx->style;
    label_h = bloom_scaled_line_height(ctx, s->font_size * 0.9f);

    bloom_draw_label(ctx,
                     bloom_v2(combo_rect.x, combo_rect.y - s->label_gap - label_h),
                     label,
                     popup_open ? s->input_cursor : s->text_disabled,
                     s->font_size * 0.9f);

    bloom_draw_rect_rounded(&ctx->draw_list, combo_rect, s->dropdown_bg, s->input_rounding);
    bloom_draw_rect_rounded_border(&ctx->draw_list, combo_rect,
                                   popup_open ? s->input_cursor : s->dropdown_border,
                                   s->input_rounding, 1.0f);

    if (preview && preview[0] != '\0')
    {
        preview_y = bloom_centered_text_y(ctx, combo_rect.y, combo_rect.h, s->font_size);
        bloom_draw_text(&ctx->draw_list,
                        bloom_v2(combo_rect.x + s->field_padding_x, preview_y),
                        preview,
                        s->text_default,
                        s->font_size,
                        ctx->default_font.texture_id);
    }

    arrow_center = bloom_v2(combo_rect.x + combo_rect.w - 14.0f,
                            combo_rect.y + combo_rect.h * 0.5f);
    if (popup_open)
    {
        bloom_draw_triangle(&ctx->draw_list,
                            bloom_v2(arrow_center.x - 4.0f, arrow_center.y + 2.0f),
                            bloom_v2(arrow_center.x + 4.0f, arrow_center.y + 2.0f),
                            bloom_v2(arrow_center.x, arrow_center.y - 3.0f),
                            s->text_default);
    }
    else
    {
        bloom_draw_triangle(&ctx->draw_list,
                            bloom_v2(arrow_center.x - 4.0f, arrow_center.y - 2.0f),
                            bloom_v2(arrow_center.x + 4.0f, arrow_center.y - 2.0f),
                            bloom_v2(arrow_center.x, arrow_center.y + 3.0f),
                            s->text_default);
    }
}

static void bloom_action_split_draw_control(bloom_context *ctx, const char *label,
                                            bloom_rect control_rect, bloom_bool popup_open,
                                            bloom_bool primary_hovered, bloom_bool primary_held,
                                            bloom_bool arrow_hovered, bloom_bool arrow_held)
{
    bloom_style *s;
    bloom_f32 arrow_w;
    bloom_rect primary_rect;
    bloom_rect arrow_rect;
    bloom_color primary_bg;
    bloom_color arrow_bg;
    bloom_color border;
    bloom_f32 text_w;
    bloom_vec2 arrow_center;

    if (!ctx)
    {
        return;
    }

    s = &ctx->style;
    arrow_w = control_rect.h;
    primary_rect = bloom_make_rect(control_rect.x, control_rect.y, control_rect.w - arrow_w - 1.0f, control_rect.h);
    arrow_rect = bloom_make_rect(primary_rect.x + primary_rect.w + 1.0f, control_rect.y, arrow_w, control_rect.h);
    primary_bg = primary_held ? s->button_bg_active : (primary_hovered ? s->button_bg_hovered : s->button_bg);
    arrow_bg = popup_open || arrow_held ? s->button_bg_active : (arrow_hovered ? s->button_bg_hovered : s->button_bg);
    border = popup_open ? s->input_cursor : s->dropdown_border;

    bloom_draw_rect_custom(&ctx->draw_list,
                           primary_rect,
                           primary_bg,
                           border,
                           1.0f,
                           bloom_make_corner_radii(s->button_rounding, 0.0f, 0.0f, s->button_rounding));
    bloom_draw_rect_custom(&ctx->draw_list,
                           arrow_rect,
                           arrow_bg,
                           border,
                           1.0f,
                           bloom_make_corner_radii(0.0f, s->button_rounding, s->button_rounding, 0.0f));

    text_w = bloom_label_width(ctx, label, s->font_size);
    bloom_draw_label(ctx,
                     bloom_v2(primary_rect.x + (primary_rect.w - text_w) * 0.5f,
                              bloom_centered_text_y(ctx, primary_rect.y, primary_rect.h, s->font_size)),
                     label,
                     s->button_text,
                     s->font_size);

    arrow_center = bloom_v2(arrow_rect.x + arrow_rect.w * 0.5f,
                            arrow_rect.y + arrow_rect.h * 0.5f);
    if (popup_open)
    {
        bloom_draw_triangle(&ctx->draw_list,
                            bloom_v2(arrow_center.x - 4.0f, arrow_center.y + 2.0f),
                            bloom_v2(arrow_center.x + 4.0f, arrow_center.y + 2.0f),
                            bloom_v2(arrow_center.x, arrow_center.y - 3.0f),
                            s->button_text);
    }
    else
    {
        bloom_draw_triangle(&ctx->draw_list,
                            bloom_v2(arrow_center.x - 4.0f, arrow_center.y - 2.0f),
                            bloom_v2(arrow_center.x + 4.0f, arrow_center.y - 2.0f),
                            bloom_v2(arrow_center.x, arrow_center.y + 3.0f),
                            s->button_text);
    }
}

static void bloom_combo_preview_note_item(const char *label, bloom_bool effective_selected)
{
    if (!g_combo_popup.open || !label)
    {
        return;
    }

    if (!effective_selected)
    {
        return;
    }

    if (g_combo_popup.multi_select)
    {
        if (!g_combo_popup.preview_from_selection)
        {
            bloom_buffer_clear(g_combo_popup.preview_text, (bloom_u32)sizeof(g_combo_popup.preview_text));
            g_combo_popup.preview_from_selection = BLOOM_TRUE;
        }

        g_combo_popup.preview_selected_count++;
        if (g_combo_popup.preview_selected_shown < 2)
        {
            if (g_combo_popup.preview_selected_shown > 0)
            {
                bloom_buffer_append(g_combo_popup.preview_text,
                                    (bloom_u32)sizeof(g_combo_popup.preview_text),
                                    ", ");
            }
            bloom_buffer_append_visible_label(g_combo_popup.preview_text,
                                              (bloom_u32)sizeof(g_combo_popup.preview_text),
                                              label);
            g_combo_popup.preview_selected_shown++;
        }
    }
    else
    {
        bloom_buffer_clear(g_combo_popup.preview_text, (bloom_u32)sizeof(g_combo_popup.preview_text));
        bloom_buffer_append_visible_label(g_combo_popup.preview_text,
                                          (bloom_u32)sizeof(g_combo_popup.preview_text),
                                          label);
        g_combo_popup.preview_from_selection = BLOOM_TRUE;
    }
}

static void bloom_combo_restore_parent_clip(bloom_context *ctx)
{
    (void)ctx;
    if (g_combo_popup.parent_clip_active)
    {
        g_combo_popup.parent_clip_active = BLOOM_FALSE;
    }
}

static void bloom_combo_suspend_parent_clip(bloom_context *ctx)
{
    (void)ctx;
}

static bloom_bool bloom_combo_activate_popup(bloom_context *ctx, const char *label, const char *preview,
                                             bloom_rect combo_rect, const bloom_combo_args *resolved,
                                             bloom_bool multi_select)
{
    bloom_style *s;
    bloom_f32 visible_h;
    bloom_color popup_bg;

    if (!ctx)
    {
        return BLOOM_FALSE;
    }

    s = &ctx->style;
    g_combo_popup.smooth_scroll = resolved->smooth_scroll;
    g_combo_popup.visible_items = resolved->visible_items;
    g_combo_popup.item_height = bloom_scaled_line_height(ctx, s->font_size) + s->field_padding_y * 2.0f;
    g_combo_popup.width = combo_rect.w;
    g_combo_popup.control_rect = combo_rect;
    g_combo_popup.preview_selected_count = 0;
    g_combo_popup.preview_selected_shown = 0;
    g_combo_popup.preview_from_selection = BLOOM_FALSE;
    g_combo_popup.selection_changed = BLOOM_FALSE;
    g_combo_popup.redraw_control_on_change = BLOOM_TRUE;
    g_combo_popup.search_enabled = BLOOM_FALSE;
    g_combo_popup.search_height = 0.0f;
    g_combo_popup.filter_buf = NULL;
    g_combo_popup.filter_buf_size = 0;
    bloom_buffer_copy(g_combo_popup.label_text, (bloom_u32)sizeof(g_combo_popup.label_text), label);
    bloom_buffer_copy(g_combo_popup.preview_text, (bloom_u32)sizeof(g_combo_popup.preview_text), preview);

    visible_h = g_combo_popup.item_height * (bloom_f32)resolved->visible_items;
    if (g_combo_popup.last_item_count > 0 && g_combo_popup.last_item_count < resolved->visible_items)
    {
        visible_h = g_combo_popup.item_height * (bloom_f32)g_combo_popup.last_item_count;
    }
    if (visible_h < g_combo_popup.item_height)
    {
        visible_h = g_combo_popup.item_height;
    }

    g_combo_popup.popup_rect = bloom_make_rect(combo_rect.x, combo_rect.y + combo_rect.h + 6.0f,
                                               combo_rect.w, visible_h + 16.0f);
    {
        bloom_bool reserve_scrollbar_lane = (g_combo_popup.last_item_count > resolved->visible_items) ? BLOOM_TRUE : BLOOM_FALSE;
        bloom_f32 viewport_w = g_combo_popup.popup_rect.w - 16.0f;
        if (reserve_scrollbar_lane)
        {
            viewport_w -= s->scrollbar_width + 4.0f;
        }
        g_combo_popup.viewport_rect = bloom_make_rect(g_combo_popup.popup_rect.x + 8.0f,
                                                      g_combo_popup.popup_rect.y + 8.0f,
                                                      viewport_w,
                                                      g_combo_popup.popup_rect.h - 16.0f);
    }
    g_combo_popup.item_origin = bloom_v2(g_combo_popup.viewport_rect.x, g_combo_popup.viewport_rect.y);
    g_combo_popup.item_count = 0;
    g_combo_popup.close_requested = BLOOM_FALSE;
    g_popup_input_block = BLOOM_TRUE;
    g_popup_input_rect = g_combo_popup.popup_rect;
    g_popup_persist_open = BLOOM_TRUE;
    g_popup_persist_rect = g_combo_popup.popup_rect;

    if (bloom_rect_contains(g_combo_popup.popup_rect, ctx->input.mouse_pos) &&
        ctx->input.mouse_wheel != 0.0f)
    {
        if (g_combo_popup.max_scroll > 0.0f)
        {
            g_combo_popup.scroll_target -= ctx->input.mouse_wheel * g_combo_popup.item_height;
            g_combo_popup.scroll_target = bloom_clamp_f32(g_combo_popup.scroll_target, 0.0f, g_combo_popup.max_scroll);
            if (!g_combo_popup.smooth_scroll)
            {
                g_combo_popup.scroll = g_combo_popup.scroll_target;
            }
        }
        ctx->input.mouse_wheel = 0.0f;
    }

    bloom_combo_suspend_parent_clip(ctx);
    bloom_popup_begin_deferred_draw(ctx);

    popup_bg = s->dropdown_bg;
    popup_bg.a = 255;

    if (s->shadow_offset > 0.0f)
    {
        bloom_color shadow_col = s->shadow;
        shadow_col.a = (bloom_u8)(s->shadow_alpha * 255);
        bloom_draw_rect_rounded(&ctx->draw_list,
                                bloom_make_rect(g_combo_popup.popup_rect.x, g_combo_popup.popup_rect.y + 2.0f,
                                                g_combo_popup.popup_rect.w, g_combo_popup.popup_rect.h),
                                shadow_col,
                                s->input_rounding + 2.0f);
    }

    bloom_draw_rect_rounded(&ctx->draw_list, g_combo_popup.popup_rect, popup_bg, s->input_rounding + 2.0f);
    bloom_draw_rect_rounded_border(&ctx->draw_list, g_combo_popup.popup_rect,
                                   s->dropdown_border, s->input_rounding + 2.0f, 1.0f);
    bloom_draw_push_clip(&ctx->draw_list, g_combo_popup.viewport_rect);
    g_combo_popup.clip_active = BLOOM_TRUE;
    return BLOOM_TRUE;
}

static void bloom_combo_prepare_search_field(void)
{
    bloom_context *ctx = bloom_get_context();
    bloom_window *win;
    bloom_layout saved_layout;
    bloom_f32 consumed_h;

    if (!ctx || !ctx->current_window || !g_combo_popup.search_enabled ||
        !g_combo_popup.filter_buf || g_combo_popup.filter_buf_size == 0)
    {
        return;
    }

    win = ctx->current_window;
    saved_layout = win->layout;
    win->layout.cursor = bloom_v2(g_combo_popup.viewport_rect.x, g_combo_popup.viewport_rect.y);
    win->layout.start = win->layout.cursor;
    win->layout.available_width = g_combo_popup.viewport_rect.w;
    win->layout.indent = 0.0f;
    win->layout.max_row_height = 0.0f;
    win->layout.type = BLOOM_LAYOUT_VERTICAL;
    win->layout.last_item_pos = win->layout.cursor;
    win->layout.last_item_size = bloom_v2(0.0f, 0.0f);

    {
        bloom_bool saved_input_block;
        bloom_rect saved_input_rect;
        bloom_id search_field_id;

        bloom_push_id(g_combo_popup.label_text);
        search_field_id = bloom_get_id("##search");

        if (g_combo_popup.open_frame == ctx->frame_count)
        {
            ctx->focus_id = search_field_id;
        }

        saved_input_block   = g_popup_input_block;
        saved_input_rect    = g_popup_input_rect;
        g_popup_input_block = BLOOM_FALSE;

        bloom_text_input("##search", g_combo_popup.filter_buf, g_combo_popup.filter_buf_size);

        g_popup_input_block = saved_input_block;
        g_popup_input_rect  = saved_input_rect;

        bloom_pop_id();
    }

    consumed_h = g_combo_popup.item_height;

    win->layout = saved_layout;
    g_combo_popup.search_height = consumed_h;

    if (g_combo_popup.clip_active)
    {
        bloom_draw_pop_clip(&ctx->draw_list);
        g_combo_popup.clip_active = BLOOM_FALSE;
    }

    g_combo_popup.viewport_rect.y += consumed_h;
    g_combo_popup.viewport_rect.h -= consumed_h;
    if (g_combo_popup.viewport_rect.h < g_combo_popup.item_height)
    {
        g_combo_popup.viewport_rect.h = g_combo_popup.item_height;
    }
    g_combo_popup.item_origin = bloom_v2(g_combo_popup.viewport_rect.x, g_combo_popup.viewport_rect.y);
    bloom_draw_push_clip(&ctx->draw_list, g_combo_popup.viewport_rect);
    g_combo_popup.clip_active = BLOOM_TRUE;
}

static void bloom_combo_close_popup(bloom_context *ctx)
{
    bloom_i32 saved_last_item_count;

    if (ctx && g_combo_popup.clip_active)
    {
        bloom_draw_pop_clip(&ctx->draw_list);
        g_combo_popup.clip_active = BLOOM_FALSE;
    }
    if (ctx)
    {
        bloom_combo_restore_parent_clip(ctx);
        if (g_popup_draw_redirect_active)
        {
            bloom_popup_end_deferred_draw(ctx);
        }
    }

    g_popup_input_block = BLOOM_FALSE;
    g_popup_input_rect = bloom_make_rect(0.0f, 0.0f, 0.0f, 0.0f);
    g_popup_persist_open = BLOOM_FALSE;
    g_popup_persist_rect = bloom_make_rect(0.0f, 0.0f, 0.0f, 0.0f);

    saved_last_item_count = g_combo_popup.last_item_count;
    memset(&g_combo_popup, 0, sizeof(g_combo_popup));
    g_combo_popup.last_item_count = saved_last_item_count;
}

static bloom_bool bloom_combo_begin_internal(const char *label, const char *preview,
                                             const bloom_combo_args *args, bloom_bool multi_select)
{
    bloom_context *ctx = bloom_get_context();
    bloom_combo_args resolved;
    bloom_style *s;
    bloom_id id;
    bloom_vec2 pos;
    bloom_f32 label_h;
    bloom_f32 combo_w;
    bloom_f32 combo_h;
    bloom_rect combo_rect;
    bloom_bool hovered;
    bloom_bool popup_hovered;
    bloom_bool popup_open;

    if (!ctx || !ctx->current_window)
    {
        return BLOOM_FALSE;
    }

    resolved = bloom_resolve_combo_args(args, 0);
    s = &ctx->style;
    id = bloom_get_id(label);
    pos = ctx->current_window->layout.cursor;
    label_h = bloom_scaled_line_height(ctx, s->font_size * 0.9f);
    combo_w = ctx->current_window->layout.available_width;
    combo_h = bloom_scaled_line_height(ctx, s->font_size) + s->field_padding_y * 2.0f;
    combo_rect = bloom_make_rect(pos.x, pos.y + label_h + s->label_gap, combo_w, combo_h);
    hovered = bloom_widget_hovered(combo_rect);
    popup_hovered = g_combo_popup.open && g_combo_popup.id == id &&
                    bloom_rect_contains(g_combo_popup.popup_rect, ctx->input.mouse_pos);

    if (hovered && ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT])
    {
        if (g_combo_popup.open && g_combo_popup.id == id && g_combo_popup.multi_select == multi_select)
        {
            bloom_combo_close_popup(ctx);
        }
        else
        {
            bloom_i32 saved_count = g_combo_popup.last_item_count;
            memset(&g_combo_popup, 0, sizeof(g_combo_popup));
            g_combo_popup.last_item_count = saved_count;
            g_combo_popup.open = BLOOM_TRUE;
            g_combo_popup.multi_select = multi_select;
            g_combo_popup.smooth_scroll = resolved.smooth_scroll;
            g_combo_popup.id = id;
            g_combo_popup.open_frame = ctx->frame_count;
            g_combo_popup.visible_items = resolved.visible_items;
            g_combo_popup.item_height = bloom_scaled_line_height(ctx, s->font_size) + s->field_padding_y * 2.0f;
            g_combo_popup.width = combo_w;
            g_combo_popup.item_count = 0;
            if (g_combo_popup.last_item_count <= 0)
            {
                g_combo_popup.last_item_count = resolved.visible_items;
            }
            g_combo_popup.scroll = 0.0f;
            g_combo_popup.scroll_target = 0.0f;
        }
    }
    else if (g_combo_popup.open && g_combo_popup.id == id && ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT] &&
             !hovered && !popup_hovered && g_combo_popup.open_frame != ctx->frame_count)
    {
        bloom_combo_close_popup(ctx);
    }

    popup_open = g_combo_popup.open && g_combo_popup.id == id && g_combo_popup.multi_select == multi_select;

    bloom_combo_draw_control(ctx, label, preview, combo_rect, popup_open);

    bloom_advance_layout(combo_w, label_h + s->label_gap + combo_h + s->touch_padding);

    if (!popup_open)
    {
        return BLOOM_FALSE;
    }

    return bloom_combo_activate_popup(ctx, label, preview, combo_rect, &resolved, multi_select);
}

bloom_bool bloom_combo_begin(const char *label, const char *preview)
{
    return bloom_combo_begin_args(label, preview, NULL);
}

bloom_bool bloom_combo_begin_ex(const char *label, const char *preview, bloom_i32 visible_items)
{
    bloom_combo_args args = bloom_resolve_combo_args(NULL, visible_items);
    return bloom_combo_begin_internal(label, preview, &args, BLOOM_FALSE);
}

bloom_bool bloom_combo_begin_args(const char *label, const char *preview, const bloom_combo_args *args)
{
    bloom_combo_args resolved = bloom_resolve_combo_args(args, 0);
    return bloom_combo_begin_internal(label, preview, &resolved, BLOOM_FALSE);
}

static void bloom_combo_end_internal(void)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_f32 content_h;
    bloom_f32 track_h;
    bloom_f32 grab_h;
    bloom_f32 scroll_ratio;
    bloom_f32 grab_y;
    bloom_id scrollbar_id;
    bloom_rect track_rect;
    bloom_rect grab_rect;
    bloom_bool track_hovered;
    bloom_bool grab_hovered;

    if (!ctx || !g_combo_popup.open)
    {
        return;
    }

    s = &ctx->style;
    content_h = g_combo_popup.item_height * (bloom_f32)g_combo_popup.item_count;
    g_combo_popup.max_scroll = content_h - g_combo_popup.viewport_rect.h;
    if (g_combo_popup.max_scroll < 0.0f)
    {
        g_combo_popup.max_scroll = 0.0f;
    }
    g_combo_popup.scroll_target = bloom_clamp_f32(g_combo_popup.scroll_target, 0.0f, g_combo_popup.max_scroll);
    if (g_combo_popup.smooth_scroll)
    {
        g_combo_popup.scroll = bloom_window_anim_state(ctx, g_combo_popup.id ^ 0x4F7A31B9u, g_combo_popup.scroll_target, 17.0f);
    }
    else
    {
        g_combo_popup.scroll = g_combo_popup.scroll_target;
    }
    g_combo_popup.scroll = bloom_clamp_f32(g_combo_popup.scroll, 0.0f, g_combo_popup.max_scroll);
    g_combo_popup.last_item_count = g_combo_popup.item_count;
    g_popup_persist_open = BLOOM_TRUE;
    g_popup_persist_rect = g_combo_popup.popup_rect;

    if (g_combo_popup.clip_active)
    {
        bloom_draw_pop_clip(&ctx->draw_list);
        g_combo_popup.clip_active = BLOOM_FALSE;
    }

    if (g_combo_popup.max_scroll > 0.0f)
    {
        {
            /* start scrollbars below search bars. */
            bloom_f32 sb_top = g_combo_popup.popup_rect.y + s->scrollbar_inset + g_combo_popup.search_height;
            bloom_f32 sb_h   = g_combo_popup.popup_rect.h - s->scrollbar_inset * 2.0f - g_combo_popup.search_height;
            if (sb_h < 1.0f) { sb_h = 1.0f; }
            track_rect = bloom_make_rect(g_combo_popup.popup_rect.x + g_combo_popup.popup_rect.w - s->scrollbar_width - s->scrollbar_inset,
                                         sb_top,
                                         s->scrollbar_width,
                                         sb_h);
        }
        track_h = track_rect.h;
        if (track_h < 1.0f)
        {
            track_h = 1.0f;
            track_rect.h = 1.0f;
        }
        grab_h = track_h * (g_combo_popup.viewport_rect.h / content_h);
        if (grab_h < 18.0f)
        {
            grab_h = 18.0f;
        }
        scroll_ratio = (g_combo_popup.max_scroll > 0.0f) ? (g_combo_popup.scroll / g_combo_popup.max_scroll) : 0.0f;
        grab_y = track_rect.y + scroll_ratio * (track_h - grab_h);
        scrollbar_id = g_combo_popup.id ^ 0x4F7A31B9u;

        bloom_draw_rect_rounded(&ctx->draw_list, track_rect, s->scrollbar_bg, s->scrollbar_rounding);

        grab_rect = bloom_make_rect(track_rect.x,
                                    grab_y,
                                    track_rect.w,
                                    grab_h);

        track_hovered = bloom_rect_contains(track_rect, ctx->input.mouse_pos);
        grab_hovered = bloom_rect_contains(grab_rect, ctx->input.mouse_pos);

        if (ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT] && track_hovered)
        {
            ctx->active_id = scrollbar_id;
            if (grab_hovered)
            {
                ctx->drag_offset.y = ctx->input.mouse_pos.y - grab_rect.y;
            }
            else
            {
                ctx->drag_offset.y = grab_h * 0.5f;
            }
        }

        if (ctx->active_id == scrollbar_id)
        {
            if (ctx->input.mouse_down[BLOOM_MOUSE_LEFT])
            {
                bloom_f32 new_grab_y = ctx->input.mouse_pos.y - ctx->drag_offset.y;
                new_grab_y = bloom_clamp_f32(new_grab_y, track_rect.y, track_rect.y + track_h - grab_h);
                scroll_ratio = (track_h - grab_h) > 0.0f ? ((new_grab_y - track_rect.y) / (track_h - grab_h)) : 0.0f;
                g_combo_popup.scroll_target = scroll_ratio * g_combo_popup.max_scroll;
                g_combo_popup.scroll_target = bloom_clamp_f32(g_combo_popup.scroll_target, 0.0f, g_combo_popup.max_scroll);
                g_combo_popup.scroll = g_combo_popup.scroll_target;
                grab_rect.y = new_grab_y;
            }
            else
            {
                ctx->active_id = 0;
            }
        }

        bloom_draw_rect_rounded(&ctx->draw_list, grab_rect,
                                grab_hovered || ctx->active_id == scrollbar_id ? s->scrollbar_grab_hovered : s->scrollbar_grab,
                                s->scrollbar_rounding);
    }

    bloom_popup_end_deferred_draw(ctx);
    bloom_combo_restore_parent_clip(ctx);

    if (g_combo_popup.multi_select && g_combo_popup.preview_from_selection &&
        g_combo_popup.preview_selected_count > g_combo_popup.preview_selected_shown)
    {
        char suffix[32];
        snprintf(suffix, sizeof(suffix), " +%d",
                 g_combo_popup.preview_selected_count - g_combo_popup.preview_selected_shown);
        bloom_buffer_append(g_combo_popup.preview_text,
                            (bloom_u32)sizeof(g_combo_popup.preview_text),
                            suffix);
    }

    if (g_combo_popup.multi_select && g_combo_popup.selection_changed && !g_combo_popup.preview_from_selection)
    {
        bloom_buffer_clear(g_combo_popup.preview_text, (bloom_u32)sizeof(g_combo_popup.preview_text));
    }

    if (g_combo_popup.redraw_control_on_change && (g_combo_popup.selection_changed || g_combo_popup.preview_from_selection))
    {
        bloom_combo_draw_control(ctx,
                                 g_combo_popup.label_text,
                                 g_combo_popup.preview_text,
                                 g_combo_popup.control_rect,
                                 BLOOM_TRUE);
    }

    if (g_combo_popup.close_requested)
    {
        bloom_combo_close_popup(ctx);
    }
}

void bloom_combo_end(void)
{
    bloom_combo_end_internal();
}

static bloom_bool bloom_combo_item_internal(const char *label, bloom_bool selected, bloom_bool keep_open)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_i32 item_index;
    bloom_rect item_rect;
    bloom_bool hovered;
    bloom_bool clicked;
    bloom_bool effective_selected;
    bloom_id item_id;
    bloom_color bg;
    bloom_vec2 indicator_center;

    if (!ctx || !g_combo_popup.open || g_combo_popup.close_requested)
    {
        return BLOOM_FALSE;
    }

    s = &ctx->style;
    item_index = g_combo_popup.item_count++;
    item_id = g_combo_popup.id ^ (0x1900u + (bloom_id)item_index);
    item_rect = bloom_make_rect(g_combo_popup.item_origin.x,
                                g_combo_popup.item_origin.y + item_index * g_combo_popup.item_height - g_combo_popup.scroll,
                                g_combo_popup.viewport_rect.w,
                                g_combo_popup.item_height);
    hovered = bloom_rect_contains(item_rect, ctx->input.mouse_pos) &&
              bloom_rect_contains(g_combo_popup.viewport_rect, ctx->input.mouse_pos);
    clicked = BLOOM_FALSE;

    if (hovered && ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT] && g_combo_popup.open_frame != ctx->frame_count)
    {
        ctx->active_id = item_id;
    }

    if (ctx->active_id == item_id && !ctx->input.mouse_down[BLOOM_MOUSE_LEFT])
    {
        clicked = hovered && ctx->input.mouse_released[BLOOM_MOUSE_LEFT];
        ctx->active_id = 0;
        if (clicked)
        {
            g_combo_popup.selection_changed = BLOOM_TRUE;
            if (!keep_open)
            {
                g_combo_popup.close_requested = BLOOM_TRUE;
            }
        }
    }

    effective_selected = selected;
    if (clicked)
    {
        effective_selected = g_combo_popup.multi_select ? (selected ? BLOOM_FALSE : BLOOM_TRUE) : BLOOM_TRUE;
    }

    bloom_combo_preview_note_item(label, effective_selected);

    bg = s->window_bg;
    if (hovered)
    {
        bg = s->dropdown_item_hovered;
    }
    if (effective_selected)
    {
        bg = bloom_apply_state_layer(bg, s->input_cursor, 0.22f);
    }


    bloom_draw_rect_filled(&ctx->draw_list, item_rect, bg);
    bloom_draw_label(ctx,
                     bloom_v2(item_rect.x + s->field_padding_x,
                              bloom_centered_text_y(ctx, item_rect.y, g_combo_popup.item_height, s->font_size)),
                     label,
                     s->text_default,
                     s->font_size);

    indicator_center = bloom_v2(item_rect.x + item_rect.w - 16.0f,
                                item_rect.y + g_combo_popup.item_height * 0.5f);
    if (effective_selected)
    {
        if (g_combo_popup.multi_select)
        {
            bloom_draw_rect_rounded(&ctx->draw_list,
                                    bloom_make_rect(indicator_center.x - 7.0f, indicator_center.y - 7.0f, 14.0f, 14.0f),
                                    s->checkbox_mark,
                                    4.0f);
            bloom_draw_line(&ctx->draw_list,
                            bloom_v2(indicator_center.x - 3.0f, indicator_center.y + 0.5f),
                            bloom_v2(indicator_center.x - 0.5f, indicator_center.y + 3.5f),
                            s->window_bg,
                            1.8f);
            bloom_draw_line(&ctx->draw_list,
                            bloom_v2(indicator_center.x - 0.5f, indicator_center.y + 3.5f),
                            bloom_v2(indicator_center.x + 4.5f, indicator_center.y - 3.0f),
                            s->window_bg,
                            1.8f);
        }
        else
        {
            bloom_draw_circle_filled(&ctx->draw_list, indicator_center, 4.0f, s->input_cursor, s->circle_segments);
        }
    }
    else if (g_combo_popup.multi_select)
    {
        bloom_draw_rect_rounded_border(&ctx->draw_list,
                                       bloom_make_rect(indicator_center.x - 7.0f, indicator_center.y - 7.0f, 14.0f, 14.0f),
                                       s->dropdown_border,
                                       4.0f,
                                       1.0f);
    }

    return clicked;
}

bloom_bool bloom_combo_item(const char *label, bloom_bool selected)
{
    return bloom_combo_item_internal(label, selected, BLOOM_FALSE);
}

bloom_bool bloom_action_split_begin(const char *label, bloom_bool *primary_pressed, bloom_i32 visible_items)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_combo_args args;
    bloom_vec2 pos;
    bloom_f32 control_h;
    bloom_f32 control_w;
    bloom_f32 arrow_w;
    bloom_rect control_rect;
    bloom_rect primary_rect;
    bloom_rect arrow_rect;
    bloom_bool popup_hovered;
    bloom_bool popup_open;
    bloom_bool primary_hovered = BLOOM_FALSE;
    bloom_bool primary_held = BLOOM_FALSE;
    bloom_bool arrow_hovered = BLOOM_FALSE;
    bloom_bool arrow_held = BLOOM_FALSE;
    bloom_bool arrow_clicked;
    bloom_id primary_id;
    bloom_id arrow_id;

    if (!ctx || !ctx->current_window)
    {
        if (primary_pressed) *primary_pressed = BLOOM_FALSE;
        return BLOOM_FALSE;
    }

    args = bloom_resolve_combo_args(NULL, visible_items);
    s = &ctx->style;
    pos = ctx->current_window->layout.cursor;
    control_h = bloom_scaled_line_height(ctx, s->font_size) + s->control_padding_y * 2.0f;
    control_w = ctx->current_window->layout.available_width;
    arrow_w = control_h;
    control_rect = bloom_make_rect(pos.x, pos.y, control_w, control_h);
    primary_rect = bloom_make_rect(control_rect.x, control_rect.y, control_rect.w - arrow_w - 1.0f, control_rect.h);
    arrow_rect = bloom_make_rect(primary_rect.x + primary_rect.w + 1.0f, control_rect.y, arrow_w, control_rect.h);

    bloom_push_id(label);
    primary_id = bloom_get_id("##splitPrimary");
    arrow_id = bloom_get_id("##splitArrow");
    bloom_pop_id();

    if (primary_pressed)
    {
        *primary_pressed = bloom_popup_control_hit(ctx, primary_id, primary_rect, &primary_hovered, &primary_held);
    }
    else
    {
        (void)bloom_popup_control_hit(ctx, primary_id, primary_rect, &primary_hovered, &primary_held);
    }

    arrow_clicked = bloom_popup_control_hit(ctx, arrow_id, arrow_rect, &arrow_hovered, &arrow_held);
    popup_hovered = g_combo_popup.open && g_combo_popup.id == arrow_id && bloom_rect_contains(g_combo_popup.popup_rect, ctx->input.mouse_pos);

    if (arrow_clicked)
    {
        if (g_combo_popup.open && g_combo_popup.id == arrow_id)
        {
            bloom_combo_close_popup(ctx);
        }
        else
        {
            bloom_i32 saved_count = g_combo_popup.last_item_count;
            memset(&g_combo_popup, 0, sizeof(g_combo_popup));
            g_combo_popup.last_item_count = saved_count;
            g_combo_popup.open = BLOOM_TRUE;
            g_combo_popup.multi_select = BLOOM_FALSE;
            g_combo_popup.id = arrow_id;
            g_combo_popup.open_frame = ctx->frame_count;
            if (g_combo_popup.last_item_count <= 0)
            {
                g_combo_popup.last_item_count = args.visible_items;
            }
        }
    }
    else if (g_combo_popup.open && g_combo_popup.id == arrow_id && ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT] &&
             !bloom_rect_contains(control_rect, ctx->input.mouse_pos) && !popup_hovered && g_combo_popup.open_frame != ctx->frame_count)
    {
        bloom_combo_close_popup(ctx);
    }

    popup_open = g_combo_popup.open && g_combo_popup.id == arrow_id;
    bloom_action_split_draw_control(ctx, label, control_rect, popup_open,
                                    primary_hovered, primary_held, arrow_hovered, arrow_held);
    bloom_advance_layout(control_w, control_h + s->touch_padding);

    if (!popup_open)
    {
        return BLOOM_FALSE;
    }

    if (bloom_combo_activate_popup(ctx, label, "", control_rect, &args, BLOOM_FALSE))
    {
        g_combo_popup.redraw_control_on_change = BLOOM_FALSE;
        return BLOOM_TRUE;
    }
    return BLOOM_FALSE;
}

void bloom_action_split_end(void)
{
    bloom_combo_end_internal();
}

bloom_bool bloom_action_split_item(const char *label)
{
    return bloom_combo_item_internal(label, BLOOM_FALSE, BLOOM_FALSE);
}

bloom_bool bloom_filter_select_begin(const char *label, const char *preview,
                                     char *filter_buf, bloom_u32 filter_buf_size,
                                     bloom_i32 visible_items)
{
    bloom_combo_args args = bloom_resolve_combo_args(NULL, visible_items);
    return bloom_filter_select_begin_args(label, preview, filter_buf, filter_buf_size, &args);
}

bloom_bool bloom_filter_select_begin_args(const char *label, const char *preview,
                                          char *filter_buf, bloom_u32 filter_buf_size,
                                          const bloom_combo_args *args)
{
    bloom_combo_args resolved = bloom_resolve_combo_args(args, 0);
    bloom_bool open = bloom_combo_begin_internal(label, preview, &resolved, BLOOM_FALSE);
    if (!open)
    {
        return BLOOM_FALSE;
    }

    g_combo_popup.search_enabled = BLOOM_TRUE;
    g_combo_popup.filter_buf = filter_buf;
    g_combo_popup.filter_buf_size = filter_buf_size;
    bloom_combo_prepare_search_field();
    return BLOOM_TRUE;
}

void bloom_filter_select_end(void)
{
    bloom_combo_end_internal();
}

bloom_bool bloom_filter_select_item(const char *label, bloom_bool selected)
{
    if (g_combo_popup.search_enabled && g_combo_popup.filter_buf)
    {
        if (!bloom_visible_label_contains(label, g_combo_popup.filter_buf))
        {
            return BLOOM_FALSE;
        }
    }
    return bloom_combo_item_internal(label, selected, BLOOM_FALSE);
}

bloom_bool bloom_multi_select_begin(const char *label, const char *preview, bloom_i32 visible_items)
{
    bloom_combo_args args = bloom_resolve_combo_args(NULL, visible_items);
    return bloom_combo_begin_internal(label, preview, &args, BLOOM_TRUE);
}

bloom_bool bloom_multi_select_begin_args(const char *label, const char *preview, const bloom_combo_args *args)
{
    bloom_combo_args resolved = bloom_resolve_combo_args(args, 0);
    return bloom_combo_begin_internal(label, preview, &resolved, BLOOM_TRUE);
}

void bloom_multi_select_end(void)
{
    bloom_combo_end_internal();
}

bloom_bool bloom_multi_select_item(const char *label, bloom_bool selected)
{
    return bloom_combo_item_internal(label, selected, BLOOM_TRUE);

}

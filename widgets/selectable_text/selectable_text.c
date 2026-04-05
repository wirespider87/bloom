#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "widgets/internal.h"

#define BLOOM_LABEL_SEL_CAPACITY 32

typedef struct bloom_label_sel_state
{
    bloom_id id;
    bloom_bool active;
    bloom_bool dragging;
    bloom_i32 caret;
    bloom_i32 anchor;
} bloom_label_sel_state;

static bloom_label_sel_state g_label_sel[BLOOM_LABEL_SEL_CAPACITY];

static bloom_label_sel_state *bloom_label_sel_get(bloom_id id)
{
    bloom_i32 i;
    bloom_label_sel_state *free_slot = NULL;

    for (i = 0; i < BLOOM_LABEL_SEL_CAPACITY; ++i)
    {
        if (g_label_sel[i].active && g_label_sel[i].id == id)
        {
            return &g_label_sel[i];
        }
        if (!free_slot && !g_label_sel[i].active)
        {
            free_slot = &g_label_sel[i];
        }
    }

    if (!free_slot)
    {
        free_slot = &g_label_sel[0];
    }

    free_slot->active = BLOOM_TRUE;
    free_slot->id = id;
    free_slot->dragging = BLOOM_FALSE;
    free_slot->caret = 0;
    free_slot->anchor = 0;
    return free_slot;
}

static bloom_i32 bloom_label_index_from_x(const char *text, bloom_i32 len,
                                           bloom_f32 local_x, bloom_f32 font_size)
{
    bloom_i32 i;

    if (local_x <= 0.0f)
    {
        return 0;
    }

    for (i = 0; i < len; ++i)
    {
        bloom_f32 advance = bloom_text_width_n(text, (bloom_u32)(i + 1), font_size);
        if (local_x < advance)
        {
            bloom_f32 prev = (i > 0) ? bloom_text_width_n(text, (bloom_u32)i, font_size) : 0.0f;
            return (local_x - prev) < (advance - local_x) ? i : (i + 1);
        }
    }

    return len;
}

static bloom_i32 bloom_label_clamp(bloom_i32 index, bloom_i32 len)
{
    if (index < 0) return 0;
    if (index > len) return len;
    return index;
}

void bloom_text_selectable(const char *text)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_id id;
    bloom_label_sel_state *sel;
    bloom_vec2 pos;
    bloom_f32 w;
    bloom_f32 h;
    bloom_rect rect;
    bloom_bool hovered;
    bloom_i32 len;
    bloom_i32 sel_start;
    bloom_i32 sel_end;

    if (!ctx || !ctx->current_window || !text)
    {
        return;
    }

    s = &ctx->style;
    id = bloom_get_id(text);
    sel = bloom_label_sel_get(id);
    pos = ctx->current_window->layout.cursor;
    w = bloom_text_width(text, s->font_size);
    h = bloom_scaled_line_height(ctx, s->font_size);
    rect = bloom_make_rect(pos.x, pos.y, w, h);
    hovered = bloom_widget_hovered(rect);
    len = (bloom_i32)strlen(text);

    sel->caret = bloom_label_clamp(sel->caret, len);
    sel->anchor = bloom_label_clamp(sel->anchor, len);

    if (hovered && ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT])
    {
        bloom_f32 local_x = ctx->input.mouse_pos.x - pos.x;
        bloom_i32 idx = bloom_label_index_from_x(text, len, local_x, s->font_size);
        bloom_bool extend = ctx->input.shift_held && (ctx->active_id == id);

        ctx->active_id = id;
        sel->dragging = BLOOM_TRUE;

        if (extend)
        {
            sel->caret = idx;
        }
        else
        {
            sel->caret = idx;
            sel->anchor = idx;
        }
    }

    if (ctx->active_id == id)
    {
        if (ctx->input.mouse_down[BLOOM_MOUSE_LEFT] && sel->dragging)
        {
            bloom_f32 local_x = ctx->input.mouse_pos.x - pos.x;
            sel->caret = bloom_label_index_from_x(text, len, local_x, s->font_size);
        }
        else if (!ctx->input.mouse_down[BLOOM_MOUSE_LEFT])
        {
            sel->dragging = BLOOM_FALSE;
        }
    }

    /* Draw selection highlight */
    sel_start = sel->caret < sel->anchor ? sel->caret : sel->anchor;
    sel_end = sel->caret > sel->anchor ? sel->caret : sel->anchor;

    if (sel_start != sel_end && ctx->active_id == id)
    {
        bloom_f32 x0 = pos.x + bloom_text_width_n(text, (bloom_u32)sel_start, s->font_size);
        bloom_f32 x1 = pos.x + bloom_text_width_n(text, (bloom_u32)sel_end, s->font_size);
        bloom_color highlight = s->input_cursor;
        highlight.a = (bloom_u8)(highlight.a * 0.3f);
        bloom_draw_rect_filled(&ctx->draw_list,
                               bloom_make_rect(x0, pos.y, x1 - x0, h),
                               highlight);
    }

    /* Draw text */
    bloom_draw_text(&ctx->draw_list, pos, text, s->text_default,
                    s->font_size, ctx->default_font.texture_id);

    /* Ctrl+C copy */
    if (ctx->active_id == id && sel_start != sel_end &&
        ctx->input.ctrl_held && ctx->input.keys_pressed[BLOOM_KEY_C])
    {
        char clipboard[2048];
        bloom_i32 copy_len = sel_end - sel_start;
        if (copy_len > (bloom_i32)(sizeof(clipboard) - 1))
        {
            copy_len = (bloom_i32)(sizeof(clipboard) - 1);
        }
        memcpy(clipboard, text + sel_start, copy_len);
        clipboard[copy_len] = '\0';
        bloom_platform_set_clipboard_text(clipboard);
    }

    bloom_advance_layout(w, h);
}

#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "widgets/internal.h"
#include "core/base/utf8.h"

#define BLOOM_TEXT_EDIT_STATE_CAPACITY 64
#define BLOOM_TEXT_UNDO_DEPTH 16
#define BLOOM_TEXT_UNDO_BUF_MAX 512

typedef struct bloom_text_undo_snap
{
    char text[BLOOM_TEXT_UNDO_BUF_MAX];
    bloom_i32 caret;
    bloom_i32 anchor;
} bloom_text_undo_snap;

typedef struct bloom_text_undo_ring
{
    bloom_text_undo_snap entries[BLOOM_TEXT_UNDO_DEPTH];
    bloom_i32 count;
    bloom_i32 cursor;
} bloom_text_undo_ring;

typedef struct bloom_text_edit_state
{
    bloom_id id;
    bloom_bool active;
    bloom_bool dragging;
    bloom_i32 caret;
    bloom_i32 anchor;
    bloom_i32 preferred_column;
} bloom_text_edit_state;

static bloom_text_edit_state g_text_edit_states[BLOOM_TEXT_EDIT_STATE_CAPACITY];
static bloom_text_undo_ring g_text_undo[BLOOM_TEXT_EDIT_STATE_CAPACITY];
static bloom_id g_repeat_owner_id = 0;
static bloom_i32 g_repeat_key = BLOOM_KEY_NONE;
static bloom_f64 g_repeat_time = 0.0;

static bloom_bool bloom_key_repeat(bloom_context *ctx, bloom_id owner_id, bloom_i32 key,
                                   bloom_f64 initial_delay, bloom_f64 repeat_interval)
{
    if (ctx->input.keys_pressed[key])
    {
        g_repeat_owner_id = owner_id;
        g_repeat_key = key;
        g_repeat_time = ctx->time + initial_delay;
        return BLOOM_TRUE;
    }

    if (!ctx->input.keys_down[key])
    {
        if (g_repeat_owner_id == owner_id && g_repeat_key == key)
        {
            g_repeat_owner_id = 0;
            g_repeat_key = BLOOM_KEY_NONE;
            g_repeat_time = 0.0;
        }
        return BLOOM_FALSE;
    }

    if (g_repeat_owner_id != owner_id || g_repeat_key != key)
    {
        g_repeat_owner_id = owner_id;
        g_repeat_key = key;
        g_repeat_time = ctx->time + initial_delay;
        return BLOOM_FALSE;
    }

    if (ctx->time >= g_repeat_time)
    {
        g_repeat_time = ctx->time + repeat_interval;
        return BLOOM_TRUE;
    }

    return BLOOM_FALSE;
}

static bloom_i32 bloom_text_clamp_index(const char *text, bloom_i32 index);

static bloom_text_undo_ring *bloom_text_undo_for_state(const bloom_text_edit_state *state)
{
    bloom_i32 idx = (bloom_i32)(state - g_text_edit_states);
    return &g_text_undo[idx];
}

static void bloom_text_undo_push(bloom_text_undo_ring *ring, const char *buf,
                                 bloom_i32 caret, bloom_i32 anchor)
{
    bloom_i32 len;
    bloom_text_undo_snap *snap;

    ring->count = ring->cursor + 1;

    if (ring->count >= BLOOM_TEXT_UNDO_DEPTH)
    {
        memmove(&ring->entries[0], &ring->entries[1],
                (BLOOM_TEXT_UNDO_DEPTH - 1) * sizeof(bloom_text_undo_snap));
        ring->count = BLOOM_TEXT_UNDO_DEPTH - 1;
    }

    snap = &ring->entries[ring->count];
    len = (bloom_i32)strlen(buf);
    if (len >= BLOOM_TEXT_UNDO_BUF_MAX)
    {
        len = BLOOM_TEXT_UNDO_BUF_MAX - 1;
    }
    memcpy(snap->text, buf, len);
    snap->text[len] = '\0';
    snap->caret = caret;
    snap->anchor = anchor;
    ring->count++;
    ring->cursor = ring->count - 1;
}

static bloom_bool bloom_text_undo_restore(bloom_text_undo_ring *ring, bloom_i32 new_cursor,
                                          char *buf, bloom_u32 buf_size,
                                          bloom_text_edit_state *state)
{
    bloom_text_undo_snap *snap;
    bloom_i32 len;

    if (new_cursor < 0 || new_cursor >= ring->count)
    {
        return BLOOM_FALSE;
    }

    ring->cursor = new_cursor;
    snap = &ring->entries[ring->cursor];
    len = (bloom_i32)strlen(snap->text);
    if (len >= (bloom_i32)buf_size)
    {
        len = (bloom_i32)buf_size - 1;
    }
    memcpy(buf, snap->text, len);
    buf[len] = '\0';
    state->caret = bloom_text_clamp_index(buf, snap->caret);
    state->anchor = bloom_text_clamp_index(buf, snap->anchor);
    return BLOOM_TRUE;
}

static bloom_text_edit_state *bloom_text_edit_state_get(bloom_id id)
{
    bloom_i32 i;
    bloom_text_edit_state *free_state = NULL;

    for (i = 0; i < BLOOM_TEXT_EDIT_STATE_CAPACITY; ++i)
    {
        if (g_text_edit_states[i].active && g_text_edit_states[i].id == id)
        {
            return &g_text_edit_states[i];
        }
        if (!free_state && !g_text_edit_states[i].active)
        {
            free_state = &g_text_edit_states[i];
        }
    }

    if (!free_state)
    {
        free_state = &g_text_edit_states[0];
    }

    free_state->active = BLOOM_TRUE;
    free_state->dragging = BLOOM_FALSE;
    free_state->id = id;
    free_state->caret = 0;
    free_state->anchor = 0;
    free_state->preferred_column = -1;
    {
        bloom_text_undo_ring *ring = bloom_text_undo_for_state(free_state);
        ring->count = 0;
        ring->cursor = -1;
    }
    return free_state;
}

static bloom_i32 bloom_text_length(const char *text)
{
    return text ? (bloom_i32)strlen(text) : 0;
}

static bloom_i32 bloom_text_clamp_index(const char *text, bloom_i32 index)
{
    bloom_i32 len = bloom_text_length(text);
    if (index < 0)
    {
        index = 0;
    }
    if (index > len)
    {
        index = len;
    }
    return bloom_utf8_snap_to_boundary(text, len, index);
}

static bloom_i32 bloom_text_min_i32(bloom_i32 a, bloom_i32 b)
{
    return a < b ? a : b;
}

static bloom_i32 bloom_text_max_i32(bloom_i32 a, bloom_i32 b)
{
    return a > b ? a : b;
}

static bloom_bool bloom_text_has_selection(const bloom_text_edit_state *state)
{
    return state && state->caret != state->anchor;
}

static bloom_i32 bloom_text_selection_start(const bloom_text_edit_state *state)
{
    return bloom_text_min_i32(state->caret, state->anchor);
}

static bloom_i32 bloom_text_selection_end(const bloom_text_edit_state *state)
{
    return bloom_text_max_i32(state->caret, state->anchor);
}

static void bloom_text_clear_selection(bloom_text_edit_state *state)
{
    if (!state)
    {
        return;
    }
    state->anchor = state->caret;
}

static void bloom_text_set_caret(bloom_text_edit_state *state, const char *text,
                                 bloom_i32 index, bloom_bool keep_selection)
{
    if (!state)
    {
        return;
    }

    state->caret = bloom_text_clamp_index(text, index);
    if (!keep_selection)
    {
        state->anchor = state->caret;
    }
}

static bloom_i32 bloom_text_line_start(const char *text, bloom_i32 index)
{
    bloom_i32 pos = bloom_text_clamp_index(text, index);
    while (pos > 0 && text[pos - 1] != '\n')
    {
        pos--;
    }
    return pos;
}

static bloom_i32 bloom_text_line_end(const char *text, bloom_i32 index)
{
    bloom_i32 len = bloom_text_length(text);
    bloom_i32 pos = bloom_text_clamp_index(text, index);
    while (pos < len && text[pos] != '\n')
    {
        pos++;
    }
    return pos;
}

static bloom_i32 bloom_text_line_count(const char *text)
{
    bloom_i32 lines = 1;

    if (!text || text[0] == '\0')
    {
        return 1;
    }

    while (*text)
    {
        if (*text == '\n')
        {
            lines++;
        }
        text++;
    }

    return lines;
}

static bloom_i32 bloom_text_current_column(const char *text, bloom_i32 index)
{
    bloom_i32 start = bloom_text_line_start(text, index);
    return bloom_text_clamp_index(text, index) - start;
}

static bloom_i32 bloom_text_position_for_column(const char *text, bloom_i32 line_start, bloom_i32 column)
{
    bloom_i32 line_end = bloom_text_line_end(text, line_start);
    bloom_i32 target = line_start + column;
    if (target > line_end)
    {
        target = line_end;
    }
    return target;
}

static void bloom_text_move_horizontal(bloom_text_edit_state *state, const char *text,
                                       bloom_i32 delta, bloom_bool keep_selection)
{
    bloom_i32 target;

    if (!state)
    {
        return;
    }

    if (!keep_selection && bloom_text_has_selection(state))
    {
        target = (delta < 0) ? bloom_text_selection_start(state) : bloom_text_selection_end(state);
        bloom_text_set_caret(state, text, target, BLOOM_FALSE);
    }
    else
    {
        bloom_i32 len = bloom_text_length(text);

        if (delta == -1)
        {
            target = bloom_utf8_prior_char(text, state->caret);
        }
        else if (delta == 1)
        {
            target = bloom_utf8_next_char(text, len, state->caret);
        }
        else
        {
            target = state->caret + delta;
        }
        bloom_text_set_caret(state, text, target, keep_selection);
    }

    state->preferred_column = bloom_text_current_column(text, state->caret);
}

static void bloom_text_move_home_end(bloom_text_edit_state *state, const char *text,
                                     bloom_bool multiline, bloom_bool move_to_end,
                                     bloom_bool keep_selection)
{
    bloom_i32 target;

    if (!state)
    {
        return;
    }

    if (multiline)
    {
        target = move_to_end ? bloom_text_line_end(text, state->caret)
                             : bloom_text_line_start(text, state->caret);
    }
    else
    {
        target = move_to_end ? bloom_text_length(text) : 0;
    }

    bloom_text_set_caret(state, text, target, keep_selection);
    state->preferred_column = bloom_text_current_column(text, state->caret);
}

static void bloom_text_move_vertical(bloom_text_edit_state *state, const char *text,
                                     bloom_bool move_down, bloom_bool keep_selection)
{
    bloom_i32 line_start;
    bloom_i32 line_end;
    bloom_i32 column;
    bloom_i32 target;

    if (!state)
    {
        return;
    }

    line_start = bloom_text_line_start(text, state->caret);
    line_end = bloom_text_line_end(text, state->caret);
    column = state->preferred_column >= 0 ? state->preferred_column
                                          : bloom_text_current_column(text, state->caret);

    if (move_down)
    {
        if (line_end >= bloom_text_length(text))
        {
            target = line_end;
        }
        else
        {
            target = bloom_text_position_for_column(text, line_end + 1, column);
        }
    }
    else
    {
        if (line_start <= 0)
        {
            target = 0;
        }
        else
        {
            bloom_i32 prev_line_end = line_start - 1;
            bloom_i32 prev_line_start = bloom_text_line_start(text, prev_line_end);
            target = bloom_text_position_for_column(text, prev_line_start, column);
        }
    }

    bloom_text_set_caret(state, text, target, keep_selection);
    state->preferred_column = column;
}

static void bloom_text_delete_range(char *buf, bloom_i32 start, bloom_i32 end)
{
    bloom_i32 len = bloom_text_length(buf);

    if (!buf || start < 0 || end < start || start > len)
    {
        return;
    }
    if (end > len)
    {
        end = len;
    }
    if (start == end)
    {
        return;
    }

    memmove(buf + start, buf + end, (size_t)(len - end + 1));
}

static bloom_bool bloom_text_insert_filtered(char *buf, bloom_u32 buf_size, bloom_i32 *caret_io,
                                             const char *text, bloom_bool multiline)
{
    bloom_i32 len;
    bloom_i32 caret;
    bloom_bool changed = BLOOM_FALSE;
    const char *p;

    if (!buf || !caret_io || !text || buf_size == 0)
    {
        return BLOOM_FALSE;
    }

    len = bloom_text_length(buf);
    caret = bloom_text_clamp_index(buf, *caret_io);

    {
        const char *end = text + strlen(text);

        for (p = text; p < end;)
        {
            bloom_u32 bl;
            bloom_u32 rem = (bloom_u32)(end - p);
            bloom_u32 cp = bloom_utf8_decode_one(p, rem, &bl);

            if (bl == 0u)
            {
                bl = 1u;
            }

            if (cp == (bloom_u32)'\r')
            {
                p += bl;
                continue;
            }
            if (cp == (bloom_u32)'\n' && !multiline)
            {
                p += bl;
                continue;
            }
            if (cp < 32u && cp != (bloom_u32)'\n')
            {
                p += bl;
                continue;
            }
            if ((bloom_u32)len + bl > buf_size - 1u)
            {
                break;
            }

            memmove(buf + caret + (bloom_i32)bl, buf + caret, (size_t)(len - caret + 1));
            memcpy(buf + caret, p, (size_t)bl);
            caret += (bloom_i32)bl;
            len += (bloom_i32)bl;
            p += bl;
            changed = BLOOM_TRUE;
        }
    }

    *caret_io = caret;
    return changed;
}

static void bloom_text_copy_selection(const char *buf, bloom_i32 start, bloom_i32 end,
                                      char *out_buf, bloom_u32 out_size)
{
    bloom_i32 len;
    bloom_i32 count;

    if (!buf || !out_buf || out_size == 0)
    {
        return;
    }

    len = bloom_text_length(buf);
    if (start < 0) start = 0;
    if (end > len) end = len;
    if (end < start) end = start;

    count = end - start;
    if ((bloom_u32)count >= out_size)
    {
        count = (bloom_i32)out_size - 1;
    }

    if (count > 0)
    {
        memcpy(out_buf, buf + start, (size_t)count);
    }
    out_buf[count] = '\0';
}

static bloom_i32 bloom_text_index_from_x(const char *text, bloom_i32 line_start, bloom_i32 line_end,
                                         bloom_f32 local_x, bloom_f32 font_size)
{
    bloom_i32 index;

    if (local_x <= 0.0f)
    {
        return line_start;
    }

    for (index = line_start; index < line_end; ++index)
    {
        bloom_f32 advance = bloom_text_width_n(text + line_start,
                                               (bloom_u32)(index - line_start + 1),
                                               font_size);
        if (local_x < advance)
        {
            bloom_f32 prev_advance = (index > line_start)
                ? bloom_text_width_n(text + line_start, (bloom_u32)(index - line_start), font_size)
                : 0.0f;
            return (local_x - prev_advance) < (advance - local_x) ? index : (index + 1);
        }
    }

    return line_end;
}

static bloom_i32 bloom_text_index_from_point(bloom_context *ctx, bloom_rect rect, const char *text,
                                             bloom_f32 font_size, bloom_bool multiline,
                                             bloom_vec2 point)
{
    bloom_style *s = &ctx->style;
    bloom_f32 local_x = point.x - (rect.x + s->field_padding_x);
    bloom_f32 line_h = bloom_scaled_line_height(ctx, font_size);

    if (!multiline)
    {
        return bloom_text_index_from_x(text, 0, bloom_text_length(text), local_x, font_size);
    }

    {
        bloom_f32 local_y = point.y - (rect.y + s->field_padding_y);
        bloom_i32 target_line = (bloom_i32)floorf(local_y / line_h);
        bloom_i32 line_index = 0;
        bloom_i32 line_start = 0;
        bloom_i32 line_end;

        if (target_line < 0)
        {
            target_line = 0;
        }

        while (1)
        {
            line_end = bloom_text_line_end(text, line_start);
            if (line_index >= target_line || line_end >= bloom_text_length(text))
            {
                return bloom_text_index_from_x(text, line_start, line_end, local_x, font_size);
            }
            line_start = line_end + 1;
            line_index++;
        }
    }
}

static void bloom_draw_text_selection(bloom_context *ctx, bloom_rect rect, const char *text,
                                      bloom_f32 font_size, bloom_bool multiline,
                                      const bloom_text_edit_state *state)
{
    bloom_style *s = &ctx->style;
    bloom_i32 sel_start;
    bloom_i32 sel_end;
    bloom_f32 line_h = bloom_scaled_line_height(ctx, font_size);

    if (!bloom_text_has_selection(state))
    {
        return;
    }

    sel_start = bloom_text_selection_start(state);
    sel_end = bloom_text_selection_end(state);

    if (!multiline)
    {
        bloom_f32 x0 = rect.x + s->field_padding_x + bloom_text_width_n(text, (bloom_u32)sel_start, font_size);
        bloom_f32 x1 = rect.x + s->field_padding_x + bloom_text_width_n(text, (bloom_u32)sel_end, font_size);
        bloom_draw_rect_rounded(&ctx->draw_list,
            bloom_make_rect(x0 - 1.0f,
                            bloom_centered_text_y(ctx, rect.y, rect.h, font_size) - 1.0f,
                            (x1 - x0) + 2.0f,
                            line_h + 2.0f),
            bloom_apply_state_layer(s->input_bg, s->input_cursor, 0.35f),
            5.0f);
        return;
    }

    {
        bloom_i32 line_start = 0;
        bloom_f32 line_y = rect.y + s->field_padding_y;

        while (1)
        {
            bloom_i32 line_end = bloom_text_line_end(text, line_start);
            bloom_i32 seg_start = bloom_text_max_i32(sel_start, line_start);
            bloom_i32 seg_end = bloom_text_min_i32(sel_end, line_end);

            bloom_bool spans_newline =
                (sel_end > line_end && line_end < bloom_text_length(text) && sel_start <= line_end);

            if (seg_end > seg_start || spans_newline)
            {
                bloom_f32 x0 = rect.x + s->field_padding_x + bloom_text_width_n(text + line_start,
                                                                                 (bloom_u32)(seg_start - line_start),
                                                                                 font_size);
                bloom_f32 x1;

                if (seg_end > seg_start)
                {
                    x1 = rect.x + s->field_padding_x + bloom_text_width_n(text + line_start,
                                                                          (bloom_u32)(seg_end - line_start),
                                                                          font_size);
                }
                else
                {
                    x1 = x0 + 6.0f;
                }

                bloom_draw_rect_rounded(&ctx->draw_list,
                    bloom_make_rect(x0 - 1.0f, line_y - 1.0f, (x1 - x0) + 2.0f, line_h + 2.0f),
                    bloom_apply_state_layer(s->input_bg, s->input_cursor, 0.35f),
                    4.0f);
            }

            if (line_end >= bloom_text_length(text))
            {
                break;
            }

            line_start = line_end + 1;
            line_y += line_h;
        }
    }
}

static void bloom_draw_text_contents(bloom_context *ctx, bloom_rect rect, const char *text,
                                     bloom_color color, bloom_f32 font_size,
                                     bloom_bool multiline)
{
    bloom_style *s = &ctx->style;

    if (!multiline)
    {
        bloom_draw_text(&ctx->draw_list,
                        bloom_v2(rect.x + s->field_padding_x,
                                 bloom_centered_text_y(ctx, rect.y, rect.h, font_size)),
                        text,
                        color,
                        font_size,
                        ctx->default_font.texture_id);
        return;
    }

    {
        bloom_i32 line_start = 0;
        bloom_f32 line_h = bloom_scaled_line_height(ctx, font_size);
        bloom_f32 line_y = rect.y + s->field_padding_y;

        while (1)
        {
            bloom_i32 line_end = bloom_text_line_end(text, line_start);
            bloom_i32 count = line_end - line_start;

            if (count > 0)
            {
                bloom_draw_text_n(&ctx->draw_list,
                                  bloom_v2(rect.x + s->field_padding_x, line_y),
                                  text + line_start,
                                  (bloom_u32)count,
                                  color,
                                  font_size,
                                  ctx->default_font.texture_id);
            }

            if (line_end >= bloom_text_length(text))
            {
                break;
            }

            line_start = line_end + 1;
            line_y += line_h;
        }
    }
}

static void bloom_draw_text_cursor(bloom_context *ctx, bloom_rect rect, const char *text,
                                   bloom_f32 font_size, bloom_bool multiline,
                                   const bloom_text_edit_state *state)
{
    bloom_style *s = &ctx->style;
    bloom_f32 line_h = bloom_scaled_line_height(ctx, font_size);
    bloom_f32 cursor_x;
    bloom_f32 cursor_y;

    if (!multiline)
    {
        cursor_x = rect.x + s->field_padding_x + bloom_text_width_n(text, (bloom_u32)state->caret, font_size);
        cursor_y = bloom_centered_text_y(ctx, rect.y, rect.h, font_size);
    }
    else
    {
        bloom_i32 line_start = bloom_text_line_start(text, state->caret);
        bloom_i32 line_index = 0;
        bloom_i32 scan = 0;

        while (scan < line_start)
        {
            if (text[scan] == '\n')
            {
                line_index++;
            }
            scan++;
        }

        cursor_x = rect.x + s->field_padding_x + bloom_text_width_n(text + line_start,
                                                                     (bloom_u32)(state->caret - line_start),
                                                                     font_size);
        cursor_y = rect.y + s->field_padding_y + (bloom_f32)line_index * line_h;
    }

    bloom_draw_rect_filled(&ctx->draw_list,
                           bloom_make_rect(cursor_x, cursor_y, 2.0f, line_h),
                           s->input_cursor);
}

static bloom_bool bloom_text_edit_process_input(bloom_context *ctx, bloom_text_edit_state *state,
                                                bloom_id id, char *buf, bloom_u32 buf_size,
                                                bloom_bool multiline)
{
    bloom_bool changed = BLOOM_FALSE;
    bloom_bool undo_action = BLOOM_FALSE;
    bloom_text_undo_ring *ring = bloom_text_undo_for_state(state);
    bloom_i32 len = bloom_text_length(buf);
    bloom_bool shift = ctx->input.shift_held;

    state->caret = bloom_text_clamp_index(buf, state->caret);
    state->anchor = bloom_text_clamp_index(buf, state->anchor);

    if (ring->count == 0)
    {
        bloom_text_undo_push(ring, buf, state->caret, state->anchor);
    }

    if (ctx->input.ctrl_held && ctx->input.keys_pressed[BLOOM_KEY_Z])
    {
        if (shift)
        {
            if (bloom_text_undo_restore(ring, ring->cursor + 1, buf, buf_size, state))
            {
                changed = BLOOM_TRUE;
                undo_action = BLOOM_TRUE;
                len = bloom_text_length(buf);
            }
        }
        else
        {
            if (bloom_text_undo_restore(ring, ring->cursor - 1, buf, buf_size, state))
            {
                changed = BLOOM_TRUE;
                undo_action = BLOOM_TRUE;
                len = bloom_text_length(buf);
            }
        }
        state->preferred_column = bloom_text_current_column(buf, state->caret);
    }

    if (ctx->input.ctrl_held && ctx->input.keys_pressed[BLOOM_KEY_Y])
    {
        if (bloom_text_undo_restore(ring, ring->cursor + 1, buf, buf_size, state))
        {
            changed = BLOOM_TRUE;
            undo_action = BLOOM_TRUE;
            len = bloom_text_length(buf);
        }
        state->preferred_column = bloom_text_current_column(buf, state->caret);
    }

    if (ctx->input.ctrl_held && ctx->input.keys_pressed[BLOOM_KEY_A])
    {
        state->anchor = 0;
        state->caret = len;
        state->preferred_column = bloom_text_current_column(buf, state->caret);
    }

    if (ctx->input.ctrl_held && ctx->input.keys_pressed[BLOOM_KEY_C] && len > 0)
    {
        char clipboard[2048];
        if (bloom_text_has_selection(state))
        {
            bloom_text_copy_selection(buf, bloom_text_selection_start(state), bloom_text_selection_end(state),
                                      clipboard, (bloom_u32)sizeof(clipboard));
        }
        else
        {
            bloom_text_copy_selection(buf, 0, len, clipboard, (bloom_u32)sizeof(clipboard));
        }
        bloom_platform_set_clipboard_text(clipboard);
    }

    if (ctx->input.ctrl_held && ctx->input.keys_pressed[BLOOM_KEY_X] && len > 0)
    {
        char clipboard[2048];
        bloom_i32 start = 0;
        bloom_i32 end = len;

        if (bloom_text_has_selection(state))
        {
            start = bloom_text_selection_start(state);
            end = bloom_text_selection_end(state);
        }

        bloom_text_copy_selection(buf, start, end, clipboard, (bloom_u32)sizeof(clipboard));
        bloom_platform_set_clipboard_text(clipboard);
        bloom_text_delete_range(buf, start, end);
        bloom_text_set_caret(state, buf, start, BLOOM_FALSE);
        state->preferred_column = bloom_text_current_column(buf, state->caret);
        changed = BLOOM_TRUE;
        len = bloom_text_length(buf);
    }

    if (ctx->input.ctrl_held && ctx->input.keys_pressed[BLOOM_KEY_V])
    {
        char clipboard[2048];
        if (bloom_platform_get_clipboard_text(clipboard, (bloom_u32)sizeof(clipboard)))
        {
            if (bloom_text_has_selection(state))
            {
                bloom_i32 start = bloom_text_selection_start(state);
                bloom_i32 end = bloom_text_selection_end(state);
                bloom_text_delete_range(buf, start, end);
                bloom_text_set_caret(state, buf, start, BLOOM_FALSE);
            }

            if (bloom_text_insert_filtered(buf, buf_size, &state->caret, clipboard, multiline))
            {
                bloom_text_clear_selection(state);
                state->preferred_column = bloom_text_current_column(buf, state->caret);
                changed = BLOOM_TRUE;
            }
        }
    }

    if (bloom_key_repeat(ctx, id, BLOOM_KEY_BACKSPACE, 0.40, 0.035))
    {
        if (bloom_text_has_selection(state))
        {
            bloom_i32 start = bloom_text_selection_start(state);
            bloom_i32 end = bloom_text_selection_end(state);
            bloom_text_delete_range(buf, start, end);
            bloom_text_set_caret(state, buf, start, BLOOM_FALSE);
            changed = BLOOM_TRUE;
        }
        else if (state->caret > 0)
        {
            bloom_i32 pos = bloom_utf8_prior_char(buf, state->caret);
            bloom_text_delete_range(buf, pos, state->caret);
            bloom_text_set_caret(state, buf, pos, BLOOM_FALSE);
            changed = BLOOM_TRUE;
        }
        state->preferred_column = bloom_text_current_column(buf, state->caret);
    }

    if (bloom_key_repeat(ctx, id, BLOOM_KEY_DELETE, 0.40, 0.035))
    {
        if (bloom_text_has_selection(state))
        {
            bloom_i32 start = bloom_text_selection_start(state);
            bloom_i32 end = bloom_text_selection_end(state);
            bloom_text_delete_range(buf, start, end);
            bloom_text_set_caret(state, buf, start, BLOOM_FALSE);
            changed = BLOOM_TRUE;
        }
        else if (state->caret < bloom_text_length(buf))
        {
            bloom_i32 next = bloom_utf8_next_char(buf, bloom_text_length(buf), state->caret);
            bloom_text_delete_range(buf, state->caret, next);
            changed = BLOOM_TRUE;
        }
        state->preferred_column = bloom_text_current_column(buf, state->caret);
    }

    if (ctx->input.keys_pressed[BLOOM_KEY_LEFT])
    {
        bloom_text_move_horizontal(state, buf, -1, shift);
    }
    if (ctx->input.keys_pressed[BLOOM_KEY_RIGHT])
    {
        bloom_text_move_horizontal(state, buf, 1, shift);
    }
    if (ctx->input.keys_pressed[BLOOM_KEY_HOME])
    {
        bloom_text_move_home_end(state, buf, multiline, BLOOM_FALSE, shift);
    }
    if (ctx->input.keys_pressed[BLOOM_KEY_END])
    {
        bloom_text_move_home_end(state, buf, multiline, BLOOM_TRUE, shift);
    }
    if (multiline && ctx->input.keys_pressed[BLOOM_KEY_UP])
    {
        bloom_text_move_vertical(state, buf, BLOOM_FALSE, shift);
    }
    if (multiline && ctx->input.keys_pressed[BLOOM_KEY_DOWN])
    {
        bloom_text_move_vertical(state, buf, BLOOM_TRUE, shift);
    }

    if (multiline && ctx->input.keys_pressed[BLOOM_KEY_ENTER])
    {
        if (bloom_text_has_selection(state))
        {
            bloom_i32 start = bloom_text_selection_start(state);
            bloom_i32 end = bloom_text_selection_end(state);
            bloom_text_delete_range(buf, start, end);
            bloom_text_set_caret(state, buf, start, BLOOM_FALSE);
        }
        if (bloom_text_insert_filtered(buf, buf_size, &state->caret, "\n", BLOOM_TRUE))
        {
            bloom_text_clear_selection(state);
            state->preferred_column = bloom_text_current_column(buf, state->caret);
            changed = BLOOM_TRUE;
        }
    }

    if (ctx->input.text_input_len > 0)
    {
        char text_chunk[65];
        bloom_i32 i;

        if (ctx->input.text_input_len > 64)
        {
            ctx->input.text_input_len = 64;
        }
        for (i = 0; i < ctx->input.text_input_len; ++i)
        {
            text_chunk[i] = ctx->input.text_input[i];
        }
        text_chunk[ctx->input.text_input_len] = '\0';

        if (bloom_text_has_selection(state))
        {
            bloom_i32 start = bloom_text_selection_start(state);
            bloom_i32 end = bloom_text_selection_end(state);
            bloom_text_delete_range(buf, start, end);
            bloom_text_set_caret(state, buf, start, BLOOM_FALSE);
        }

        if (bloom_text_insert_filtered(buf, buf_size, &state->caret, text_chunk, multiline))
        {
            bloom_text_clear_selection(state);
            state->preferred_column = bloom_text_current_column(buf, state->caret);
            changed = BLOOM_TRUE;
        }
    }

    if (ctx->input.keys_pressed[BLOOM_KEY_ESCAPE] || (!multiline && ctx->input.keys_pressed[BLOOM_KEY_ENTER]))
    {
        ctx->focus_id = 0;
        state->dragging = BLOOM_FALSE;
    }

    if (changed && !undo_action)
    {
        bloom_text_undo_push(ring, buf, state->caret, state->anchor);
    }

    return changed;
}

static bloom_bool bloom_text_input_internal(const char *label, char *buf, bloom_u32 buf_size,
                                            bloom_bool multiline, bloom_i32 lines)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_id id;
    bloom_text_edit_state *state;
    bloom_vec2 pos;
    bloom_u32 visible_len;
    bloom_bool show_label;
    bloom_f32 label_h;
    bloom_f32 label_gap;
    bloom_f32 line_h;
    bloom_f32 input_w;
    bloom_f32 input_h;
    bloom_rect input_rect;
    bloom_bool hovered;
    bloom_bool focused;
    bloom_bool changed = BLOOM_FALSE;

    if (!ctx || !ctx->current_window || !buf || buf_size == 0)
    {
        return BLOOM_FALSE;
    }

    if (lines < 1)
    {
        lines = 1;
    }

    s = &ctx->style;
    id = bloom_get_id(label);
    state = bloom_text_edit_state_get(id);
    pos = ctx->current_window->layout.cursor;
    visible_len = bloom_label_visible_length(label);
    show_label = visible_len > 0;
    label_h = show_label ? bloom_scaled_line_height(ctx, s->font_size * 0.9f) : 0.0f;
    label_gap = show_label ? s->label_gap : 0.0f;
    line_h = bloom_scaled_line_height(ctx, s->font_size);
    input_w = ctx->current_window->layout.available_width;
    input_h = multiline ? (line_h * (bloom_f32)lines + s->field_padding_y * 2.0f)
                        : (line_h + s->field_padding_y * 2.0f);
    input_rect = bloom_make_rect(pos.x, pos.y + label_h + label_gap, input_w, input_h);
    hovered = bloom_widget_hovered(input_rect);

    state->caret = bloom_text_clamp_index(buf, state->caret);
    state->anchor = bloom_text_clamp_index(buf, state->anchor);

    if (hovered && ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT])
    {
        bloom_i32 click_index = bloom_text_index_from_point(ctx, input_rect, buf, s->font_size, multiline, ctx->input.mouse_pos);
        bloom_bool extend = (ctx->focus_id == id) && ctx->input.shift_held;

        ctx->focus_id = id;
        ctx->active_id = id;
        state->dragging = BLOOM_TRUE;
        bloom_text_set_caret(state, buf, click_index, extend);
        state->preferred_column = bloom_text_current_column(buf, state->caret);
    }
    else if (ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT] && !hovered && ctx->focus_id == id)
    {
        ctx->focus_id = 0;
        state->dragging = BLOOM_FALSE;
        if (ctx->active_id == id)
        {
            ctx->active_id = 0;
        }
    }

    if (ctx->active_id == id)
    {
        if (ctx->input.mouse_down[BLOOM_MOUSE_LEFT])
        {
            if (state->dragging)
            {
                bloom_i32 drag_index = bloom_text_index_from_point(ctx, input_rect, buf, s->font_size, multiline, ctx->input.mouse_pos);
                bloom_text_set_caret(state, buf, drag_index, BLOOM_TRUE);
                state->preferred_column = bloom_text_current_column(buf, state->caret);
            }
        }
        else
        {
            ctx->active_id = 0;
            state->dragging = BLOOM_FALSE;
        }
    }

    focused = (ctx->focus_id == id);

    if (show_label)
    {
        bloom_draw_label(ctx,
                         bloom_v2(pos.x, pos.y),
                         label,
                         focused ? s->input_cursor : s->text_disabled,
                         s->font_size * 0.9f);
    }

    bloom_draw_rect_rounded(&ctx->draw_list, input_rect, s->input_bg, s->input_rounding);
    bloom_draw_rect_rounded_border(&ctx->draw_list,
                                   input_rect,
                                   focused ? s->input_cursor : s->input_border,
                                   s->input_rounding,
                                   1.0f);

    if (focused)
    {
        changed |= bloom_text_edit_process_input(ctx, state, id, buf, buf_size, multiline);
    }

    bloom_draw_push_clip(&ctx->draw_list,
                         bloom_make_rect(input_rect.x + 1.0f, input_rect.y + 1.0f,
                                         input_rect.w - 2.0f, input_rect.h - 2.0f));
    bloom_draw_text_selection(ctx, input_rect, buf, s->font_size, multiline, state);
    bloom_draw_text_contents(ctx, input_rect, buf, s->input_text, s->font_size, multiline);

    if (focused)
    {
        bloom_f32 blink = (bloom_f32)ctx->time;
        if (((int)(blink * 2.0f)) % 2)
        {
            bloom_draw_text_cursor(ctx, input_rect, buf, s->font_size, multiline, state);
        }
    }
    bloom_draw_pop_clip(&ctx->draw_list);

    bloom_advance_layout(input_w, label_h + label_gap + input_h + s->touch_padding);
    return changed;
}

bloom_bool bloom_text_input(const char *label, char *buf, bloom_u32 buf_size)
{
    return bloom_text_input_internal(label, buf, buf_size, BLOOM_FALSE, 1);
}

bloom_bool bloom_text_area(const char *label, char *buf, bloom_u32 buf_size, bloom_i32 lines)
{
    return bloom_text_input_internal(label, buf, buf_size, BLOOM_TRUE, lines < 2 ? 2 : lines);
}
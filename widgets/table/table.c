#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "widgets/internal.h"
void bloom_begin_table(const char *label, bloom_i32 columns)
{
    bloom_context *ctx = bloom_get_context();
    if (!ctx || !ctx->current_window)
    {
        return;
    }
    (void)label;
    ctx->table_column_count = columns;
    ctx->table_current_column = 0;
    ctx->table_row_index = 0;

    bloom_f32 total_w = ctx->current_window->layout.available_width;
    bloom_f32 col_w = total_w / (bloom_f32)columns;
    int i;
    for (i = 0; i < columns && i < BLOOM_MAX_COLUMNS; i++)
    {
        ctx->table_column_widths[i] = col_w;
    }
    ctx->table_row_y = ctx->current_window->layout.cursor.y;
}

void bloom_end_table(void)
{
    bloom_context *ctx = bloom_get_context();
    if (!ctx || !ctx->current_window)
    {
        return;
    }
    ctx->current_window->layout.cursor.x =
        ctx->current_window->content_rect.x + ctx->style.window_padding;
    ctx->current_window->layout.cursor.y = ctx->table_row_y + bloom_table_row_height(ctx) + ctx->style.item_spacing;
    {
        bloom_window *win = ctx->current_window;
        win->layout.last_item_pos = bloom_v2(win->content_rect.x + ctx->style.window_padding, ctx->table_row_y);
        win->layout.last_item_size = bloom_v2(win->layout.available_width, bloom_table_row_height(ctx));
        bloom_f32 table_bottom = ctx->table_row_y + bloom_table_row_height(ctx) + win->scroll_y - win->content_rect.y;
        if (table_bottom > win->content_extent_y)
        {
            win->content_extent_y = table_bottom;
        }
    }
    ctx->table_column_count = 0;
}

void bloom_table_next_column(void)
{
    bloom_context *ctx = bloom_get_context();
    if (!ctx || !ctx->current_window)
    {
        return;
    }
    ctx->table_current_column++;
    if (ctx->table_current_column >= ctx->table_column_count)
    {
        bloom_table_next_row();
        return;
    }

    bloom_f32 offset = 0;
    int i;
    for (i = 0; i < ctx->table_current_column; i++)
    {
        offset += ctx->table_column_widths[i];
    }
    ctx->current_window->layout.cursor.x =
        ctx->current_window->content_rect.x + ctx->style.window_padding + offset + 12.0f;
    ctx->current_window->layout.cursor.y = ctx->table_row_y + 8.0f;
}

void bloom_table_next_row(void)
{
    bloom_context *ctx = bloom_get_context();
    if (!ctx || !ctx->current_window)
    {
        return;
    }
    ctx->table_current_column = 0;
    ctx->table_row_y += bloom_table_row_height(ctx);
    ctx->table_row_index++;

    bloom_color row_bg = (ctx->table_row_index % 2 == 0)
        ? ctx->style.table_row_bg : ctx->style.table_row_bg_alt;
    bloom_draw_rect_filled(&ctx->draw_list,
        bloom_make_rect(ctx->current_window->content_rect.x + ctx->style.window_padding,
                       ctx->table_row_y,
                       ctx->current_window->layout.available_width,
                       bloom_table_row_height(ctx)),
        row_bg);

    ctx->current_window->layout.cursor.x =
        ctx->current_window->content_rect.x + ctx->style.window_padding + 12.0f;
    ctx->current_window->layout.cursor.y = ctx->table_row_y + 8.0f;
}

void bloom_table_header(const char *label)
{
    bloom_context *ctx = bloom_get_context();
    if (!ctx || !ctx->current_window)
    {
        return;
    }
    bloom_style *s = &ctx->style;
    bloom_vec2 pos = ctx->current_window->layout.cursor;
    bloom_f32 col_w = 0;
    if (ctx->table_current_column < BLOOM_MAX_COLUMNS)
    {
        col_w = ctx->table_column_widths[ctx->table_current_column];
    }

    bloom_draw_rect_filled(&ctx->draw_list,
        bloom_make_rect(pos.x, pos.y, col_w, bloom_table_row_height(ctx)),
        s->table_header_bg);
    bloom_draw_label(ctx,
                   bloom_v2(pos.x + 12.0f,
                            bloom_centered_text_y(ctx, pos.y, bloom_table_row_height(ctx), s->font_size)),
                   label, s->title_text, s->font_size);

    if (ctx->table_current_column + 1 < ctx->table_column_count)
    {
        ctx->table_current_column++;
        ctx->current_window->layout.cursor.x = pos.x + col_w;
        ctx->current_window->layout.cursor.y = pos.y;
    }
}

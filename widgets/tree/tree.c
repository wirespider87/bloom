#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "widgets/internal.h"
static struct
{
    bloom_id id;
    bloom_bool open;
} g_tree_state[256];
static int g_tree_count = 0;

static bloom_bool *bloom_tree_find(bloom_id id)
{
    int i;
    for (i = 0; i < g_tree_count; i++)
    {
        if (g_tree_state[i].id == id)
        {
            return &g_tree_state[i].open;
        }
    }
    if (g_tree_count < 256)
    {
        g_tree_state[g_tree_count].id = id;
        g_tree_state[g_tree_count].open = BLOOM_FALSE;
        return &g_tree_state[g_tree_count++].open;
    }
    return NULL;
}

bloom_bool bloom_tree_node(const char *label)
{
    bloom_context *ctx = bloom_get_context();
    if (!ctx || !ctx->current_window)
    {
        return BLOOM_FALSE;
    }

    bloom_id id = bloom_get_id(label);
    bloom_bool *open = bloom_tree_find(id);
    if (!open)
    {
        return BLOOM_FALSE;
    }

    bloom_style *s = &ctx->style;
    bloom_vec2 pos = ctx->current_window->layout.cursor;
    bloom_f32 arrow_size = s->font_size;
    bloom_f32 text_w = bloom_label_width(ctx, label, s->font_size);
    bloom_f32 total_h = bloom_scaled_line_height(ctx, s->font_size) + s->touch_padding * 2.0f;
    bloom_f32 total_w = arrow_size + s->item_inner_spacing + text_w;
    bloom_rect hit = bloom_make_rect(pos.x, pos.y, total_w, total_h);

    if (bloom_widget_hovered(hit) && ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT])
    {
        *open = !(*open);
    }

    if (*open)
    {
        bloom_draw_triangle(&ctx->draw_list,
            bloom_v2(pos.x, pos.y + total_h * 0.5f - 4.0f),
            bloom_v2(pos.x + arrow_size * 0.5f, pos.y + total_h * 0.5f + 3.0f),
            bloom_v2(pos.x + arrow_size, pos.y + total_h * 0.5f - 4.0f),
            s->text_default);
    }
    else
    {
        bloom_draw_triangle(&ctx->draw_list,
            bloom_v2(pos.x + 2, pos.y + total_h * 0.5f - 6.0f),
            bloom_v2(pos.x + arrow_size - 2, pos.y + total_h * 0.5f),
            bloom_v2(pos.x + 2, pos.y + total_h * 0.5f + 6.0f),
            s->text_default);
    }

    bloom_draw_label(ctx,
                   bloom_v2(pos.x + arrow_size + s->item_inner_spacing,
                            bloom_centered_text_y(ctx, pos.y, total_h, s->font_size)),
                   label, s->text_default, s->font_size);

    bloom_advance_layout(total_w, total_h);

    if (*open)
    {
        bloom_indent(0);
    }
    return *open;
}

void bloom_tree_pop(void)
{
    bloom_unindent(0);
}

bloom_bool bloom_collapsing_header(const char *label)
{
    bloom_context *ctx = bloom_get_context();
    if (!ctx || !ctx->current_window)
    {
        return BLOOM_FALSE;
    }

    bloom_id id = bloom_get_id(label);
    bloom_bool *open = bloom_tree_find(id);
    if (!open)
    {
        return BLOOM_FALSE;
    }

    bloom_style *s = &ctx->style;
    bloom_vec2 pos = ctx->current_window->layout.cursor;
    bloom_f32 w = ctx->current_window->layout.available_width;
    bloom_f32 h = bloom_scaled_line_height(ctx, s->font_size) + s->control_padding_y;
    bloom_rect header_rect = bloom_make_rect(pos.x, pos.y, w, h);

    bloom_bool hovered = bloom_widget_hovered(header_rect);
    if (hovered && ctx->input.mouse_pressed[BLOOM_MOUSE_LEFT])
    {
        *open = !(*open);
    }

    bloom_color bg = s->title_bg;
    if (*open)
    {
        bg = bloom_apply_state_layer(bg, s->button_bg, 0.10f);
    }
    if (hovered)
    {
        bg = bloom_apply_state_layer(bg, s->button_bg, 0.08f);
    }
    bloom_draw_rect_rounded(&ctx->draw_list, header_rect, bg, 14.0f);

    bloom_f32 arrow_x = pos.x + 8;
    bloom_f32 arrow_y_c = pos.y + h * 0.5f;
    if (*open)
    {
        bloom_draw_triangle(&ctx->draw_list,
            bloom_v2(arrow_x - 4, arrow_y_c - 3),
            bloom_v2(arrow_x + 4, arrow_y_c - 3),
            bloom_v2(arrow_x, arrow_y_c + 3),
            s->title_text);
    }
    else
    {
        bloom_draw_triangle(&ctx->draw_list,
            bloom_v2(arrow_x - 3, arrow_y_c - 4),
            bloom_v2(arrow_x + 3, arrow_y_c),
            bloom_v2(arrow_x - 3, arrow_y_c + 4),
            s->title_text);
    }

    bloom_draw_label(ctx,
                   bloom_v2(pos.x + 24, bloom_centered_text_y(ctx, pos.y, h, s->font_size)),
                   label, s->title_text, s->font_size);

    bloom_advance_layout(w, h);
    return *open;
}

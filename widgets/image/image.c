#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "widgets/internal.h"

void bloom_image(bloom_u32 texture_id, bloom_f32 w, bloom_f32 h)
{
    bloom_context *ctx = bloom_get_context();
    bloom_vec2 pos;
    bloom_draw_list *dl;
    bloom_u32 old_tex;
    bloom_color white;

    if (!ctx || !ctx->current_window || texture_id == 0)
    {
        return;
    }

    pos = ctx->current_window->layout.cursor;
    dl = &ctx->draw_list;
    old_tex = dl->current_texture;
    dl->current_texture = texture_id;
    white = bloom_rgba(255, 255, 255, 255);

    /* emit two textured triangles (quad) */
    if (dl->vertex_count + 4 <= dl->vertex_capacity &&
        dl->index_count + 6 <= dl->index_capacity)
    {
        bloom_u32 base = dl->vertex_count;
        bloom_draw_idx *idx;
        bloom_vertex *v;

        v = &dl->vertices[base];
        v[0].pos = bloom_v2(pos.x,     pos.y);
        v[0].uv  = bloom_v2(0.0f, 0.0f);
        v[0].col  = bloom_color_to_u32(white);

        v[1].pos = bloom_v2(pos.x + w, pos.y);
        v[1].uv  = bloom_v2(1.0f, 0.0f);
        v[1].col  = bloom_color_to_u32(white);

        v[2].pos = bloom_v2(pos.x + w, pos.y + h);
        v[2].uv  = bloom_v2(1.0f, 1.0f);
        v[2].col  = bloom_color_to_u32(white);

        v[3].pos = bloom_v2(pos.x,     pos.y + h);
        v[3].uv  = bloom_v2(0.0f, 1.0f);
        v[3].col  = bloom_color_to_u32(white);

        dl->vertex_count += 4;

        idx = &dl->indices[dl->index_count];
        idx[0] = (bloom_draw_idx)base;
        idx[1] = (bloom_draw_idx)(base + 1);
        idx[2] = (bloom_draw_idx)(base + 2);
        idx[3] = (bloom_draw_idx)base;
        idx[4] = (bloom_draw_idx)(base + 2);
        idx[5] = (bloom_draw_idx)(base + 3);
        dl->index_count += 6;

        /* register draw command */
        {
            bloom_rect clip_rect;
            bloom_draw_cmd *cmd;

            if (dl->clip_depth > 0)
            {
                clip_rect = dl->clip_stack[dl->clip_depth - 1];
            }
            else
            {
                clip_rect = bloom_make_rect(-1, -1, -1, -1);
            }

            if (dl->cmd_count > 0)
            {
                bloom_draw_cmd *prev = &dl->commands[dl->cmd_count - 1];
                if (prev->type == BLOOM_DRAW_CMD_TRIANGLES &&
                    prev->texture_id == texture_id &&
                    prev->clip_rect.x == clip_rect.x &&
                    prev->clip_rect.y == clip_rect.y &&
                    prev->clip_rect.w == clip_rect.w &&
                    prev->clip_rect.h == clip_rect.h)
                {
                    prev->elem_count += 6;
                    goto done;
                }
            }

            if (dl->cmd_count < dl->cmd_capacity)
            {
                cmd = &dl->commands[dl->cmd_count++];
                cmd->type = BLOOM_DRAW_CMD_TRIANGLES;
                cmd->elem_count = 6;
                cmd->texture_id = texture_id;
                cmd->clip_rect = clip_rect;
            }
        }
    }

done:
    dl->current_texture = old_tex;
    bloom_advance_layout(w, h);
}

bloom_bool bloom_image_button(const char *label, bloom_u32 texture_id, bloom_f32 w, bloom_f32 h)
{
    bloom_context *ctx = bloom_get_context();
    bloom_style *s;
    bloom_id id;
    bloom_vec2 pos;
    bloom_rect rect;
    bloom_bool hovered;
    bloom_bool held;
    bloom_bool pressed;
    bloom_f32 hover_t;
    bloom_f32 press_t;

    if (!ctx || !ctx->current_window || texture_id == 0)
    {
        return BLOOM_FALSE;
    }

    s = &ctx->style;
    id = bloom_get_id(label);
    pos = ctx->current_window->layout.cursor;
    rect = bloom_make_rect(pos.x, pos.y, w, h);

    hovered = bloom_widget_hovered(rect);
    held = (ctx->active_id == id);
    pressed = BLOOM_FALSE;

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

    hover_t = bloom_window_anim_toggle(ctx, id ^ 0x7B01u, hovered || held, 16.0f);
    press_t = bloom_window_anim_pulse(ctx, id ^ 0x7B02u, held, 44.0f, 20.0f);

    /* hover glow */
    if (hover_t > 0.001f)
    {
        bloom_draw_rect_rounded(&ctx->draw_list,
            bloom_make_rect(rect.x - 2.0f, rect.y - 2.0f, rect.w + 4.0f, rect.h + 4.0f),
            bloom_apply_state_layer(s->window_bg, s->input_cursor,
                                    0.04f + hover_t * 0.06f + press_t * 0.04f),
            s->button_rounding + 2.0f);
    }

    /* draw the image itself inside the rect */
    {
        bloom_draw_list *dl = &ctx->draw_list;
        bloom_u32 old_tex = dl->current_texture;
        bloom_color white = bloom_rgba(255, 255, 255, 255);
        dl->current_texture = texture_id;

        if (dl->vertex_count + 4 <= dl->vertex_capacity &&
            dl->index_count + 6 <= dl->index_capacity)
        {
            bloom_u32 base = dl->vertex_count;
            bloom_draw_idx *idx;
            bloom_vertex *v;

            v = &dl->vertices[base];
            v[0].pos = bloom_v2(rect.x, rect.y);          v[0].uv = bloom_v2(0, 0); v[0].col = bloom_color_to_u32(white);
            v[1].pos = bloom_v2(rect.x + w, rect.y);      v[1].uv = bloom_v2(1, 0); v[1].col = bloom_color_to_u32(white);
            v[2].pos = bloom_v2(rect.x + w, rect.y + h);  v[2].uv = bloom_v2(1, 1); v[2].col = bloom_color_to_u32(white);
            v[3].pos = bloom_v2(rect.x, rect.y + h);      v[3].uv = bloom_v2(0, 1); v[3].col = bloom_color_to_u32(white);
            dl->vertex_count += 4;

            idx = &dl->indices[dl->index_count];
            idx[0] = (bloom_draw_idx)base;     idx[1] = (bloom_draw_idx)(base + 1); idx[2] = (bloom_draw_idx)(base + 2);
            idx[3] = (bloom_draw_idx)base;     idx[4] = (bloom_draw_idx)(base + 2); idx[5] = (bloom_draw_idx)(base + 3);
            dl->index_count += 6;

            {
                bloom_rect clip_rect;
                if (dl->clip_depth > 0)
                    clip_rect = dl->clip_stack[dl->clip_depth - 1];
                else
                    clip_rect = bloom_make_rect(-1, -1, -1, -1);

                if (dl->cmd_count > 0)
                {
                    bloom_draw_cmd *prev = &dl->commands[dl->cmd_count - 1];
                    if (prev->type == BLOOM_DRAW_CMD_TRIANGLES &&
                        prev->texture_id == texture_id &&
                        prev->clip_rect.x == clip_rect.x &&
                        prev->clip_rect.y == clip_rect.y &&
                        prev->clip_rect.w == clip_rect.w &&
                        prev->clip_rect.h == clip_rect.h)
                    {
                        prev->elem_count += 6;
                        goto img_done;
                    }
                }

                if (dl->cmd_count < dl->cmd_capacity)
                {
                    bloom_draw_cmd *cmd = &dl->commands[dl->cmd_count++];
                    cmd->type = BLOOM_DRAW_CMD_TRIANGLES;
                    cmd->elem_count = 6;
                    cmd->texture_id = texture_id;
                    cmd->clip_rect = clip_rect;
                }
            }
        }

    img_done:
        dl->current_texture = old_tex;
    }

    bloom_advance_layout(w, h);
    return pressed;
}

#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "core/runtime/animation/animation.h"
#include "core/runtime/context/context.h"

#include <math.h>
#include <string.h>

#ifndef BLOOM_ANIM_PI
#define BLOOM_ANIM_PI 3.14159265358979323846f
#endif

static bloom_anim_track *bloom_anim_get_track(bloom_context *ctx, bloom_id id, bloom_bool create)
{
    bloom_anim_track *free_track = NULL;
    bloom_anim_track *oldest_track = NULL;
    bloom_u32 oldest_frame = 0xFFFFFFFFu;
    bloom_i32 i;

    if (!ctx || id == 0)
    {
        return NULL;
    }

    for (i = 0; i < BLOOM_MAX_ANIM_STATES; ++i)
    {
        bloom_anim_track *track = &ctx->anim_tracks[i];
        if (track->id == id)
        {
            return track;
        }
        if (track->id == 0 && !free_track)
        {
            free_track = track;
        }
        if (track->last_frame < oldest_frame)
        {
            oldest_frame = track->last_frame;
            oldest_track = track;
        }
    }

    if (!create)
    {
        return NULL;
    }

    if (!free_track)
    {
        free_track = oldest_track;
    }

    if (!free_track)
    {
        return NULL;
    }

    memset(free_track, 0, sizeof(*free_track));
    free_track->id = id;
    return free_track;
}

static bloom_f32 bloom_anim_response_alpha(bloom_context *ctx, bloom_f32 response)
{
    bloom_f32 dt;
    bloom_f32 alpha;

    if (!ctx)
    {
        return 1.0f;
    }

    if (response <= 0.0f)
    {
        return 1.0f;
    }

    dt = ctx->delta_time;
    if (dt <= 0.0f)
    {
        return 1.0f;
    }

    alpha = 1.0f - expf(-response * dt);
    if (alpha < 0.0f)
    {
        alpha = 0.0f;
    }
    if (alpha > 1.0f)
    {
        alpha = 1.0f;
    }
    return alpha;
}

static bloom_f32 bloom_anim_time_seconds(void)
{
    bloom_context *ctx = bloom_get_context();
    return ctx ? (bloom_f32)ctx->time : 0.0f;
}

bloom_f32 bloom_anim_clamp01(bloom_f32 t)
{
    if (t < 0.0f)
    {
        return 0.0f;
    }
    if (t > 1.0f)
    {
        return 1.0f;
    }
    return t;
}

bloom_f32 bloom_anim_ease_apply(bloom_f32 t, bloom_anim_ease ease)
{
    t = bloom_anim_clamp01(t);

    switch (ease)
    {
    case BLOOM_ANIM_EASE_SMOOTHSTEP:
        return t * t * (3.0f - 2.0f * t);
    case BLOOM_ANIM_EASE_SMOOTHERSTEP:
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    case BLOOM_ANIM_EASE_IN_QUAD:
        return t * t;
    case BLOOM_ANIM_EASE_OUT_QUAD:
        return 1.0f - (1.0f - t) * (1.0f - t);
    case BLOOM_ANIM_EASE_IN_OUT_QUAD:
        if (t < 0.5f)
        {
            return 2.0f * t * t;
        }
        return 1.0f - powf(-2.0f * t + 2.0f, 2.0f) * 0.5f;
    case BLOOM_ANIM_EASE_IN_OUT_CUBIC:
        if (t < 0.5f)
        {
            return 4.0f * t * t * t;
        }
        return 1.0f - powf(-2.0f * t + 2.0f, 3.0f) * 0.5f;
    case BLOOM_ANIM_EASE_OUT_BACK:
        {
            bloom_f32 c1 = 1.70158f;
            bloom_f32 c3 = c1 + 1.0f;
            bloom_f32 inv = t - 1.0f;
            return 1.0f + c3 * inv * inv * inv + c1 * inv * inv;
        }
    case BLOOM_ANIM_EASE_LINEAR:
    default:
        return t;
    }
}

bloom_f32 bloom_anim_state(bloom_id id, bloom_f32 target, bloom_f32 response)
{
    bloom_context *ctx = bloom_get_context();
    bloom_anim_track *track;
    bloom_f32 alpha;

    if (!ctx || id == 0)
    {
        return target;
    }

    track = bloom_anim_get_track(ctx, id, BLOOM_TRUE);
    if (!track)
    {
        return target;
    }

    if (track->last_frame == 0)
    {
        track->value = target;
        track->aux = target;
    }
    else if (track->last_frame != ctx->frame_count || fabsf(track->aux - target) > 0.0005f)
    {
        alpha = bloom_anim_response_alpha(ctx, response);
        track->value += (target - track->value) * alpha;
        track->aux = target;
    }

    if (fabsf(target - track->value) < 0.0005f)
    {
        track->value = target;
    }

    track->last_frame = ctx->frame_count;
    return track->value;
}

bloom_f32 bloom_anim_toggle(bloom_id id, bloom_bool on, bloom_f32 response)
{
    return bloom_anim_state(id, on ? 1.0f : 0.0f, response);
}

bloom_f32 bloom_anim_range(bloom_id id, bloom_f32 from, bloom_f32 to,
                           bloom_f32 target_t, bloom_f32 response,
                           bloom_anim_ease ease)
{
    bloom_f32 t = bloom_anim_state(id, bloom_anim_clamp01(target_t), response);
    t = bloom_anim_ease_apply(t, ease);
    return from + (to - from) * t;
}

bloom_f32 bloom_anim_pulse(bloom_id id, bloom_bool active,
                           bloom_f32 attack, bloom_f32 decay)
{
    bloom_context *ctx = bloom_get_context();
    bloom_anim_track *track;
    bloom_f32 response;
    bloom_f32 target;

    if (!ctx || id == 0)
    {
        return active ? 1.0f : 0.0f;
    }

    track = bloom_anim_get_track(ctx, id, BLOOM_TRUE);
    if (!track)
    {
        return active ? 1.0f : 0.0f;
    }

    target = active ? 1.0f : 0.0f;
    if (track->last_frame == 0)
    {
        track->value = target;
        track->aux = target;
    }
    else
    {
        response = (target >= track->value) ? attack : decay;
        track->value += (target - track->value) * bloom_anim_response_alpha(ctx, response);
        track->aux = target;
    }
    if (fabsf(target - track->value) < 0.0005f)
    {
        track->value = target;
    }
    track->last_frame = ctx->frame_count;
    return track->value;
}

bloom_f32 bloom_anim_spring(bloom_id id, bloom_f32 target,
                            bloom_f32 stiffness, bloom_f32 damping)
{
    bloom_context *ctx = bloom_get_context();
    bloom_anim_track *track;
    bloom_f32 dt;
    bloom_f32 accel;

    if (!ctx || id == 0)
    {
        return target;
    }

    track = bloom_anim_get_track(ctx, id, BLOOM_TRUE);
    if (!track)
    {
        return target;
    }

    if (stiffness <= 0.0f)
    {
        stiffness = 140.0f;
    }
    if (damping <= 0.0f)
    {
        damping = 20.0f;
    }

    if (track->last_frame == 0)
    {
        track->value = target;
        track->aux = 0.0f;
        track->aux2 = target;
        track->last_frame = ctx->frame_count;
        return track->value;
    }

    dt = ctx->delta_time;
    if (dt <= 0.0f)
    {
        dt = 1.0f / 60.0f;
    }
    if (dt > 0.05f)
    {
        dt = 0.05f;
    }

    accel = (target - track->value) * stiffness;
    track->aux += accel * dt;
    track->aux *= expf(-damping * dt);
    track->value += track->aux * dt;
    track->aux2 = target;

    if (fabsf(target - track->value) < 0.0005f && fabsf(track->aux) < 0.0005f)
    {
        track->value = target;
        track->aux = 0.0f;
    }

    track->last_frame = ctx->frame_count;
    return track->value;
}

bloom_f32 bloom_anim_delay(bloom_id id, bloom_bool active,
                           bloom_f32 delay, bloom_f32 response)
{
    bloom_context *ctx = bloom_get_context();
    bloom_anim_track *track;
    bloom_id output_id;
    bloom_bool last_active;
    bloom_f32 target;

    if (!ctx || id == 0)
    {
        return active ? 1.0f : 0.0f;
    }

    track = bloom_anim_get_track(ctx, id, BLOOM_TRUE);
    if (!track)
    {
        return active ? 1.0f : 0.0f;
    }

    if (delay < 0.0f)
    {
        delay = 0.0f;
    }

    output_id = id ^ 0x7A11C0DEu;

    last_active = track->aux2 > 0.5f;
    if (track->last_frame == 0)
    {
        track->value = active ? 1.0f : 0.0f;
        track->aux = active ? ((bloom_f32)ctx->time + delay) : 0.0f;
        track->aux2 = active ? 1.0f : 0.0f;
        track->last_frame = ctx->frame_count;
        return bloom_anim_state(output_id, (active && delay <= 0.0f) ? 1.0f : 0.0f, response);
    }

    if (active && !last_active)
    {
        track->aux = (bloom_f32)ctx->time + delay;
    }
    else if (!active)
    {
        track->aux = 0.0f;
    }

    track->aux2 = active ? 1.0f : 0.0f;
    target = (active && (bloom_f32)ctx->time >= track->aux) ? 1.0f : 0.0f;
    track->last_frame = ctx->frame_count;
    return bloom_anim_state(output_id, target, response);
}

bloom_f32 bloom_anim_stagger(bloom_id id, bloom_bool active, bloom_i32 index,
                             bloom_f32 step_delay, bloom_f32 response)
{
    bloom_id indexed_id;

    if (index < 0)
    {
        index = 0;
    }
    if (step_delay < 0.0f)
    {
        step_delay = 0.0f;
    }

    indexed_id = bloom_hash_bytes(&index, sizeof(index), id);
    return bloom_anim_delay(indexed_id, active, step_delay * (bloom_f32)index, response);
}

bloom_f32 bloom_anim_loop(bloom_f32 speed, bloom_f32 offset)
{
    bloom_f32 value = bloom_anim_time_seconds() * speed + offset;
    value -= floorf(value);
    if (value < 0.0f)
    {
        value += 1.0f;
    }
    return value;
}

bloom_f32 bloom_anim_ping_pong(bloom_f32 speed, bloom_f32 offset)
{
    bloom_f32 phase = bloom_anim_loop(speed, offset);
    return phase < 0.5f ? (phase * 2.0f) : (2.0f - phase * 2.0f);
}

bloom_f32 bloom_anim_sine(bloom_f32 speed, bloom_f32 offset)
{
    bloom_f32 angle = (bloom_anim_time_seconds() * speed + offset) * (BLOOM_ANIM_PI * 2.0f);
    return 0.5f + sinf(angle) * 0.5f;
}

void bloom_anim_reset(bloom_id id)
{
    bloom_context *ctx = bloom_get_context();
    bloom_anim_track *track;

    if (!ctx || id == 0)
    {
        return;
    }

    track = bloom_anim_get_track(ctx, id, BLOOM_FALSE);
    if (track)
    {
        memset(track, 0, sizeof(*track));
    }
}

void bloom_anim_reset_all(void)
{
    bloom_context *ctx = bloom_get_context();
    if (!ctx)
    {
        return;
    }

    memset(ctx->anim_tracks, 0, sizeof(ctx->anim_tracks));
}

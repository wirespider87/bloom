#ifndef BLOOM_CORE_RUNTIME_ANIMATION_H
#define BLOOM_CORE_RUNTIME_ANIMATION_H

#include "core/base/types/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum bloom_anim_ease
{
    BLOOM_ANIM_EASE_LINEAR = 0,
    BLOOM_ANIM_EASE_SMOOTHSTEP,
    BLOOM_ANIM_EASE_SMOOTHERSTEP,
    BLOOM_ANIM_EASE_IN_QUAD,
    BLOOM_ANIM_EASE_OUT_QUAD,
    BLOOM_ANIM_EASE_IN_OUT_QUAD,
    BLOOM_ANIM_EASE_IN_OUT_CUBIC,
    BLOOM_ANIM_EASE_OUT_BACK
} bloom_anim_ease;

typedef struct bloom_anim_track
{
    bloom_id  id;
    bloom_f32 value;
    bloom_f32 aux;
    bloom_f32 aux2;
    bloom_u32 last_frame;
} bloom_anim_track;

#if defined(BLOOM_INTERNAL_BUILD) || defined(BLOOM_ENABLE_LEGACY_API)
bloom_f32 bloom_anim_clamp01(bloom_f32 t);
bloom_f32 bloom_anim_ease_apply(bloom_f32 t, bloom_anim_ease ease);
bloom_f32 bloom_anim_state(bloom_id id, bloom_f32 target, bloom_f32 response);
bloom_f32 bloom_anim_toggle(bloom_id id, bloom_bool on, bloom_f32 response);
bloom_f32 bloom_anim_range(bloom_id id, bloom_f32 from, bloom_f32 to,
                           bloom_f32 target_t, bloom_f32 response,
                           bloom_anim_ease ease);
bloom_f32 bloom_anim_pulse(bloom_id id, bloom_bool active,
                           bloom_f32 attack, bloom_f32 decay);
bloom_f32 bloom_anim_spring(bloom_id id, bloom_f32 target,
                            bloom_f32 stiffness, bloom_f32 damping);
bloom_f32 bloom_anim_delay(bloom_id id, bloom_bool active,
                           bloom_f32 delay, bloom_f32 response);
bloom_f32 bloom_anim_stagger(bloom_id id, bloom_bool active, bloom_i32 index,
                             bloom_f32 step_delay, bloom_f32 response);
bloom_f32 bloom_anim_loop(bloom_f32 speed, bloom_f32 offset);
bloom_f32 bloom_anim_ping_pong(bloom_f32 speed, bloom_f32 offset);
bloom_f32 bloom_anim_sine(bloom_f32 speed, bloom_f32 offset);
void bloom_anim_reset(bloom_id id);
void bloom_anim_reset_all(void);
#endif

#ifdef __cplusplus
}
#endif

#endif

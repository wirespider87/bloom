#ifndef BLOOM_CORE_BASE_TYPES_H
#define BLOOM_CORE_BASE_TYPES_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t  bloom_u8;
typedef uint16_t bloom_u16;
typedef uint32_t bloom_u32;
typedef uint64_t bloom_u64;
typedef int8_t   bloom_i8;
typedef int16_t  bloom_i16;
typedef int32_t  bloom_i32;
typedef int64_t  bloom_i64;
typedef float    bloom_f32;
typedef double   bloom_f64;
typedef int      bloom_bool;

#define BLOOM_TRUE  1
#define BLOOM_FALSE 0

#define BLOOM_MAX_WINDOWS     128
#define BLOOM_MAX_ID_STACK    64
#define BLOOM_MAX_CLIP_STACK  64
#define BLOOM_MAX_COLUMNS     32
#define BLOOM_MAX_TEXT_LEN    4096
#define BLOOM_MAX_DRAW_CMDS   4096
#define BLOOM_MAX_VERTICES    65536
#define BLOOM_MAX_INDICES     131072
#define BLOOM_MAX_POPUPS      32
#define BLOOM_MAX_ANIM_STATES 1024
#define BLOOM_ARENA_SIZE      (4 * 1024 * 1024)
#define BLOOM_HASH_SEED       0x811c9dc5u

typedef struct bloom_vec2
{
    bloom_f32 x;
    bloom_f32 y;
} bloom_vec2;

typedef struct bloom_vec4
{
    bloom_f32 x;
    bloom_f32 y;
    bloom_f32 z;
    bloom_f32 w;
} bloom_vec4;

typedef struct bloom_rect
{
    bloom_f32 x;
    bloom_f32 y;
    bloom_f32 w;
    bloom_f32 h;
} bloom_rect;

typedef struct bloom_color
{
    bloom_u8 r;
    bloom_u8 g;
    bloom_u8 b;
    bloom_u8 a;
} bloom_color;

static inline bloom_color bloom_rgba(bloom_u8 r, bloom_u8 g, bloom_u8 b, bloom_u8 a)
{
    bloom_color c;
    c.r = r; c.g = g; c.b = b; c.a = a;
    return c;
}

static inline bloom_color bloom_rgb(bloom_u8 r, bloom_u8 g, bloom_u8 b)
{
    return bloom_rgba(r, g, b, 255);
}

static inline bloom_u32 bloom_color_to_u32(bloom_color c)
{
    return ((bloom_u32)c.a << 24) | ((bloom_u32)c.b << 16) |
           ((bloom_u32)c.g << 8)  | ((bloom_u32)c.r);
}

static inline bloom_color bloom_color_from_u32(bloom_u32 v)
{
    bloom_color c;
    c.r = (bloom_u8)(v & 0xFF);
    c.g = (bloom_u8)((v >> 8) & 0xFF);
    c.b = (bloom_u8)((v >> 16) & 0xFF);
    c.a = (bloom_u8)((v >> 24) & 0xFF);
    return c;
}

static inline bloom_vec2 bloom_v2(bloom_f32 x, bloom_f32 y)
{
    bloom_vec2 v;
    v.x = x; v.y = y;
    return v;
}

static inline bloom_rect bloom_make_rect(bloom_f32 x, bloom_f32 y, bloom_f32 w, bloom_f32 h)
{
    bloom_rect r;
    r.x = x; r.y = y; r.w = w; r.h = h;
    return r;
}

static inline bloom_bool bloom_rect_contains(bloom_rect r, bloom_vec2 p)
{
    return p.x >= r.x && p.x < r.x + r.w &&
           p.y >= r.y && p.y < r.y + r.h;
}

static inline bloom_rect bloom_rect_intersect(bloom_rect a, bloom_rect b)
{
    bloom_f32 x1 = a.x > b.x ? a.x : b.x;
    bloom_f32 y1 = a.y > b.y ? a.y : b.y;
    bloom_f32 x2a = a.x + a.w;
    bloom_f32 x2b = b.x + b.w;
    bloom_f32 y2a = a.y + a.h;
    bloom_f32 y2b = b.y + b.h;
    bloom_f32 x2 = x2a < x2b ? x2a : x2b;
    bloom_f32 y2 = y2a < y2b ? y2a : y2b;
    bloom_rect result;
    result.x = x1;
    result.y = y1;
    result.w = x2 - x1 > 0 ? x2 - x1 : 0;
    result.h = y2 - y1 > 0 ? y2 - y1 : 0;
    return result;
}

typedef bloom_u32 bloom_id;

#ifdef __cplusplus
}
#endif

#endif

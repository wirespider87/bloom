#ifndef BLOOM_CORE_GRAPHICS_FONT_H
#define BLOOM_CORE_GRAPHICS_FONT_H

#include "core/base/types/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bloom_glyph
{
    bloom_u32   codepoint;
    bloom_f32   advance;
    bloom_f32   x0, y0, x1, y1;
    bloom_f32   u0, v0, u1, v1;
} bloom_glyph;

typedef struct bloom_font
{
    bloom_glyph *glyphs;
    bloom_u32    glyph_count;
    bloom_u32    glyph_capacity;
    bloom_f32    size;
    bloom_f32    ascent;
    bloom_f32    descent;
    bloom_f32    line_height;

    bloom_u8    *atlas_pixels;
    bloom_u32    atlas_width;
    bloom_u32    atlas_height;
    bloom_u32    texture_id;

    bloom_bool   valid;
} bloom_font;

#if defined(BLOOM_INTERNAL_BUILD) || defined(BLOOM_ENABLE_LEGACY_API)
void bloom_font_init(bloom_font *font);
bloom_bool bloom_font_build_default(bloom_font *font, bloom_f32 size);
bloom_bool bloom_font_load_from_memory(bloom_font *font, const bloom_u8 *data, bloom_u64 size, bloom_f32 pixel_size);
void bloom_font_destroy(bloom_font *font);
void bloom_font_set_active(bloom_font *font);
bloom_font *bloom_font_get_active(void);

bloom_f32 bloom_font_text_width(bloom_font *font, const char *text);
bloom_f32 bloom_font_char_width(bloom_font *font, bloom_u32 codepoint);
#endif

#ifdef __cplusplus
}
#endif

#endif

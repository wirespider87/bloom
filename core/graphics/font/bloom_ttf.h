#ifndef BLOOM_CORE_GRAPHICS_FONT_BLOOM_TTF_H
#define BLOOM_CORE_GRAPHICS_FONT_BLOOM_TTF_H

#include "core/base/types/types.h"

typedef struct bloom_ttf_font
{
    const bloom_u8 *data;
    int             size;
    bloom_u32       off_cmap;
    bloom_u32       off_glyf;
    bloom_u32       off_loca;
    bloom_u32       off_head;
    bloom_u32       off_maxp;
    bloom_u32       off_hhea;
    bloom_u32       off_hmtx;
    bloom_u16       units_per_em;
    bloom_u16       index_to_loc_format;
    bloom_u16       num_glyphs;
} bloom_ttf_font;

bloom_bool bloom_ttf_init(bloom_ttf_font *f, const bloom_u8 *data, int size);
int        bloom_ttf_cmap_lookup(const bloom_ttf_font *f, bloom_u32 cp);
void       bloom_ttf_glyph_hmetrics(const bloom_ttf_font *f, int gidx, int *adv, int *lsb);
void       bloom_ttf_line_metrics(const bloom_ttf_font *f, int *ascent, int *descent, int *line_gap);
/* Renders glyph into buf (grayscale 0-255). scale_px is em height in pixels. */
bloom_bool bloom_ttf_render_glyph(const bloom_ttf_font *f, int gidx, bloom_f32 scale_px,
                                  bloom_u8 *buf, int buf_w, int buf_h, int stride,
                                  int *out_w, int *out_h);

#endif

#ifndef BLOOM_CORE_GRAPHICS_FONT_BLOOM_TEXT_SHAPE_H
#define BLOOM_CORE_GRAPHICS_FONT_BLOOM_TEXT_SHAPE_H

#include "core/base/types/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Decode UTF-8 (text_len = byte count), apply simplified bidirectional reorder per line (LTR UI:
 * reverse contiguous Arabic/Hebrew/presentation-form runs). Arabic contextual joining to
 * presentation glyphs and color emoji need a larger atlas or OpenType layout - not applied here.
 * Writes at most max_out scalars; returns count. Newlines preserved as U+000A.
 */
int bloom_text_shape_visual(const char *text, bloom_u32 text_len, bloom_u32 *out, int max_out);

#ifdef __cplusplus
}
#endif

#endif

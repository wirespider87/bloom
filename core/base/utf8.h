#ifndef BLOOM_CORE_BASE_UTF8_H
#define BLOOM_CORE_BASE_UTF8_H

#include "core/base/types/types.h"

#ifdef __cplusplus
extern "C" {
#endif

bloom_u32 bloom_utf8_decode_one(const char *text, bloom_u32 text_len, bloom_u32 *out_byte_len);

/* Snap index forward to the next UTF-8 character boundary (0..len). */
bloom_i32 bloom_utf8_snap_to_boundary(const char *text, bloom_i32 len, bloom_i32 index);

/* Byte index of the first byte of the scalar before `index` (for backspace / left arrow). */
bloom_i32 bloom_utf8_prior_char(const char *text, bloom_i32 index);

/* Byte index after the scalar that starts at `index` (index must be on a boundary). */
bloom_i32 bloom_utf8_next_char(const char *text, bloom_i32 len, bloom_i32 index);

#ifdef __cplusplus
}
#endif

#endif

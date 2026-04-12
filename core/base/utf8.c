#include "core/base/utf8.h"

#define BLOOM_U_REPLACEMENT 0xFFFDu

bloom_u32 bloom_utf8_decode_one(const char *text, bloom_u32 text_len, bloom_u32 *out_byte_len)
{
    bloom_u8 c0;
    bloom_u8 c1;
    bloom_u8 c2;
    bloom_u8 c3;
    bloom_u32 cp;

    if (!text || !out_byte_len || text_len == 0)
    {
        if (out_byte_len)
        {
            *out_byte_len = 0;
        }
        return 0;
    }

    c0 = (bloom_u8)text[0];
    if (c0 < 0x80u)
    {
        *out_byte_len = 1;
        return (bloom_u32)c0;
    }

    if ((c0 & 0xE0u) == 0xC0u)
    {
        if (text_len < 2u)
        {
            *out_byte_len = 1;
            return BLOOM_U_REPLACEMENT;
        }
        c1 = (bloom_u8)text[1];
        if ((c1 & 0xC0u) != 0x80u)
        {
            *out_byte_len = 1;
            return BLOOM_U_REPLACEMENT;
        }
        cp = ((bloom_u32)(c0 & 0x1Fu) << 6) | (bloom_u32)(c1 & 0x3Fu);
        if (cp < 0x80u)
        {
            *out_byte_len = 2;
            return BLOOM_U_REPLACEMENT;
        }
        *out_byte_len = 2;
        return cp;
    }

    if ((c0 & 0xF0u) == 0xE0u)
    {
        if (text_len < 3u)
        {
            *out_byte_len = 1;
            return BLOOM_U_REPLACEMENT;
        }
        c1 = (bloom_u8)text[1];
        c2 = (bloom_u8)text[2];
        if (((c1 & 0xC0u) != 0x80u) || ((c2 & 0xC0u) != 0x80u))
        {
            *out_byte_len = 1;
            return BLOOM_U_REPLACEMENT;
        }
        cp = ((bloom_u32)(c0 & 0x0Fu) << 12) | ((bloom_u32)(c1 & 0x3Fu) << 6) | (bloom_u32)(c2 & 0x3Fu);
        if (cp < 0x800u || (cp >= 0xD800u && cp <= 0xDFFFu))
        {
            *out_byte_len = 3;
            return BLOOM_U_REPLACEMENT;
        }
        *out_byte_len = 3;
        return cp;
    }

    if ((c0 & 0xF8u) == 0xF0u)
    {
        if (text_len < 4u)
        {
            *out_byte_len = 1;
            return BLOOM_U_REPLACEMENT;
        }
        c1 = (bloom_u8)text[1];
        c2 = (bloom_u8)text[2];
        c3 = (bloom_u8)text[3];
        if (((c1 & 0xC0u) != 0x80u) || ((c2 & 0xC0u) != 0x80u) || ((c3 & 0xC0u) != 0x80u))
        {
            *out_byte_len = 1;
            return BLOOM_U_REPLACEMENT;
        }
        cp = ((bloom_u32)(c0 & 0x07u) << 18) | ((bloom_u32)(c1 & 0x3Fu) << 12) |
             ((bloom_u32)(c2 & 0x3Fu) << 6) | (bloom_u32)(c3 & 0x3Fu);
        if (cp < 0x10000u || cp > 0x10FFFFu)
        {
            *out_byte_len = 4;
            return BLOOM_U_REPLACEMENT;
        }
        *out_byte_len = 4;
        return cp;
    }

    *out_byte_len = 1;
    return BLOOM_U_REPLACEMENT;
}

bloom_i32 bloom_utf8_snap_to_boundary(const char *text, bloom_i32 len, bloom_i32 index)
{
    if (!text || len <= 0)
    {
        return 0;
    }
    if (index <= 0)
    {
        return 0;
    }
    if (index >= len)
    {
        return len;
    }
    while (index < len && ((unsigned char)text[index] & 0xC0u) == 0x80u)
    {
        index++;
    }
    return index;
}

bloom_i32 bloom_utf8_prior_char(const char *text, bloom_i32 index)
{
    bloom_i32 i;

    if (!text || index <= 0)
    {
        return 0;
    }
    i = index - 1;
    while (i > 0 && ((unsigned char)text[i] & 0xC0u) == 0x80u)
    {
        i--;
    }
    return i;
}

bloom_i32 bloom_utf8_next_char(const char *text, bloom_i32 len, bloom_i32 index)
{
    bloom_u32 bl;

    if (!text || index < 0 || index >= len)
    {
        return len;
    }
    bloom_utf8_decode_one(text + index, (bloom_u32)(len - index), &bl);
    if (bl == 0u)
    {
        bl = 1u;
    }
    return index + (bloom_i32)bl;
}

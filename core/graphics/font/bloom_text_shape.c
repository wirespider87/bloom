/*
 * Needs a lot of work to be useful. Too overly simplified.
 */

#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "core/graphics/font/bloom_text_shape.h"
#include "core/base/utf8.h"
#include <string.h>

static int bidi_strong(bloom_u32 cp)
{
    if ((cp >= 0x0600u && cp <= 0x06FFu) || (cp >= 0x0750u && cp <= 0x077Fu) || (cp >= 0x08A0u && cp <= 0x08FFu))
    {
        return 1;
    }
    if (cp >= 0x0590u && cp <= 0x05FFu)
    {
        return 1;
    }
    if ((cp >= 0x0041u && cp <= 0x005Au) || (cp >= 0x0061u && cp <= 0x007Au))
    {
        return -1;
    }
    if (cp >= 0x0400u && cp <= 0x052Fu)
    {
        return -1;
    }
    if (cp >= 0x4E00u && cp <= 0x9FFFu)
    {
        return -1;
    }
    return 0;
}

static void reverse_range(bloom_u32 *a, int lo, int hi)
{
    while (lo < hi)
    {
        bloom_u32 t = a[lo];
        a[lo] = a[hi];
        a[hi] = t;
        ++lo;
        --hi;
    }
}

static int bidi_is_rtl_cluster(bloom_u32 cp)
{
    if (bidi_strong(cp) > 0)
    {
        return 1;
    }
    if (cp >= 0xFE70u && cp <= 0xFEFFu)
    {
        return 1;
    }
    return 0;
}

/* LTR-first UI: reverse each contiguous RTL script run for left-to-right drawing. */
static void bidi_reorder_line(bloom_u32 *buf, int n)
{
    int i;
    int r;

    if (n <= 0)
    {
        return;
    }

    i = 0;
    while (i < n)
    {
        while (i < n && (buf[i] == '\n' || !bidi_is_rtl_cluster(buf[i])))
        {
            ++i;
        }
        if (i >= n)
        {
            break;
        }
        r = i;
        while (i < n && buf[i] != '\n' && bidi_is_rtl_cluster(buf[i]))
        {
            ++i;
        }
        if (i > r)
        {
            reverse_range(buf, r, i - 1);
        }
    }
}

int bloom_text_shape_visual(const char *text, bloom_u32 text_len, bloom_u32 *out, int max_out)
{
    bloom_u32 stack_buf[4096];
    int n = 0;
    const char *end;
    const char *p;

    if (!text || max_out <= 0)
    {
        return 0;
    }

    if (text_len > 4096u * 4u)
    {
        text_len = 4096u * 4u;
    }

    end = text + text_len;
    for (p = text; p < end && n < 4096;)
    {
        bloom_u32 bl;
        bloom_u32 cp = bloom_utf8_decode_one(p, (bloom_u32)(end - p), &bl);

        if (bl == 0)
        {
            bl = 1;
        }
        p += bl;
        stack_buf[n++] = cp;
    }

    {
        int i;
        int line0 = 0;
        for (i = 0; i <= n; ++i)
        {
            if (i == n || stack_buf[i] == '\n')
            {
                bidi_reorder_line(stack_buf + line0, i - line0);
                line0 = i + 1;
            }
        }
    }

    if (n > max_out)
    {
        n = max_out;
    }
    memcpy(out, stack_buf, (size_t)n * sizeof(bloom_u32));
    return n;
}

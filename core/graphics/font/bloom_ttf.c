#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "core/graphics/font/bloom_ttf.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define TTF_TAG(a, b, c, d) (((bloom_u32)(a) << 24) | ((bloom_u32)(b) << 16) | ((bloom_u32)(c) << 8) | (bloom_u32)(d))

static bloom_u16 ttf_rd_u16(const bloom_u8 *p)
{
    return (bloom_u16)((p[0] << 8) | p[1]);
}

static bloom_i16 ttf_rd_i16(const bloom_u8 *p)
{
    return (bloom_i16)ttf_rd_u16(p);
}

static bloom_u32 ttf_rd_u32(const bloom_u8 *p)
{
    return ((bloom_u32)p[0] << 24) | ((bloom_u32)p[1] << 16) | ((bloom_u32)p[2] << 8) | (bloom_u32)p[3];
}

static bloom_bool ttf_bounds_check(const bloom_ttf_font *f, bloom_u32 off, bloom_u32 len)
{
    if (off > (bloom_u32)f->size || len > (bloom_u32)f->size - off)
    {
        return BLOOM_FALSE;
    }
    return BLOOM_TRUE;
}

static bloom_u32 ttf_find_table(const bloom_u8 *data, int size, bloom_u32 font_offset, bloom_u32 tag)
{
    bloom_u16 num_tables;
    bloom_u32 i;
    bloom_u32 dir;

    if (font_offset + 12 > (bloom_u32)size)
    {
        return 0;
    }

    num_tables = ttf_rd_u16(data + font_offset + 4);
    dir = font_offset + 12;

    for (i = 0; i < num_tables; ++i)
    {
        bloom_u32 t = ttf_rd_u32(data + dir + i * 16);
        if (t == tag)
        {
            return ttf_rd_u32(data + dir + i * 16 + 8);
        }
    }
    return 0;
}

static bloom_u32 ttf_resolve_font_offset(const bloom_u8 *data, int size, bloom_u32 *out_offset)
{
    bloom_u32 tag;

    if (size < 12)
    {
        return 0;
    }

    tag = ttf_rd_u32(data);
    if (tag == TTF_TAG('t', 't', 'c', 'f'))
    {
        bloom_u32 num_fonts;
        num_fonts = ttf_rd_u32(data + 8);
        if (num_fonts == 0 || 12u + num_fonts * 4u > (bloom_u32)size)
        {
            return 0;
        }
        *out_offset = ttf_rd_u32(data + 12);
        return tag;
    }

    *out_offset = 0;
    return tag;
}

bloom_bool bloom_ttf_init(bloom_ttf_font *f, const bloom_u8 *data, int size)
{
    bloom_u32 font_offset = 0;
    bloom_u32 tag;
    bloom_u32 cmap, glyf, loca, head, maxp, hhea, hmtx;

    memset(f, 0, sizeof(*f));
    if (!data || size < 12)
    {
        return BLOOM_FALSE;
    }

    f->data = data;
    f->size = size;

    tag = ttf_resolve_font_offset(data, size, &font_offset);
    if (tag == TTF_TAG('O', 'T', 'T', 'O'))
    {
        return BLOOM_FALSE;
    }

    if (tag != TTF_TAG(0, 1, 0, 0) && tag != TTF_TAG('t', 'r', 'u', 'e') &&
        tag != TTF_TAG('t', 'y', 'p', '1') && tag != TTF_TAG('t', 't', 'c', 'f'))
    {
        return BLOOM_FALSE;
    }

    cmap = ttf_find_table(data, size, font_offset, TTF_TAG('c', 'm', 'a', 'p'));
    glyf = ttf_find_table(data, size, font_offset, TTF_TAG('g', 'l', 'y', 'f'));
    loca = ttf_find_table(data, size, font_offset, TTF_TAG('l', 'o', 'c', 'a'));
    head = ttf_find_table(data, size, font_offset, TTF_TAG('h', 'e', 'a', 'd'));
    maxp = ttf_find_table(data, size, font_offset, TTF_TAG('m', 'a', 'x', 'p'));
    hhea = ttf_find_table(data, size, font_offset, TTF_TAG('h', 'h', 'e', 'a'));
    hmtx = ttf_find_table(data, size, font_offset, TTF_TAG('h', 'm', 't', 'x'));

    if (!cmap || !glyf || !loca || !head || !maxp || !hhea || !hmtx)
    {
        return BLOOM_FALSE;
    }

    if (!ttf_bounds_check(f, head, 54u) || !ttf_bounds_check(f, maxp, 6u) || !ttf_bounds_check(f, hhea, 36u))
    {
        return BLOOM_FALSE;
    }

    f->off_cmap = cmap;
    f->off_glyf = glyf;
    f->off_loca = loca;
    f->off_head = head;
    f->off_maxp = maxp;
    f->off_hhea = hhea;
    f->off_hmtx = hmtx;
    f->units_per_em = ttf_rd_u16(data + head + 18);
    f->index_to_loc_format = ttf_rd_u16(data + head + 50);
    f->num_glyphs = ttf_rd_u16(data + maxp + 4);

    if (f->units_per_em < 16 || f->units_per_em > 16384)
    {
        return BLOOM_FALSE;
    }

    return BLOOM_TRUE;
}

static int ttf_cmap4_lookup(const bloom_u8 *table, int table_size, bloom_u32 cp)
{
    bloom_u16 segCount;
    bloom_u16 *endCount;
    bloom_u16 *startCount;
    bloom_u16 *idDelta;
    bloom_u16 *idRangeOffset;
    bloom_u32 i;

    if (table_size < 24 || cp > 0xFFFFu)
    {
        return -1;
    }

    if (ttf_rd_u16(table) != 4)
    {
        return -1;
    }

    segCount = (bloom_u16)(ttf_rd_u16(table + 6) / 2u);
    if (segCount == 0 || (bloom_u32)segCount * 8u + 16u > (bloom_u32)table_size)
    {
        return -1;
    }

    endCount = (bloom_u16 *)(table + 14);
    startCount = (bloom_u16 *)(table + 16 + (size_t)segCount * 2u);
    idDelta = (bloom_u16 *)(table + 16 + (size_t)segCount * 4u);
    idRangeOffset = (bloom_u16 *)(table + 16 + (size_t)segCount * 6u);

    for (i = 0; i < segCount; ++i)
    {
        bloom_u16 end = ttf_rd_u16((bloom_u8 *)&endCount[i]);
        if ((bloom_u32)cp > (bloom_u32)end)
        {
            continue;
        }
        {
            bloom_u16 start = ttf_rd_u16((bloom_u8 *)&startCount[i]);
            bloom_u16 ro = ttf_rd_u16((bloom_u8 *)&idRangeOffset[i]);
            bloom_i16 d = (bloom_i16)ttf_rd_u16((bloom_u8 *)&idDelta[i]);
            int g;

            if ((bloom_u32)cp < (bloom_u32)start)
            {
                return -1;
            }

            if (ro == 0)
            {
                g = ((int)cp + (int)d) & 0xFFFF;
                return g;
            }
            {
                const bloom_u8 *pbase = (const bloom_u8 *)&idRangeOffset[i];
                bloom_u32 off = (bloom_u32)(pbase - table) + (bloom_u32)ro + 2u * ((bloom_u32)cp - (bloom_u32)start);
                if (off + 2u > (bloom_u32)table_size)
                {
                    return -1;
                }
                g = (int)ttf_rd_u16(table + off);
                if (g == 0)
                {
                    return -1;
                }
                g = (g + (int)d) & 0xFFFF;
                return g;
            }
        }
    }
    return -1;
}

static int ttf_cmap12_lookup(const bloom_u8 *table, int table_size, bloom_u32 cp)
{
    bloom_u32 nGroups;
    bloom_u32 g;

    if (table_size < 16 || ttf_rd_u16(table) != 12)
    {
        return -1;
    }

    nGroups = ttf_rd_u32(table + 12);
    if (nGroups == 0 || 16u + nGroups * 12u > (bloom_u32)table_size)
    {
        return -1;
    }

    for (g = 0; g < nGroups; ++g)
    {
        bloom_u32 start = ttf_rd_u32(table + 16 + g * 12);
        bloom_u32 end = ttf_rd_u32(table + 16 + g * 12 + 4);
        bloom_u32 startGlyph = ttf_rd_u32(table + 16 + g * 12 + 8);
        if (cp >= start && cp <= end)
        {
            return (int)(startGlyph + (cp - start));
        }
    }
    return -1;
}

int bloom_ttf_cmap_lookup(const bloom_ttf_font *f, bloom_u32 cp)
{
    const bloom_u8 *data = f->data;
    bloom_u32 cmap_off = f->off_cmap;
    bloom_u16 numTables;
    bloom_u32 t;
    int best = -1;

    if (!ttf_bounds_check(f, cmap_off, 4u))
    {
        return -1;
    }

    numTables = ttf_rd_u16(data + cmap_off + 2);

    for (t = 0; t < numTables; ++t)
    {
        bloom_u32 rec = cmap_off + 4 + t * 8;
        bloom_u16 platformID;
        bloom_u16 encodingID;
        bloom_u32 offset;
        const bloom_u8 *sub;
        int sz;
        bloom_u16 fmt;
        int r;

        if (!ttf_bounds_check(f, rec, 8u))
        {
            break;
        }

        platformID = ttf_rd_u16(data + rec);
        encodingID = ttf_rd_u16(data + rec + 2);
        offset = ttf_rd_u32(data + rec + 4);

        if (!ttf_bounds_check(f, cmap_off + offset, 4u))
        {
            continue;
        }

        sub = data + cmap_off + offset;
        sz = f->size - (int)(cmap_off + offset);
        fmt = ttf_rd_u16(sub);
        r = -1;

        if (fmt == 4 && (platformID == 3 || platformID == 0))
        {
            r = ttf_cmap4_lookup(sub, sz, cp);
        }
        else if (fmt == 12 && platformID == 3)
        {
            r = ttf_cmap12_lookup(sub, sz, cp);
        }

        if (r >= 0 && platformID == 3 && encodingID == 10)
        {
            return r;
        }
        if (r >= 0 && best < 0)
        {
            best = r;
        }
    }

    if (best >= 0)
    {
        return best;
    }

    for (t = 0; t < numTables; ++t)
    {
        bloom_u32 rec = cmap_off + 4 + t * 8;
        bloom_u32 offset = ttf_rd_u32(data + rec + 4);
        const bloom_u8 *sub;
        int sz;
        bloom_u16 fmt;
        int r;

        if (!ttf_bounds_check(f, cmap_off + offset, 4u))
        {
            continue;
        }

        sub = data + cmap_off + offset;
        sz = f->size - (int)(cmap_off + offset);
        fmt = ttf_rd_u16(sub);
        if (fmt == 4)
        {
            r = ttf_cmap4_lookup(sub, sz, cp);
            if (r >= 0)
            {
                return r;
            }
        }
        else if (fmt == 12)
        {
            r = ttf_cmap12_lookup(sub, sz, cp);
            if (r >= 0)
            {
                return r;
            }
        }
    }

    return -1;
}

static bloom_u32 ttf_glyph_offset(const bloom_ttf_font *f, int gidx)
{
    const bloom_u8 *d = f->data;
    bloom_u32 loca = f->off_loca;
    bloom_u32 glyf = f->off_glyf;

    if (gidx < 0 || gidx > (int)f->num_glyphs)
    {
        return 0;
    }

    if (f->index_to_loc_format == 0)
    {
        bloom_u32 o0;
        if (!ttf_bounds_check(f, loca + (bloom_u32)gidx * 2u, 4u))
        {
            return 0;
        }
        o0 = (bloom_u32)ttf_rd_u16(d + loca + (bloom_u32)gidx * 2u) * 2u;
        return glyf + o0;
    }
    else
    {
        bloom_u32 o0;
        if (!ttf_bounds_check(f, loca + (bloom_u32)gidx * 4u, 4u))
        {
            return 0;
        }
        o0 = ttf_rd_u32(d + loca + (bloom_u32)gidx * 4u);
        return glyf + o0;
    }
}

static bloom_u32 ttf_glyph_length(const bloom_ttf_font *f, int gidx)
{
    const bloom_u8 *d = f->data;
    bloom_u32 loca = f->off_loca;

    if (gidx < 0 || gidx >= (int)f->num_glyphs)
    {
        return 0;
    }

    if (f->index_to_loc_format == 0)
    {
        bloom_u32 o0 = (bloom_u32)ttf_rd_u16(d + loca + (bloom_u32)gidx * 2u) * 2u;
        bloom_u32 o1 = (bloom_u32)ttf_rd_u16(d + loca + (bloom_u32)(gidx + 1) * 2u) * 2u;
        return o1 - o0;
    }
    else
    {
        bloom_u32 o0 = ttf_rd_u32(d + loca + (bloom_u32)gidx * 4u);
        bloom_u32 o1 = ttf_rd_u32(d + loca + (bloom_u32)(gidx + 1) * 4u);
        return o1 - o0;
    }
}

void bloom_ttf_glyph_hmetrics(const bloom_ttf_font *f, int gidx, int *adv, int *lsb)
{
    const bloom_u8 *d = f->data;
    bloom_u32 hmtx = f->off_hmtx;
    bloom_u16 nh;
    int a;
    int l;

    if (!ttf_bounds_check(f, f->off_hhea, 36u))
    {
        *adv = 0;
        *lsb = 0;
        return;
    }

    nh = ttf_rd_u16(d + f->off_hhea + 34);

    if (gidx < 0 || gidx >= (int)f->num_glyphs)
    {
        *adv = 0;
        *lsb = 0;
        return;
    }

    if ((bloom_u32)gidx < nh)
    {
        if (!ttf_bounds_check(f, hmtx + (bloom_u32)gidx * 4u, 4u))
        {
            *adv = *lsb = 0;
            return;
        }
        a = (int)ttf_rd_u16(d + hmtx + (bloom_u32)gidx * 4u);
        l = (int)ttf_rd_i16(d + hmtx + (bloom_u32)gidx * 4u + 2u);
    }
    else
    {
        bloom_u32 last = nh > 0 ? nh - 1u : 0u;
        bloom_u32 off = hmtx + last * 4u + (bloom_u32)(gidx - (int)nh) * 2u;
        if (!ttf_bounds_check(f, hmtx + last * 4u, 4u) || !ttf_bounds_check(f, off, 2u))
        {
            *adv = *lsb = 0;
            return;
        }
        a = (int)ttf_rd_u16(d + hmtx + last * 4u);
        l = (int)ttf_rd_i16(d + off);
    }

    *adv = a;
    *lsb = l;
}

void bloom_ttf_line_metrics(const bloom_ttf_font *f, int *ascent, int *descent, int *line_gap)
{
    const bloom_u8 *d = f->data;

    if (!ttf_bounds_check(f, f->off_hhea, 36u))
    {
        *ascent = *descent = *line_gap = 0;
        return;
    }

    *ascent = (int)ttf_rd_i16(d + f->off_hhea + 4);
    *descent = (int)ttf_rd_i16(d + f->off_hhea + 6);
    *line_gap = (int)ttf_rd_i16(d + f->off_hhea + 8);
}

/* --- Vertex outline (TrueType glyf) --- */

typedef enum
{
    TTF_V_MOVE,
    TTF_V_LINE,
    TTF_V_CURVE
} TtfVerb;

typedef struct
{
    TtfVerb   v;
    bloom_f32 x;
    bloom_f32 y;
    bloom_f32 cx;
    bloom_f32 cy;
} TtfVertex;

typedef struct
{
    TtfVertex *v;
    int        n;
    int        cap;
} TtfBuf;

static void ttf_buf_free(TtfBuf *b)
{
    free(b->v);
    b->v = NULL;
    b->n = b->cap = 0;
}

static bloom_bool ttf_buf_push(TtfBuf *b, TtfVerb verb, bloom_f32 x, bloom_f32 y, bloom_f32 cx, bloom_f32 cy)
{
    if (b->n >= b->cap)
    {
        int nc = b->cap ? b->cap * 2 : 64;
        TtfVertex *p = (TtfVertex *)realloc(b->v, (size_t)nc * sizeof(TtfVertex));
        if (!p)
        {
            return BLOOM_FALSE;
        }
        b->v = p;
        b->cap = nc;
    }
    b->v[b->n].v = verb;
    b->v[b->n].x = x;
    b->v[b->n].y = y;
    b->v[b->n].cx = cx;
    b->v[b->n].cy = cy;
    b->n++;
    return BLOOM_TRUE;
}

static int ttf_close_shape(TtfBuf *b, int nv, int was_off, int start_off, bloom_i32 sx, bloom_i32 sy, bloom_i32 scx,
                           bloom_i32 scy, bloom_i32 cx, bloom_i32 cy)
{
    if (start_off)
    {
        if (was_off)
        {
            if (!ttf_buf_push(b, TTF_V_CURVE, (bloom_f32)((cx + scx) >> 1), (bloom_f32)((cy + scy) >> 1),
                              (bloom_f32)cx, (bloom_f32)cy))
            {
                return nv;
            }
            nv = b->n;
        }
        if (!ttf_buf_push(b, TTF_V_CURVE, (bloom_f32)sx, (bloom_f32)sy, (bloom_f32)scx, (bloom_f32)scy))
        {
            return nv;
        }
        nv = b->n;
    }
    else
    {
        if (was_off)
        {
            if (!ttf_buf_push(b, TTF_V_CURVE, (bloom_f32)sx, (bloom_f32)sy, (bloom_f32)cx, (bloom_f32)cy))
            {
                return nv;
            }
            nv = b->n;
        }
        else
        {
            if (!ttf_buf_push(b, TTF_V_LINE, (bloom_f32)sx, (bloom_f32)sy, 0, 0))
            {
                return nv;
            }
            nv = b->n;
        }
    }
    return nv;
}

static void ttf_transform_vertex(TtfVertex *q, bloom_f32 m0, bloom_f32 m1, bloom_f32 m2, bloom_f32 m3, bloom_f32 tx,
                                 bloom_f32 ty)
{
    bloom_f32 x = q->x;
    bloom_f32 y = q->y;
    q->x = m0 * x + m2 * y + tx;
    q->y = m1 * x + m3 * y + ty;
    if (q->v == TTF_V_CURVE)
    {
        bloom_f32 cx = q->cx;
        bloom_f32 cy = q->cy;
        q->cx = m0 * cx + m2 * cy + tx;
        q->cy = m1 * cx + m3 * cy + ty;
    }
}

static bloom_bool ttf_get_shape_simple(const bloom_u8 *gdata, int glen, TtfBuf *out)
{
    bloom_i16 numberOfContours;
    const bloom_u8 *endPtsOfContours;
    bloom_u32 ins;
    const bloom_u8 *points;
    bloom_i32 n;
    bloom_i32 i;
    bloom_i32 j = 0;
    bloom_i32 m;
    bloom_i32 next_move;
    bloom_i32 was_off = 0;
    bloom_i32 off;
    bloom_i32 start_off = 0;
    bloom_i32 x;
    bloom_i32 y;
    bloom_i32 cx = 0;
    bloom_i32 cy = 0;
    bloom_i32 sx = 0;
    bloom_i32 sy = 0;
    bloom_i32 scx = 0;
    bloom_i32 scy = 0;
    bloom_u8 flagcount = 0;
    bloom_u8 flags = 0;
    bloom_u8 *vflags = NULL;

    if (glen < 10)
    {
        return BLOOM_FALSE;
    }

    numberOfContours = ttf_rd_i16(gdata);
    if (numberOfContours <= 0)
    {
        return BLOOM_TRUE;
    }

    endPtsOfContours = gdata + 10;
    ins = (bloom_i32)ttf_rd_u16(gdata + 10 + (bloom_u32)numberOfContours * 2u);
    points = gdata + 10 + (bloom_u32)numberOfContours * 2u + 2u + (bloom_u32)ins;

    n = 1 + (bloom_i32)ttf_rd_u16((bloom_u8 *)(endPtsOfContours + (numberOfContours - 1) * 2));
    m = n + 2 * (bloom_i32)numberOfContours;

    vflags = (bloom_u8 *)malloc((size_t)m * sizeof(bloom_u8) + (size_t)n * sizeof(TtfVertex));
    if (!vflags)
    {
        return BLOOM_FALSE;
    }

    {
        TtfVertex *tmp = (TtfVertex *)(vflags + m);
        off = m - n;
        for (i = 0; i < n; ++i)
        {
            if (flagcount == 0)
            {
                if (points - gdata >= glen)
                {
                    free(vflags);
                    return BLOOM_FALSE;
                }
                flags = *points++;
                if (flags & 8)
                {
                    if (points - gdata >= glen)
                    {
                        free(vflags);
                        return BLOOM_FALSE;
                    }
                    flagcount = *points++;
                }
            }
            else
            {
                --flagcount;
            }
            vflags[off + i] = flags;
            tmp[i].x = 0;
            tmp[i].y = 0;
        }
    }

    x = 0;
    for (i = 0; i < n; ++i)
    {
        flags = vflags[off + i];
        if (flags & 2)
        {
            bloom_u8 dx;
            if (points - gdata >= glen)
            {
                free(vflags);
                return BLOOM_FALSE;
            }
            dx = *points++;
            x += (flags & 16) ? (bloom_i32)dx : -(bloom_i32)dx;
        }
        else if (!(flags & 16))
        {
            if (points - gdata + 2 > glen)
            {
                free(vflags);
                return BLOOM_FALSE;
            }
            x += (bloom_i32)ttf_rd_i16(points);
            points += 2;
        }
        ((TtfVertex *)(vflags + m))[i].x = (bloom_i16)x;
    }

    y = 0;
    for (i = 0; i < n; ++i)
    {
        flags = vflags[off + i];
        if (flags & 4)
        {
            bloom_u8 dy;
            if (points - gdata >= glen)
            {
                free(vflags);
                return BLOOM_FALSE;
            }
            dy = *points++;
            y += (flags & 32) ? (bloom_i32)dy : -(bloom_i32)dy;
        }
        else if (!(flags & 32))
        {
            if (points - gdata + 2 > glen)
            {
                free(vflags);
                return BLOOM_FALSE;
            }
            y += (bloom_i32)ttf_rd_i16(points);
            points += 2;
        }
        ((TtfVertex *)(vflags + m))[i].y = (bloom_i16)y;
    }

    {
        TtfVertex *vertices = (TtfVertex *)(vflags + m);
        int num_vertices = 0;

        next_move = 0;
        was_off = 0;
        j = 0;
        sx = sy = cx = cy = scx = scy = 0;

        for (i = 0; i < n; ++i)
        {
            flags = vflags[off + i];
            x = (bloom_i32)vertices[i].x;
            y = (bloom_i32)vertices[i].y;

            if (next_move == i)
            {
                if (i != 0)
                {
                    num_vertices = ttf_close_shape(out, num_vertices, was_off, start_off, sx, sy, scx, scy, cx, cy);
                }

                start_off = !(flags & 1);
                if (start_off)
                {
                    scx = x;
                    scy = y;
                    if (!(vflags[off + i + 1] & 1))
                    {
                        sx = (x + (bloom_i32)vertices[i + 1].x) >> 1;
                        sy = (y + (bloom_i32)vertices[i + 1].y) >> 1;
                    }
                    else
                    {
                        sx = (bloom_i32)vertices[i + 1].x;
                        sy = (bloom_i32)vertices[i + 1].y;
                        ++i;
                    }
                }
                else
                {
                    sx = x;
                    sy = y;
                }
                if (!ttf_buf_push(out, TTF_V_MOVE, (bloom_f32)sx, (bloom_f32)sy, 0, 0))
                {
                    free(vflags);
                    return BLOOM_FALSE;
                }
                num_vertices = out->n;
                was_off = 0;
                next_move = 1 + (bloom_i32)ttf_rd_u16((bloom_u8 *)(endPtsOfContours + j * 2));
                ++j;
            }
            else
            {
                if (!(flags & 1))
                {
                    if (was_off)
                    {
                        if (!ttf_buf_push(out, TTF_V_CURVE, (bloom_f32)((cx + x) >> 1), (bloom_f32)((cy + y) >> 1),
                                          (bloom_f32)cx, (bloom_f32)cy))
                        {
                            free(vflags);
                            return BLOOM_FALSE;
                        }
                    }
                    cx = x;
                    cy = y;
                    was_off = 1;
                }
                else
                {
                    if (was_off)
                    {
                        if (!ttf_buf_push(out, TTF_V_CURVE, (bloom_f32)x, (bloom_f32)y, (bloom_f32)cx, (bloom_f32)cy))
                        {
                            free(vflags);
                            return BLOOM_FALSE;
                        }
                    }
                    else
                    {
                        if (!ttf_buf_push(out, TTF_V_LINE, (bloom_f32)x, (bloom_f32)y, 0, 0))
                        {
                            free(vflags);
                            return BLOOM_FALSE;
                        }
                    }
                    was_off = 0;
                }
            }
        }
        ttf_close_shape(out, out->n, was_off, start_off, sx, sy, scx, scy, cx, cy);
    }

    free(vflags);
    return BLOOM_TRUE;
}

static bloom_bool ttf_get_shape_glyph(const bloom_ttf_font *f, int gidx, TtfBuf *out, int depth);

static bloom_bool ttf_get_shape_composite(const bloom_ttf_font *f, const bloom_u8 *gdata, int glen, TtfBuf *out,
                                          int depth)
{
    const bloom_u8 *comp = gdata + 10;
    const bloom_u8 *end = gdata + glen;
    int more = 1;

    if (depth > 16)
    {
        return BLOOM_FALSE;
    }

    while (more && comp + 4 <= end)
    {
        bloom_u16 comp_flags = ttf_rd_u16(comp);
        bloom_u16 gidx = ttf_rd_u16(comp + 2);
        bloom_f32 mtx[6];
        TtfBuf sub;
        int i;

        mtx[0] = 1.0f;
        mtx[1] = 0.0f;
        mtx[2] = 0.0f;
        mtx[3] = 1.0f;
        mtx[4] = 0.0f;
        mtx[5] = 0.0f;

        comp += 4;

        if (comp_flags & 2)
        {
            if (comp_flags & 1)
            {
                if (comp + 4 > end)
                {
                    return BLOOM_FALSE;
                }
                mtx[4] = (bloom_f32)ttf_rd_i16(comp);
                mtx[5] = (bloom_f32)ttf_rd_i16(comp + 2);
                comp += 4;
            }
            else
            {
                if (comp + 2 > end)
                {
                    return BLOOM_FALSE;
                }
                mtx[4] = (bloom_f32)(bloom_i8)comp[0];
                mtx[5] = (bloom_f32)(bloom_i8)comp[1];
                comp += 2;
            }
        }
        else
        {
            /* anchor attachment - skip composite */
            return BLOOM_FALSE;
        }

        if (comp_flags & (1u << 3))
        {
            bloom_i16 sc;
            if (comp + 2 > end)
            {
                return BLOOM_FALSE;
            }
            sc = ttf_rd_i16(comp);
            comp += 2;
            mtx[0] = mtx[3] = (bloom_f32)sc / 16384.0f;
            mtx[1] = mtx[2] = 0.0f;
        }
        else if (comp_flags & (1u << 6))
        {
            bloom_i16 sx;
            bloom_i16 sy;
            if (comp + 4 > end)
            {
                return BLOOM_FALSE;
            }
            sx = ttf_rd_i16(comp);
            sy = ttf_rd_i16(comp + 2);
            comp += 4;
            mtx[0] = (bloom_f32)sx / 16384.0f;
            mtx[1] = mtx[2] = 0.0f;
            mtx[3] = (bloom_f32)sy / 16384.0f;
        }
        else if (comp_flags & (1u << 7))
        {
            bloom_i16 aa;
            bloom_i16 bb;
            bloom_i16 cc;
            bloom_i16 dd;
            if (comp + 8 > end)
            {
                return BLOOM_FALSE;
            }
            aa = ttf_rd_i16(comp);
            bb = ttf_rd_i16(comp + 2);
            cc = ttf_rd_i16(comp + 4);
            dd = ttf_rd_i16(comp + 6);
            comp += 8;
            mtx[0] = (bloom_f32)aa / 16384.0f;
            mtx[1] = (bloom_f32)bb / 16384.0f;
            mtx[2] = (bloom_f32)cc / 16384.0f;
            mtx[3] = (bloom_f32)dd / 16384.0f;
        }

        memset(&sub, 0, sizeof(sub));
        if (!ttf_get_shape_glyph(f, (int)gidx, &sub, depth + 1))
        {
            ttf_buf_free(&sub);
            return BLOOM_FALSE;
        }

        for (i = 0; i < sub.n; ++i)
        {
            ttf_transform_vertex(&sub.v[i], mtx[0], mtx[1], mtx[2], mtx[3], mtx[4], mtx[5]);
        }

        if (sub.n > 0)
        {
            TtfVertex *nv =
                (TtfVertex *)realloc(out->v, (size_t)(out->n + sub.n) * sizeof(TtfVertex));
            if (!nv)
            {
                ttf_buf_free(&sub);
                return BLOOM_FALSE;
            }
            out->v = nv;
            memcpy(out->v + out->n, sub.v, (size_t)sub.n * sizeof(TtfVertex));
            out->n += sub.n;
            out->cap = out->n > out->cap ? out->n : out->cap;
        }
        ttf_buf_free(&sub);

        more = (comp_flags & (1u << 5)) ? 1 : 0;
    }

    return BLOOM_TRUE;
}

static bloom_bool ttf_get_shape_glyph(const bloom_ttf_font *f, int gidx, TtfBuf *out, int depth)
{
    bloom_u32 goff = ttf_glyph_offset(f, gidx);
    bloom_u32 glen = ttf_glyph_length(f, gidx);
    const bloom_u8 *gd;
    bloom_i16 ncont;

    if (!ttf_bounds_check(f, goff, glen) || glen < 10)
    {
        return BLOOM_TRUE;
    }

    gd = f->data + goff;
    ncont = ttf_rd_i16(gd);

    if (ncont > 0)
    {
        return ttf_get_shape_simple(gd, (int)glen, out);
    }
    if (ncont < 0)
    {
        return ttf_get_shape_composite(f, gd, (int)glen, out, depth);
    }
    return BLOOM_TRUE;
}

typedef struct
{
    bloom_f32 x;
    bloom_f32 y;
} TtfPt;

static void ttf_tess_quad(TtfPt *pts, int *np, bloom_f32 x0, bloom_f32 y0, bloom_f32 x1, bloom_f32 y1, bloom_f32 x2,
                          bloom_f32 y2, bloom_f32 flat2, int rec)
{
    bloom_f32 mx = (x0 + 2.0f * x1 + x2) * 0.25f;
    bloom_f32 my = (y0 + 2.0f * y1 + y2) * 0.25f;
    bloom_f32 dx = (x0 + x2) * 0.5f - mx;
    bloom_f32 dy = (y0 + y2) * 0.5f - my;

    if (rec > 16)
    {
        return;
    }

    if (dx * dx + dy * dy > flat2)
    {
        ttf_tess_quad(pts, np, x0, y0, (x0 + x1) * 0.5f, (y0 + y1) * 0.5f, mx, my, flat2, rec + 1);
        ttf_tess_quad(pts, np, mx, my, (x1 + x2) * 0.5f, (y1 + y2) * 0.5f, x2, y2, flat2, rec + 1);
    }
    else
    {
        if (*np < 4096)
        {
            pts[*np].x = x2;
            pts[*np].y = y2;
            (*np)++;
        }
    }
}

typedef struct
{
    bloom_f32 x0, y0, x1, y1;
} TtfSeg;

static void ttf_seg_push(TtfSeg **segs, int *n, int *cap, bloom_f32 x0, bloom_f32 y0, bloom_f32 x1, bloom_f32 y1)
{
    TtfSeg s;
    s.x0 = x0;
    s.y0 = y0;
    s.x1 = x1;
    s.y1 = y1;
    if (*n >= *cap)
    {
        int nc = *cap ? *cap * 2 : 128;
        TtfSeg *p = (TtfSeg *)realloc(*segs, (size_t)nc * sizeof(TtfSeg));
        if (!p)
        {
            return;
        }
        *segs = p;
        *cap = nc;
    }
    (*segs)[(*n)++] = s;
}

static bloom_bool ttf_vertices_to_segments(const TtfVertex *v, int nv, TtfSeg **segs, int *nseg, int *capseg,
                                           bloom_f32 flat2)
{
    bloom_f32 x = 0;
    bloom_f32 y = 0;
    int i;

    for (i = 0; i < nv; ++i)
    {
        switch (v[i].v)
        {
        case TTF_V_MOVE:
            x = v[i].x;
            y = v[i].y;
            break;
        case TTF_V_LINE:
            ttf_seg_push(segs, nseg, capseg, x, y, v[i].x, v[i].y);
            x = v[i].x;
            y = v[i].y;
            break;
        case TTF_V_CURVE:
        {
            TtfPt stack[2048];
            int ns = 0;
            ttf_tess_quad(stack, &ns, x, y, v[i].cx, v[i].cy, v[i].x, v[i].y, flat2, 0);
            {
                int k;
                bloom_f32 px = x;
                bloom_f32 py = y;
                for (k = 0; k < ns; ++k)
                {
                    ttf_seg_push(segs, nseg, capseg, px, py, stack[k].x, stack[k].y);
                    px = stack[k].x;
                    py = stack[k].y;
                }
            }
            x = v[i].x;
            y = v[i].y;
        }
        break;
        }
    }
    return BLOOM_TRUE;
}

static void ttf_aa_raster(const TtfSeg *segs, int nseg, bloom_u8 *buf, int bw, int bh, int stride, bloom_f32 ox,
                          bloom_f32 oy)
{
    int y;
    for (y = 0; y < bh; ++y)
    {
        int x;
        for (x = 0; x < bw; ++x)
        {
            bloom_f32 c = 0;
            int sy;
            int sx;
            for (sy = 0; sy < 2; ++sy)
            {
                for (sx = 0; sx < 2; ++sx)
                {
                    bloom_f32 fx = ox + (bloom_f32)x + (sx + 0.5f) * 0.5f;
                    bloom_f32 fy = oy + (bloom_f32)y + (sy + 0.5f) * 0.5f;
                    bloom_f32 wind = 0;
                    int s;
                    for (s = 0; s < nseg; ++s)
                    {
                        bloom_f32 x0 = segs[s].x0;
                        bloom_f32 y0 = segs[s].y0;
                        bloom_f32 x1 = segs[s].x1;
                        bloom_f32 y1 = segs[s].y1;
                        bloom_f32 ymin = y0 < y1 ? y0 : y1;
                        bloom_f32 ymax = y0 > y1 ? y0 : y1;

                        if (fy < ymin || fy >= ymax)
                        {
                            continue;
                        }

                        if (fabsf(y1 - y0) < 1e-5f)
                        {
                            continue;
                        }

                        {
                            bloom_f32 t = (fy - y0) / (y1 - y0);
                            if (t < 0.0f || t > 1.0f)
                            {
                                continue;
                            }
                            bloom_f32 ix = x0 + t * (x1 - x0);
                            if (ix > fx)
                            {
                                wind += (y1 > y0) ? 1.0f : -1.0f;
                            }
                        }
                    }
                    if (wind != 0.0f)
                    {
                        c += 1.0f;
                    }
                }
            }
            buf[y * stride + x] = (bloom_u8)(c * 63.75f + 0.5f);
        }
    }
}

bloom_bool bloom_ttf_render_glyph(const bloom_ttf_font *f, int gidx, bloom_f32 scale_px, bloom_u8 *buf, int buf_w,
                                  int buf_h, int stride, int *out_w, int *out_h)
{
    TtfBuf vb;
    TtfSeg *segs = NULL;
    int nseg = 0;
    int capseg = 0;
    int asc;
    int des;
    int lg;
    bloom_f32 scale;
    bloom_f32 flat2;
    int i;
    bloom_f32 minx = 1e9f;
    bloom_f32 miny = 1e9f;
    bloom_f32 maxx = -1e9f;
    bloom_f32 maxy = -1e9f;

    *out_w = 0;
    *out_h = 0;

    if (!f || !buf || buf_w <= 0 || buf_h <= 0 || scale_px <= 0.0f || gidx < 0)
    {
        return BLOOM_FALSE;
    }

    memset(&vb, 0, sizeof(vb));
    if (!ttf_get_shape_glyph(f, gidx, &vb, 0))
    {
        ttf_buf_free(&vb);
        return BLOOM_FALSE;
    }

    bloom_ttf_line_metrics(f, &asc, &des, &lg);
    {
        bloom_f32 em_h = (bloom_f32)(asc - des);
        if (em_h < 1.0f)
        {
            em_h = (bloom_f32)f->units_per_em;
        }
        scale = em_h > 0.0f ? scale_px / em_h : scale_px / (bloom_f32)f->units_per_em;
    }

    flat2 = (0.35f / scale);
    flat2 = flat2 * flat2;

    if (!ttf_vertices_to_segments(vb.v, vb.n, &segs, &nseg, &capseg, flat2))
    {
        ttf_buf_free(&vb);
        free(segs);
        return BLOOM_FALSE;
    }
    ttf_buf_free(&vb);

    for (i = 0; i < nseg; ++i)
    {
        bloom_f32 x0 = segs[i].x0 * scale;
        bloom_f32 y0 = -segs[i].y0 * scale;
        bloom_f32 x1 = segs[i].x1 * scale;
        bloom_f32 y1 = -segs[i].y1 * scale;
        if (x0 < minx)
        {
            minx = x0;
        }
        if (x1 < minx)
        {
            minx = x1;
        }
        if (y0 < miny)
        {
            miny = y0;
        }
        if (y1 < miny)
        {
            miny = y1;
        }
        if (x0 > maxx)
        {
            maxx = x0;
        }
        if (x1 > maxx)
        {
            maxx = x1;
        }
        if (y0 > maxy)
        {
            maxy = y0;
        }
        if (y1 > maxy)
        {
            maxy = y1;
        }
        segs[i].x0 = x0;
        segs[i].y0 = y0;
        segs[i].x1 = x1;
        segs[i].y1 = y1;
    }

    if (nseg == 0)
    {
        return BLOOM_TRUE;
    }

    {
        int ix0 = (int)floorf(minx);
        int iy0 = (int)floorf(miny);
        int ix1 = (int)ceilf(maxx);
        int iy1 = (int)ceilf(maxy);
        int gw = ix1 - ix0;
        int gh = iy1 - iy0;

        if (gw <= 0)
        {
            gw = 1;
        }
        if (gh <= 0)
        {
            gh = 1;
        }

        if (gw > buf_w || gh > buf_h)
        {
            free(segs);
            return BLOOM_FALSE;
        }

        memset(buf, 0, (size_t)stride * (size_t)gh);
        ttf_aa_raster(segs, nseg, buf, gw, gh, stride, (bloom_f32)(-ix0), (bloom_f32)(-iy0));
        free(segs);
        *out_w = gw;
        *out_h = gh;
    }

    return BLOOM_TRUE;
}

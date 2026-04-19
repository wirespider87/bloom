#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "core/graphics/font/font.h"
#include "core/base/utf8.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#ifndef CLEARTYPE_NATURAL_QUALITY
#define CLEARTYPE_NATURAL_QUALITY ANTIALIASED_QUALITY
#endif
#endif

static bloom_font *g_bloom_active_font = NULL;

typedef struct bloom_font_cp_sort
{
    bloom_u32 cp;
    bloom_u16 slot;
} bloom_font_cp_sort;

static int bloom_font_cp_sort_cmp(const void *a, const void *b)
{
    bloom_u32 ac = ((const bloom_font_cp_sort *)a)->cp;
    bloom_u32 bc = ((const bloom_font_cp_sort *)b)->cp;
    if (ac < bc)
    {
        return -1;
    }
    if (ac > bc)
    {
        return 1;
    }
    return 0;
}

static bloom_bool bloom_font_build_sorted_lookup(bloom_font *font)
{
    bloom_font_cp_sort *pairs;
    bloom_u32 i;

    if (!font || font->glyph_count == 0 || !font->glyphs)
    {
        return BLOOM_FALSE;
    }

    pairs = (bloom_font_cp_sort *)malloc(sizeof(bloom_font_cp_sort) * font->glyph_count);
    if (!pairs)
    {
        return BLOOM_FALSE;
    }

    for (i = 0; i < font->glyph_count; ++i)
    {
        pairs[i].cp = font->glyphs[i].codepoint;
        pairs[i].slot = (bloom_u16)i;
    }

    qsort(pairs, (size_t)font->glyph_count, sizeof(bloom_font_cp_sort), bloom_font_cp_sort_cmp);

    font->glyph_sorted_cp = (bloom_u32 *)malloc(sizeof(bloom_u32) * font->glyph_count);
    font->glyph_sorted_slot = (bloom_u16 *)malloc(sizeof(bloom_u16) * font->glyph_count);
    if (!font->glyph_sorted_cp || !font->glyph_sorted_slot)
    {
        free(pairs);
        free(font->glyph_sorted_cp);
        free(font->glyph_sorted_slot);
        font->glyph_sorted_cp = NULL;
        font->glyph_sorted_slot = NULL;
        return BLOOM_FALSE;
    }

    for (i = 0; i < font->glyph_count; ++i)
    {
        font->glyph_sorted_cp[i] = pairs[i].cp;
        font->glyph_sorted_slot[i] = pairs[i].slot;
    }

    free(pairs);
    return BLOOM_TRUE;
}

bloom_i32 bloom_font_glyph_slot(bloom_font *font, bloom_u32 codepoint)
{
    bloom_u32 lo = 0;
    bloom_u32 hi;

    if (!font || !font->glyph_sorted_cp || font->glyph_count == 0)
    {
        return -1;
    }

    hi = font->glyph_count;
    while (lo < hi)
    {
        bloom_u32 mid = (lo + hi) >> 1;
        bloom_u32 k = font->glyph_sorted_cp[mid];
        if (k < codepoint)
        {
            lo = mid + 1;
        }
        else if (k > codepoint)
        {
            hi = mid;
        }
        else
        {
            return (bloom_i32)font->glyph_sorted_slot[mid];
        }
    }
    return -1;
}

void bloom_font_set_active(bloom_font *font)
{
    g_bloom_active_font = font;
}

bloom_font *bloom_font_get_active(void)
{
    return g_bloom_active_font;
}

/* extract font family name from raw TTF/OTF data (platform 3 UTF-16BE or platform 1 ASCII) */
static bloom_bool bloom_font_extract_ttf_name(const bloom_u8 *data, bloom_u64 data_size,
                                               char *out, int out_size)
{
    bloom_u16 num_tables, i, count, string_offset;
    bloom_u32 name_table_offset = 0;
    bloom_u32 offset;

    if (data_size < 12 || out_size < 2)
        return BLOOM_FALSE;

    num_tables = (bloom_u16)((data[4] << 8) | data[5]);
    offset = 12;

    for (i = 0; i < num_tables && offset + 16 <= data_size; i++, offset += 16)
    {
        if (data[offset] == 'n' && data[offset + 1] == 'a' &&
            data[offset + 2] == 'm' && data[offset + 3] == 'e')
        {
            name_table_offset = (bloom_u32)((data[offset + 8] << 24) | (data[offset + 9] << 16) |
                                             (data[offset + 10] << 8) | data[offset + 11]);
            break;
        }
    }

    if (!name_table_offset || name_table_offset + 6 > data_size)
        return BLOOM_FALSE;

    count = (bloom_u16)((data[name_table_offset + 2] << 8) | data[name_table_offset + 3]);
    string_offset = (bloom_u16)((data[name_table_offset + 4] << 8) | data[name_table_offset + 5]);

    for (i = 0; i < count; i++)
    {
        bloom_u32 rec = name_table_offset + 6 + (bloom_u32)i * 12;
        bloom_u16 platformID, nameID, length, str_off;
        bloom_u32 str_start;

        if (rec + 12 > data_size)
            break;

        platformID = (bloom_u16)((data[rec] << 8) | data[rec + 1]);
        nameID     = (bloom_u16)((data[rec + 6] << 8) | data[rec + 7]);
        length     = (bloom_u16)((data[rec + 8] << 8) | data[rec + 9]);
        str_off    = (bloom_u16)((data[rec + 10] << 8) | data[rec + 11]);

        if (nameID != 1) /* font family name */
            continue;

        str_start = name_table_offset + string_offset + str_off;
        if (str_start + length > data_size)
            continue;

        if (platformID == 3 || platformID == 0) /* Windows / Unicode: UTF-16BE */
        {
            int j = 0, k;
            for (k = 0; k < length && j < out_size - 1; k += 2)
            {
                bloom_u8 hi = data[str_start + k];
                bloom_u8 lo = data[str_start + k + 1];
                if (hi == 0 && lo >= 32 && lo < 127)
                    out[j++] = (char)lo;
            }
            out[j] = '\0';
            if (j > 0)
                return BLOOM_TRUE;
        }
        else if (platformID == 1) /* Macintosh: ASCII */
        {
            int copy_len = length < out_size - 1 ? length : out_size - 1;
            memcpy(out, &data[str_start], copy_len);
            out[copy_len] = '\0';
            return BLOOM_TRUE;
        }
    }

    return BLOOM_FALSE;
}

static int bloom_font_align_up_int(int value, int alignment)
{
    int remainder;

    if (alignment <= 1)
    {
        return value;
    }

    remainder = value % alignment;
    if (remainder == 0)
    {
        return value;
    }

    return value + (alignment - remainder);
}

static int bloom_font_align_down_int(int value, int alignment)
{
    int remainder;

    if (alignment <= 1)
    {
        return value;
    }

    remainder = value % alignment;
    if (remainder == 0)
    {
        return value;
    }
    if (remainder < 0)
    {
        remainder += alignment;
    }

    return value - remainder;
}

#define BLOOM_FONT_ATLAS_COLUMNS 64
#define BLOOM_FONT_ATLAS_ROWS    64

static int bloom_font_fill_default_codepoints(bloom_u32 *out, int max_out)
{
    int n = 0;
    int i;

    /* 1. ASCII printable: U+0020-U+007E */
    for (i = 0x0020; i <= 0x007E && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 2. Latin-1 Supplement: U+00A0-U+00FF */
    for (i = 0x00A0; i <= 0x00FF && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 3. Latin Extended-A: U+0100-U+017F */
    for (i = 0x0100; i <= 0x017F && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 4. Latin Extended-B: U+0180-U+024F */
    for (i = 0x0180; i <= 0x024F && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 5. IPA Extensions: U+0250-U+02AF */
    for (i = 0x0250; i <= 0x02AF && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 6. Greek and Coptic: U+0370-U+03FF */
    for (i = 0x0370; i <= 0x03FF && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 7. Cyrillic: U+0400-U+04FF */
    for (i = 0x0400; i <= 0x04FF && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 8. Cyrillic Supplement: U+0500-U+052F */
    for (i = 0x0500; i <= 0x052F && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 9. Devanagari: U+0900-U+097F */
    for (i = 0x0900; i <= 0x097F && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 10. Thai: U+0E00-U+0E7F */
    for (i = 0x0E00; i <= 0x0E7F && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 11. Hebrew letters: U+05D0-U+05EA */
    for (i = 0x05D0; i <= 0x05EA && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 12. Arabic: U+0600-U+06FF */
    for (i = 0x0600; i <= 0x06FF && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 13. General Punctuation: U+2000-U+206F */
    for (i = 0x2000; i <= 0x206F && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 14. Superscripts and Subscripts: U+2070-U+209F */
    for (i = 0x2070; i <= 0x209F && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 15. Currency Symbols: U+20A0-U+20CF */
    for (i = 0x20A0; i <= 0x20CF && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 16. Letterlike Symbols: U+2100-U+214F */
    for (i = 0x2100; i <= 0x214F && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 17. Number Forms: U+2150-U+218F */
    for (i = 0x2150; i <= 0x218F && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 18. Arrows: U+2190-U+21FF */
    for (i = 0x2190; i <= 0x21FF && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 19. Mathematical Operators: U+2200-U+22FF */
    for (i = 0x2200; i <= 0x22FF && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 20. Miscellaneous Technical: U+2300-U+23FF */
    for (i = 0x2300; i <= 0x23FF && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 21. Box Drawing: U+2500-U+257F */
    for (i = 0x2500; i <= 0x257F && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 22. Block Elements: U+2580-U+259F */
    for (i = 0x2580; i <= 0x259F && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 23. Geometric Shapes: U+25A0-U+25FF */
    for (i = 0x25A0; i <= 0x25FF && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 24. Miscellaneous Symbols: U+2600-U+26FF */
    for (i = 0x2600; i <= 0x26FF && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 25. Dingbats: U+2700-U+27BF */
    for (i = 0x2700; i <= 0x27BF && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 26. CJK common subset: U+4E00-U+4EFF */
    for (i = 0x4E00; i <= 0x4EFF && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 27. CJK continuation: U+4F00-U+4FFF */
    for (i = 0x4F00; i <= 0x4FFF && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 28. More CJK: U+5000-U+51FF */
    for (i = 0x5000; i <= 0x51FF && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 29. More CJK: U+6000-U+62FF */
    for (i = 0x6000; i <= 0x62FF && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 30. More CJK: U+7000-U+72FF */
    for (i = 0x7000; i <= 0x72FF && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 31. More CJK: U+8000-U+82FF */
    for (i = 0x8000; i <= 0x82FF && n < max_out; ++i) out[n++] = (bloom_u32)i;
    /* 32. More CJK: U+9000-U+9FFF */
    for (i = 0x9000; i <= 0x9FFF && n < max_out; ++i) out[n++] = (bloom_u32)i;

    return n;
}

static int bloom_font_fill_codepoints_from_ranges(bloom_u32 *out, int max_out, const bloom_u32 *ranges, int range_count)
{
    int n = 0;
    int i;
    int r;

    if (!ranges || range_count <= 0)
    {
        return 0;
    }

    for (r = 0; r < range_count && n < max_out; ++r)
    {
        bloom_u32 start = ranges[r * 2];
        bloom_u32 count = ranges[r * 2 + 1];
        for (i = 0; (bloom_u32)i < count && n < max_out; ++i)
        {
            out[n++] = start + (bloom_u32)i;
        }
    }

    return n;
}

#ifdef _WIN32

static bloom_f32 bloom_font_source_coverage(bloom_u32 pixel)
{
    bloom_u8 blue = (bloom_u8)(pixel & 0xFF);
    bloom_u8 green = (bloom_u8)((pixel >> 8) & 0xFF);
    bloom_u8 red = (bloom_u8)((pixel >> 16) & 0xFF);

    return (bloom_f32)((int)red + (int)green + (int)blue) / (255.0f * 3.0f);
}

static bloom_u8 bloom_font_downsample_coverage(const bloom_u32 *src_pixels,
                                               int src_width, int src_height,
                                               int src_x, int src_y, int factor)
{
    bloom_f32 coverage = 0.0f;
    int sample_count = 0;
    int y;

    for (y = 0; y < factor; ++y)
    {
        int py = src_y + y;
        int x;

        if (py >= src_height)
        {
            break;
        }

        for (x = 0; x < factor; ++x)
        {
            int px = src_x + x;

            if (px >= src_width)
            {
                break;
            }

            coverage += bloom_font_source_coverage(src_pixels[py * src_width + px]);
            sample_count++;
        }
    }

    if (sample_count <= 0)
    {
        return 0;
    }

    coverage /= (bloom_f32)sample_count;
    coverage = powf(coverage, 0.85f);

    return (bloom_u8)(coverage * 255.0f + 0.5f);
}

static bloom_bool bloom_font_build_gdi_ex(bloom_font *font, bloom_f32 size, const char *face_name,
                                          const bloom_u32 *custom_ranges, bloom_i32 range_count)
{
    const int oversample = 4;
    const int columns = BLOOM_FONT_ATLAS_COLUMNS;
    const int rows = BLOOM_FONT_ATLAS_ROWS;
    bloom_u32 codepoints[BLOOM_FONT_ATLAS_COLUMNS * BLOOM_FONT_ATLAS_ROWS];
    int glyph_count;
    const int raster_size = (int)ceilf(size * (bloom_f32)oversample);
    const int src_cell_w = bloom_font_align_up_int(raster_size * 2, oversample);
    const int src_cell_h = bloom_font_align_up_int(raster_size * 2, oversample);
    const int dst_cell_w = src_cell_w / oversample;
    const int dst_cell_h = src_cell_h / oversample;
    const int dst_atlas_w = columns * dst_cell_w;
    const int dst_atlas_h = rows * dst_cell_h;
    const int glyph_padding = oversample * 2;
    const bloom_f32 inv_os = 1.0f / (bloom_f32)oversample;
    int i;
    int x;
    int y;

    HDC screen_dc = GetDC(NULL);
    HDC mem_dc = CreateCompatibleDC(screen_dc);
    BITMAPINFO bmi;
    void *dib_bits = NULL;
    HBITMAP bmp = NULL;
    HFONT font_handle = NULL;
    HFONT old_font = NULL;
    TEXTMETRICA tm;
    const bloom_u32 *src_pixels = NULL;
    bloom_bool ok = BLOOM_FALSE;

    if (!font || !face_name || !face_name[0] || size <= 0.0f)
    {
        if (mem_dc) DeleteDC(mem_dc);
        if (screen_dc) ReleaseDC(NULL, screen_dc);
        return BLOOM_FALSE;
    }

    if (!screen_dc || !mem_dc)
    {
        if (mem_dc) DeleteDC(mem_dc);
        if (screen_dc) ReleaseDC(NULL, screen_dc);
        return BLOOM_FALSE;
    }

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = src_cell_w;
    bmi.bmiHeader.biHeight = -src_cell_h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    bmp = CreateDIBSection(mem_dc, &bmi, DIB_RGB_COLORS, &dib_bits, NULL, 0);
    if (!bmp || !dib_bits)
    {
        goto cleanup;
    }

    SelectObject(mem_dc, bmp);
    SetBkMode(mem_dc, TRANSPARENT);
    SetBkColor(mem_dc, RGB(0, 0, 0));
    SetTextColor(mem_dc, RGB(255, 255, 255));
    SetMapMode(mem_dc, MM_TEXT);

    font_handle = CreateFontA(
        -raster_size,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_OUTLINE_PRECIS,
        CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY,
        VARIABLE_PITCH,
        face_name);
    if (!font_handle)
    {
        goto cleanup;
    }

    old_font = (HFONT)SelectObject(mem_dc, font_handle);

    if (!GetTextMetricsA(mem_dc, &tm))
    {
        goto cleanup;
    }

    if (custom_ranges && range_count > 0)
    {
        glyph_count = bloom_font_fill_codepoints_from_ranges(codepoints, columns * rows, custom_ranges, range_count);
    }
    else
    {
        glyph_count = bloom_font_fill_default_codepoints(codepoints, columns * rows);
    }

    if (glyph_count <= 0 || glyph_count > columns * rows)
    {
        goto cleanup;
    }

    font->atlas_width = (bloom_u32)dst_atlas_w;
    font->atlas_height = (bloom_u32)dst_atlas_h;
    font->atlas_pixels = (bloom_u8 *)malloc((size_t)dst_atlas_w * (size_t)dst_atlas_h);
    font->glyph_capacity = (bloom_u32)glyph_count;
    font->glyphs = (bloom_glyph *)malloc(sizeof(bloom_glyph) * (size_t)glyph_count);
    if (!font->atlas_pixels || !font->glyphs)
    {
        goto cleanup;
    }
    memset(font->atlas_pixels, 0, (size_t)dst_atlas_w * (size_t)dst_atlas_h);

    src_pixels = (const bloom_u32 *)dib_bits;

    for (i = 0; i < glyph_count; ++i)
    {
        bloom_u32 cp = codepoints[i];
        WCHAR wch = (WCHAR)((cp <= 0xFFFFu) ? cp : 0xFFFDu);
        SIZE extent;
        int col = i % columns;
        int row = i / columns;
        int dst_cell_x = col * dst_cell_w;
        int dst_cell_y = row * dst_cell_h;
        int draw_x;
        int draw_y;
        int draw_y_offset;
        int dst_x0;
        int dst_y0;
        int dst_x1;
        int dst_y1;
        bloom_glyph *g;

/* Pre render clear */ 
PatBlt(mem_dc, 0, 0, src_cell_w, src_cell_h, BLACKNESS);
        GetTextExtentPoint32W(mem_dc, &wch, 1, &extent);

        draw_y_offset = bloom_font_align_down_int((src_cell_h - tm.tmHeight) / 2, oversample);
        if (draw_y_offset < 0)
        {
            draw_y_offset = 0;
        }

        draw_x = glyph_padding;
        draw_y = draw_y_offset;
        TextOutW(mem_dc, draw_x, draw_y, &wch, 1);
        GdiFlush();


        dst_x0 = dst_cell_x + (draw_x / oversample);
        dst_y0 = dst_cell_y + (draw_y / oversample);
        dst_x1 = dst_x0 + (bloom_font_align_up_int(extent.cx, oversample) / oversample);
        dst_y1 = dst_y0 + (bloom_font_align_up_int(tm.tmHeight, oversample) / oversample);

        g = &font->glyphs[i];
        g->codepoint = cp;
        g->advance = (bloom_f32)extent.cx * inv_os;
        if (g->advance < size * 0.33f)
        {
            g->advance = size * 0.33f;
        }
        g->x0 = 0.0f;
        g->y0 = 0.0f;
        g->x1 = (bloom_f32)(dst_x1 - dst_x0);
        g->y1 = (bloom_f32)(dst_y1 - dst_y0);
        g->u0 = (bloom_f32)dst_x0 / (bloom_f32)dst_atlas_w;
        g->v0 = (bloom_f32)dst_y0 / (bloom_f32)dst_atlas_h;
        g->u1 = (bloom_f32)dst_x1 / (bloom_f32)dst_atlas_w;
        g->v1 = (bloom_f32)dst_y1 / (bloom_f32)dst_atlas_h;

       /* downsample */
        for (y = 0; y < dst_cell_h; ++y)
        {
            for (x = 0; x < dst_cell_w; ++x)
            {
                int atlas_x = dst_cell_x + x;
                int atlas_y = dst_cell_y + y;
                font->atlas_pixels[atlas_y * dst_atlas_w + atlas_x] =
                    bloom_font_downsample_coverage(src_pixels,
                                                   src_cell_w, src_cell_h,
                                                   x * oversample, y * oversample,
                                                   oversample);
            }
        }
    }

    font->glyph_count = (bloom_u32)glyph_count;
    font->size = size;
    font->ascent = (bloom_f32)tm.tmAscent * inv_os;
    font->descent = (bloom_f32)tm.tmDescent * inv_os;
    font->line_height = (bloom_f32)(tm.tmHeight + tm.tmExternalLeading) * inv_os;

    if (!bloom_font_build_sorted_lookup(font))
    {
        ok = BLOOM_FALSE;
        goto cleanup;
    }

    font->valid = BLOOM_TRUE;
    ok = BLOOM_TRUE;

cleanup:
    if (!ok)
    {
        free(font->glyph_sorted_cp);
        free(font->glyph_sorted_slot);
        font->glyph_sorted_cp = NULL;
        font->glyph_sorted_slot = NULL;
        free(font->atlas_pixels);
        free(font->glyphs);
        font->atlas_pixels = NULL;
        font->glyphs = NULL;
        font->atlas_width = 0;
        font->atlas_height = 0;
        font->glyph_count = 0;
        font->glyph_capacity = 0;
        font->size = 0.0f;
        font->ascent = 0.0f;
        font->descent = 0.0f;
        font->line_height = 0.0f;
        font->valid = BLOOM_FALSE;
    }
    if (old_font)
    {
        SelectObject(mem_dc, old_font);
    }
    if (font_handle)
    {
        DeleteObject(font_handle);
    }
    if (bmp)
    {
        DeleteObject(bmp);
    }
    DeleteDC(mem_dc);
    ReleaseDC(NULL, screen_dc);
    return ok;
}

static bloom_bool bloom_font_build_gdi(bloom_font *font, bloom_f32 size)
{
    static const char *const candidates[] = {
        "Segoe UI",
        "Microsoft YaHei UI",
        "Arial",
        "Tahoma"
    };
    bloom_u32 i;

    for (i = 0; i < (bloom_u32)(sizeof(candidates) / sizeof(candidates[0])); ++i)
    {
        if (bloom_font_build_gdi_ex(font, size, candidates[i], NULL, 0))
        {
            return BLOOM_TRUE;
        }
    }

    return BLOOM_FALSE;
}

#endif

#ifndef _WIN32

#include "core/graphics/font/bloom_ttf.h"

static void bloom_font_ttf_blit(bloom_u8 *atlas, int atlas_w, int atlas_h,
                              int dst_x, int dst_y, const bloom_u8 *bmp, int bw, int bh)
{
    int x;
    int y;

    for (y = 0; y < bh; ++y)
    {
        for (x = 0; x < bw; ++x)
        {
            int ax = dst_x + x;
            int ay = dst_y + y;
            bloom_u8 v = bmp[y * bw + x];
            bloom_f32 a;

            if (ax < 0 || ay < 0 || ax >= atlas_w || ay >= atlas_h)
            {
                continue;
            }

            a = (bloom_f32)v / 255.0f;
            a = powf(a, 0.85f);
            v = (bloom_u8)(a * 255.0f + 0.5f);
            atlas[ay * atlas_w + ax] = v;
        }
    }
}

static bloom_bool bloom_font_build_ttf_from_buffer(bloom_font *font, const bloom_u8 *data, int data_len,
                                                    bloom_f32 size, const bloom_u32 *custom_ranges, bloom_i32 range_count)

{
    bloom_ttf_font ttf;
    const int oversample = 4;
    const int columns = BLOOM_FONT_ATLAS_COLUMNS;
    const int rows = BLOOM_FONT_ATLAS_ROWS;
    bloom_u32 codepoints[BLOOM_FONT_ATLAS_COLUMNS * BLOOM_FONT_ATLAS_ROWS];
    int glyph_count;
    const int raster_size = (int)ceilf(size * (bloom_f32)oversample);
    const int src_cell_w = bloom_font_align_up_int(raster_size * 2, oversample);
    const int src_cell_h = bloom_font_align_up_int(raster_size * 2, oversample);
    const int dst_cell_w = src_cell_w / oversample;
    const int dst_cell_h = src_cell_h / oversample;
    const int dst_atlas_w = columns * dst_cell_w;
    const int dst_atlas_h = rows * dst_cell_h;
    const int glyph_padding = oversample * 2;
    int ascent_i;
    int descent_i;
    int line_gap_i;
    bloom_f32 em_scale;
    int i;
    bloom_u8 *bmp = NULL;
    bloom_bool ok = BLOOM_FALSE;

    if (!font || !data || data_len <= 0 || size <= 0.0f)
    {
        return BLOOM_FALSE;
    }

    if (!bloom_ttf_init(&ttf, data, data_len))
    {
        return BLOOM_FALSE;
    }

    if (custom_ranges && range_count > 0)
    {
        glyph_count = bloom_font_fill_codepoints_from_ranges(codepoints, columns * rows, custom_ranges, range_count);
    }
    else
    {
        glyph_count = bloom_font_fill_default_codepoints(codepoints, columns * rows);
    }

    if (glyph_count <= 0 || glyph_count > columns * rows)
    {
        return BLOOM_FALSE;
    }

    bloom_ttf_line_metrics(&ttf, &ascent_i, &descent_i, &line_gap_i);
    {
        bloom_f32 em_h = (bloom_f32)(ascent_i - descent_i);
        if (em_h < 1.0f)
        {
            em_h = (bloom_f32)ttf.units_per_em;
        }
        em_scale = em_h > 0.0f ? (bloom_f32)dst_cell_h / em_h : (bloom_f32)dst_cell_h / (bloom_f32)ttf.units_per_em;
    }

    font->size = size;
    font->ascent = (bloom_f32)ascent_i * em_scale;
    font->descent = (bloom_f32)(-descent_i) * em_scale;
    font->line_height = (bloom_f32)(ascent_i - descent_i + line_gap_i) * em_scale;

    font->atlas_width = (bloom_u32)dst_atlas_w;
    font->atlas_height = (bloom_u32)dst_atlas_h;
    font->atlas_pixels = (bloom_u8 *)calloc((size_t)dst_atlas_w * (size_t)dst_atlas_h, 1);
    font->glyph_capacity = (bloom_u32)glyph_count;
    font->glyphs = (bloom_glyph *)malloc(sizeof(bloom_glyph) * (size_t)glyph_count);
    bmp = (bloom_u8 *)malloc((size_t)dst_cell_w * (size_t)dst_cell_h);
    if (!font->atlas_pixels || !font->glyphs || !bmp)
    {
        goto cleanup;
    }

    for (i = 0; i < glyph_count; ++i)
    {
        bloom_u32 cp = codepoints[i];
        int advance;
        int lsb;
        int gidx;
        int bw;
        int bh;
        int col = i % columns;
        int row = i / columns;
        int dst_cell_x = col * dst_cell_w;
        int dst_cell_y = row * dst_cell_h;
        int dst_x0;
        int dst_y0;
        bloom_glyph *gl = &font->glyphs[i];

        gidx = bloom_ttf_cmap_lookup(&ttf, cp);
        gl->codepoint = cp;

        if (gidx < 0)
        {
            gl->advance = size * 0.33f;
            gl->x0 = 0.0f;
            gl->y0 = 0.0f;
            gl->x1 = 0.0f;
            gl->y1 = 0.0f;
            gl->u0 = gl->v0 = gl->u1 = gl->v1 = 0.0f;
            continue;
        }

        bloom_ttf_glyph_hmetrics(&ttf, gidx, &advance, &lsb);
        gl->advance = (bloom_f32)advance * em_scale;
        if (gl->advance < size * 0.33f)
        {
            gl->advance = size * 0.33f;
        }

        if (!bloom_ttf_render_glyph(&ttf, gidx, (bloom_f32)dst_cell_h, bmp, dst_cell_w, dst_cell_h, dst_cell_w, &bw,
                                    &bh))
        {
            gl->x0 = gl->y0 = gl->x1 = gl->y1 = 0.0f;
            gl->u0 = gl->v0 = gl->u1 = gl->v1 = 0.0f;
            continue;
        }

        if (bw <= 0 || bh <= 0)
        {
            gl->x0 = 0.0f;
            gl->y0 = 0.0f;
            gl->x1 = 0.0f;
            gl->y1 = 0.0f;
            gl->u0 = gl->v0 = gl->u1 = gl->v1 = 0.0f;
            continue;
        }

        dst_x0 = dst_cell_x + (glyph_padding / oversample);
        dst_y0 = dst_cell_y + (dst_cell_h - bh) / 2;
        if (dst_x0 + bw > dst_cell_x + dst_cell_w)
        {
            dst_x0 = dst_cell_x + 1;
        }

        bloom_font_ttf_blit(font->atlas_pixels, dst_atlas_w, dst_atlas_h, dst_x0, dst_y0, bmp, bw, bh);

        gl->x0 = 0.0f;
        gl->y0 = 0.0f;
        gl->x1 = (bloom_f32)bw;
        gl->y1 = (bloom_f32)bh;
        gl->u0 = (bloom_f32)dst_x0 / (bloom_f32)dst_atlas_w;
        gl->v0 = (bloom_f32)dst_y0 / (bloom_f32)dst_atlas_h;
        gl->u1 = (bloom_f32)(dst_x0 + bw) / (bloom_f32)dst_atlas_w;
        gl->v1 = (bloom_f32)(dst_y0 + bh) / (bloom_f32)dst_atlas_h;
    }

    font->glyph_count = (bloom_u32)glyph_count;
    if (!bloom_font_build_sorted_lookup(font))
    {
        goto cleanup;
    }

    font->valid = BLOOM_TRUE;
    ok = BLOOM_TRUE;

cleanup:
    if (!ok)
    {
        free(font->glyph_sorted_cp);
        free(font->glyph_sorted_slot);
        font->glyph_sorted_cp = NULL;
        font->glyph_sorted_slot = NULL;
        free(font->atlas_pixels);
        free(font->glyphs);
        font->atlas_pixels = NULL;
        font->glyphs = NULL;
        font->atlas_width = 0;
        font->atlas_height = 0;
        font->glyph_count = 0;
        font->glyph_capacity = 0;
        font->size = 0.0f;
        font->ascent = 0.0f;
        font->descent = 0.0f;
        font->line_height = 0.0f;
        font->valid = BLOOM_FALSE;
    }

    free(bmp);
    return ok;
}

static bloom_bool bloom_font_try_default_unix_font(bloom_font *font, bloom_f32 size)
{
    static const char *const paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        NULL
    };
    int pi;

    for (pi = 0; paths[pi] != NULL; ++pi)
    {
        FILE *fp = fopen(paths[pi], "rb");
        long sz;
        bloom_u8 *buf;
        bloom_bool built;

        if (!fp)
        {
            continue;
        }

        if (fseek(fp, 0, SEEK_END) != 0)
        {
            fclose(fp);
            continue;
        }

        sz = ftell(fp);
        if (sz <= 0 || sz > (48 * 1024 * 1024))
        {
            fclose(fp);
            continue;
        }

        if (fseek(fp, 0, SEEK_SET) != 0)
        {
            fclose(fp);
            continue;
        }

        buf = (bloom_u8 *)malloc((size_t)sz);
        if (!buf)
        {
            fclose(fp);
            continue;
        }

        if (fread(buf, 1, (size_t)sz, fp) != (size_t)sz)
        {
            free(buf);
            fclose(fp);
            continue;
        }

        fclose(fp);
        built = bloom_font_build_ttf_from_buffer(font, buf, (int)sz, size, NULL, 0);

        free(buf);
        if (built)
        {
            return BLOOM_TRUE;
        }
    }

    return BLOOM_FALSE;
}

#endif /* !_WIN32 */

void bloom_font_init(bloom_font *font)
{
    memset(font, 0, sizeof(bloom_font));
}

bloom_bool bloom_font_build_default(bloom_font *font, bloom_f32 size)
{
#ifdef _WIN32
    return bloom_font_build_gdi(font, size);
#else
    return bloom_font_try_default_unix_font(font, size);
#endif
}

bloom_bool bloom_font_load_from_memory(bloom_font *font, const bloom_u8 *data, bloom_u64 size, bloom_f32 pixel_size)
{
    return bloom_font_load_from_memory_ex(font, data, size, pixel_size, NULL, 0);
}

bloom_bool bloom_font_load_from_memory_ex(bloom_font *font, const bloom_u8 *data, bloom_u64 size, bloom_f32 pixel_size, const bloom_u32 *ranges, bloom_i32 range_count)
{
    if (!font || !data || size == 0 || pixel_size <= 0.0f)
    {
        return BLOOM_FALSE;
    }

#ifdef _WIN32
    HANDLE font_resource = NULL;
    DWORD fonts_added = 0;
    char face_name[128];
    bloom_bool built = BLOOM_FALSE;

    if (size > 0xFFFFFFFFull)
    {
        return BLOOM_FALSE;
    }

    face_name[0] = '\0';
    font_resource = AddFontMemResourceEx((PVOID)data, (DWORD)size, NULL, &fonts_added);
    if (!font_resource || fonts_added == 0)
    {
        return BLOOM_FALSE;
    }

    if (bloom_font_extract_ttf_name(data, size, face_name, (int)sizeof(face_name)) && face_name[0] != '\0')
    {
        built = bloom_font_build_gdi_ex(font, pixel_size, face_name, ranges, range_count);
    }

    RemoveFontMemResourceEx(font_resource);
    return built;
#else
    if (size > (bloom_u64)INT_MAX)
    {
        return BLOOM_FALSE;
    }
    return bloom_font_build_ttf_from_buffer(font, data, (int)size, pixel_size, ranges, range_count);
#endif
}

bloom_bool bloom_font_load_from_file(bloom_font *font, const char *path, bloom_f32 pixel_size)
{
    return bloom_font_load_from_file_ex(font, path, pixel_size, NULL, 0);
}

bloom_bool bloom_font_load_from_file_ex(bloom_font *font, const char *path, bloom_f32 pixel_size, const bloom_u32 *ranges, bloom_i32 range_count)
{
    FILE *fp;
    long sz;
    bloom_u8 *buf;
    bloom_bool result;

    if (!font || !path || pixel_size <= 0.0f)
    {
        return BLOOM_FALSE;
    }

    fp = fopen(path, "rb");
    if (!fp)
    {
        return BLOOM_FALSE;
    }

    fseek(fp, 0, SEEK_END);
    sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (sz <= 0)
    {
        fclose(fp);
        return BLOOM_FALSE;
    }

    buf = (bloom_u8 *)malloc((size_t)sz);
    if (!buf)
    {
        fclose(fp);
        return BLOOM_FALSE;
    }

    if (fread(buf, 1, (size_t)sz, fp) != (size_t)sz)
    {
        free(buf);
        fclose(fp);
        return BLOOM_FALSE;
    }

    fclose(fp);
    result = bloom_font_load_from_memory_ex(font, buf, (bloom_u64)sz, pixel_size, ranges, range_count);
    free(buf);

    return result;
}

bloom_bool bloom_font_load_from_system(bloom_font *font, const char *name, bloom_f32 pixel_size)
{
    if (!font || !name || pixel_size <= 0.0f)
    {
        return BLOOM_FALSE;
    }

#ifdef _WIN32
    return bloom_font_build_gdi_ex(font, pixel_size, name, NULL, 0);
#else
    /* TODO: handle non windows */
    return BLOOM_FALSE;

#endif
}

void bloom_font_destroy(bloom_font *font)
{
    if (font->glyph_sorted_cp)
    {
        free(font->glyph_sorted_cp);
        font->glyph_sorted_cp = NULL;
    }
    if (font->glyph_sorted_slot)
    {
        free(font->glyph_sorted_slot);
        font->glyph_sorted_slot = NULL;
    }
    if (font->atlas_pixels)
    {
        free(font->atlas_pixels);
        font->atlas_pixels = NULL;
    }
    if (font->glyphs)
    {
        free(font->glyphs);
        font->glyphs = NULL;
    }
    if (g_bloom_active_font == font)
    {
        g_bloom_active_font = NULL;
    }
    font->valid = BLOOM_FALSE;
}

bloom_f32 bloom_font_text_width(bloom_font *font, const char *text)
{
    bloom_f32 width = 0;
    bloom_f32 line_width = 0;
    const char *p;
    const char *end;

    if (!text || !font)
    {
        return 0;
    }

    end = text + strlen(text);
    for (p = text; p < end;)
    {
        bloom_u32 bl;
        bloom_u32 cp = bloom_utf8_decode_one(p, (bloom_u32)(end - p), &bl);

        if (bl == 0)
        {
            bl = 1;
        }
        p += bl;

        if (cp == '\n')
        {
            if (line_width > width)
            {
                width = line_width;
            }
            line_width = 0;
        }
        else if (cp >= 32u)
        {
            line_width += bloom_font_char_width(font, cp);
        }
    }
    if (line_width > width)
    {
        width = line_width;
    }
    return width;
}

bloom_f32 bloom_font_char_width(bloom_font *font, bloom_u32 codepoint)
{
    bloom_i32 slot;

    if (!font)
    {
        return 0.0f;
    }

    if (!font->valid)
    {

        if (codepoint >= 32u)
        {
            return font->size * 0.55f;
        }
        return 0.0f;
    }

    slot = bloom_font_glyph_slot(font, codepoint);
    if (slot >= 0 && (bloom_u32)slot < font->glyph_count)
    {
        return font->glyphs[(bloom_u32)slot].advance;
    }

    slot = bloom_font_glyph_slot(font, (bloom_u32)'?');
    if (slot >= 0 && (bloom_u32)slot < font->glyph_count)
    {
        return font->glyphs[(bloom_u32)slot].advance;
    }

    return font->size * 0.55f;
}

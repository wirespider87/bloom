#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "core/graphics/font/font.h"
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

void bloom_font_set_active(bloom_font *font)
{
    g_bloom_active_font = font;
}

bloom_font *bloom_font_get_active(void)
{
    return g_bloom_active_font;
}

#ifdef _WIN32

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

static bloom_bool bloom_font_build_gdi_ex(bloom_font *font, bloom_f32 size, const char *face_name)
{
    const int oversample = 4;
    const int first_char = 32;
    const int glyph_count = 96;
    const int columns = 16;
    const int rows = 6;
    const int raster_size = (int)ceilf(size * (bloom_f32)oversample);
    const int src_cell_w = bloom_font_align_up_int(raster_size * 2, oversample);
    const int src_cell_h = bloom_font_align_up_int(raster_size * 2, oversample);
    const int dst_cell_w = src_cell_w / oversample;
    const int dst_cell_h = src_cell_h / oversample;
    const int src_atlas_w = columns * src_cell_w;
    const int src_atlas_h = rows * src_cell_h;
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
    bmi.bmiHeader.biWidth = src_atlas_w;
    bmi.bmiHeader.biHeight = -src_atlas_h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    bmp = CreateDIBSection(mem_dc, &bmi, DIB_RGB_COLORS, &dib_bits, NULL, 0);
    if (!bmp || !dib_bits)
    {
        goto cleanup;
    }

    SelectObject(mem_dc, bmp);
    memset(dib_bits, 0, (size_t)src_atlas_w * (size_t)src_atlas_h * 4u);
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

    font->atlas_width = (bloom_u32)dst_atlas_w;
    font->atlas_height = (bloom_u32)dst_atlas_h;
    font->atlas_pixels = (bloom_u8 *)malloc((size_t)dst_atlas_w * (size_t)dst_atlas_h);
    font->glyph_capacity = glyph_count;
    font->glyphs = (bloom_glyph *)malloc(sizeof(bloom_glyph) * glyph_count);
    if (!font->atlas_pixels || !font->glyphs)
    {
        goto cleanup;
    }
    memset(font->atlas_pixels, 0, (size_t)dst_atlas_w * (size_t)dst_atlas_h);

    for (i = 0; i < glyph_count; ++i)
    {
        char text[2];
        SIZE extent;
        int col = i % columns;
        int row = i / columns;
        int src_cell_x = col * src_cell_w;
        int src_cell_y = row * src_cell_h;
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

        text[0] = (char)(first_char + i);
        text[1] = '\0';

        PatBlt(mem_dc, src_cell_x, src_cell_y, src_cell_w, src_cell_h, BLACKNESS);
        GetTextExtentPoint32A(mem_dc, text, 1, &extent);

        draw_y_offset = bloom_font_align_down_int((src_cell_h - tm.tmHeight) / 2, oversample);
        if (draw_y_offset < 0)
        {
            draw_y_offset = 0;
        }

        draw_x = src_cell_x + glyph_padding;
        draw_y = src_cell_y + draw_y_offset;
        TextOutA(mem_dc, draw_x, draw_y, text, 1);

        dst_x0 = dst_cell_x + ((draw_x - src_cell_x) / oversample);
        dst_y0 = dst_cell_y + ((draw_y - src_cell_y) / oversample);
        dst_x1 = dst_x0 + (bloom_font_align_up_int(extent.cx, oversample) / oversample);
        dst_y1 = dst_y0 + (bloom_font_align_up_int(tm.tmHeight, oversample) / oversample);

        g = &font->glyphs[i];
        g->codepoint = (bloom_u32)(first_char + i);
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
    }

    src_pixels = (const bloom_u32 *)dib_bits;
    for (y = 0; y < dst_atlas_h; ++y)
    {
        for (x = 0; x < dst_atlas_w; ++x)
        {
            font->atlas_pixels[y * dst_atlas_w + x] = bloom_font_downsample_coverage(
                src_pixels,
                src_atlas_w,
                src_atlas_h,
                x * oversample,
                y * oversample,
                oversample);
        }
    }

    font->glyph_count = glyph_count;
    font->size = size;
    font->ascent = (bloom_f32)tm.tmAscent * inv_os;
    font->descent = (bloom_f32)tm.tmDescent * inv_os;
    font->line_height = (bloom_f32)(tm.tmHeight + tm.tmExternalLeading) * inv_os;
    font->valid = BLOOM_TRUE;
    ok = BLOOM_TRUE;

cleanup:
    if (!ok)
    {
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
        "Arial",
        "Tahoma"
    };
    bloom_u32 i;

    for (i = 0; i < (bloom_u32)(sizeof(candidates) / sizeof(candidates[0])); ++i)
    {
        if (bloom_font_build_gdi_ex(font, size, candidates[i]))
        {
            return BLOOM_TRUE;
        }
    }

    return BLOOM_FALSE;
}

#endif

void bloom_font_init(bloom_font *font)
{
    memset(font, 0, sizeof(bloom_font));
}

bloom_bool bloom_font_build_default(bloom_font *font, bloom_f32 size)
{
#ifdef _WIN32
    return bloom_font_build_gdi(font, size);
#endif
    return BLOOM_FALSE;
}

bloom_bool bloom_font_load_from_memory(bloom_font *font, const bloom_u8 *data, bloom_u64 size, bloom_f32 pixel_size)
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
        built = bloom_font_build_gdi_ex(font, pixel_size, face_name);
    }

    RemoveFontMemResourceEx(font_resource);
    return built;
#else
    (void)font;
    (void)data;
    (void)size;
    (void)pixel_size;
    return BLOOM_FALSE;
#endif
}

void bloom_font_destroy(bloom_font *font)
{
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
    while (*text)
    {
        if (*text == '\n')
        {
            if (line_width > width)
            {
                width = line_width;
            }
            line_width = 0;
        }
        else
        {
            bloom_u32 codepoint = (bloom_u32)(bloom_u8)*text;
            if (codepoint >= 32)
            {
                line_width += bloom_font_char_width(font, codepoint);
            }
        }
        text++;
    }
    if (line_width > width)
    {
        width = line_width;
    }
    return width;
}

bloom_f32 bloom_font_char_width(bloom_font *font, bloom_u32 codepoint)
{
    if (codepoint >= 32 && codepoint < 128 && font->glyph_count > 0)
    {
        bloom_u32 idx = codepoint - 32;
        if (idx < font->glyph_count)
        {
            return font->glyphs[idx].advance;
        }
    }
    return font->size * 0.55f;
}

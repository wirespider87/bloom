#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "core/graphics/font/font.h"
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

static bloom_bool bloom_font_build_gdi_ex(bloom_font *font, bloom_f32 size, const char *face_name)
{
    const bloom_f32 oversample = 4.0f;
    const int first_char = 32;
    const int glyph_count = 96;
    const int columns = 16;
    const int rows = 6;
    const int raster_size = (int)ceilf(size * oversample);
    const int cell_w = raster_size * 2;
    const int cell_h = raster_size * 2;
    const int atlas_w = columns * cell_w;
    const int atlas_h = rows * cell_h;
    const int glyph_padding = (int)ceilf(oversample * 2.0f);
    const bloom_f32 inv_os = 1.0f / oversample;
    int i;

    HDC screen_dc = GetDC(NULL);
    HDC mem_dc = CreateCompatibleDC(screen_dc);
    BITMAPINFO bmi;
    void *dib_bits = NULL;
    HBITMAP bmp = NULL;
    HFONT font_handle = NULL;
    HFONT old_font = NULL;
    bloom_bool ok = BLOOM_FALSE;

    if (!screen_dc || !mem_dc)
    {
        if (mem_dc) DeleteDC(mem_dc);
        if (screen_dc) ReleaseDC(NULL, screen_dc);
        return BLOOM_FALSE;
    }

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = atlas_w;
    bmi.bmiHeader.biHeight = -atlas_h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    bmp = CreateDIBSection(mem_dc, &bmi, DIB_RGB_COLORS, &dib_bits, NULL, 0);
    if (!bmp || !dib_bits)
    {
        goto cleanup;
    }

    SelectObject(mem_dc, bmp);
    memset(dib_bits, 0, atlas_w * atlas_h * 4);
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
        CLEARTYPE_NATURAL_QUALITY,
        VARIABLE_PITCH,
        face_name);
    if (!font_handle)
    {
        goto cleanup;
    }

    old_font = (HFONT)SelectObject(mem_dc, font_handle);

    font->atlas_width = (bloom_u32)atlas_w;
    font->atlas_height = (bloom_u32)atlas_h;
    font->atlas_pixels = (bloom_u8 *)malloc((size_t)atlas_w * (size_t)atlas_h);
    font->glyph_capacity = glyph_count;
    font->glyphs = (bloom_glyph *)malloc(sizeof(bloom_glyph) * glyph_count);
    if (!font->atlas_pixels || !font->glyphs)
    {
        goto cleanup;
    }
    memset(font->atlas_pixels, 0, (size_t)atlas_w * (size_t)atlas_h);

    for (i = 0; i < glyph_count; ++i)
    {
        char text[2];
        SIZE extent;
        TEXTMETRICA tm;
        int col = i % columns;
        int row = i / columns;
        int cell_x = col * cell_w;
        int cell_y = row * cell_h;
        int draw_x;
        int draw_y;
        int x;
        int y;
        bloom_glyph *g;

        text[0] = (char)(first_char + i);
        text[1] = '\0';

        PatBlt(mem_dc, cell_x, cell_y, cell_w, cell_h, BLACKNESS);
        GetTextExtentPoint32A(mem_dc, text, 1, &extent);
        GetTextMetricsA(mem_dc, &tm);

        draw_x = cell_x + glyph_padding;
        draw_y = cell_y + ((cell_h - tm.tmHeight) / 2);
        TextOutA(mem_dc, draw_x, draw_y, text, 1);

        for (y = 0; y < cell_h; ++y)
        {
            for (x = 0; x < cell_w; ++x)
            {
                bloom_u32 *src = (bloom_u32 *)dib_bits;
                bloom_u32 pixel = src[(cell_y + y) * atlas_w + (cell_x + x)];
                bloom_u8 r = (bloom_u8)(pixel & 0xFF);
                bloom_u8 gch = (bloom_u8)((pixel >> 8) & 0xFF);
                bloom_u8 b = (bloom_u8)((pixel >> 16) & 0xFF);
                int lum = ((int)r + (int)gch + (int)b) / 3;
                float t = (float)lum / 255.0f;
                float boosted = powf(t, 0.55f);
                bloom_u8 a = (bloom_u8)(boosted * 255.0f + 0.5f);
                font->atlas_pixels[(cell_y + y) * atlas_w + (cell_x + x)] = a;
            }
        }

        g = &font->glyphs[i];
        g->codepoint = (bloom_u32)(first_char + i);
        g->advance = (bloom_f32)extent.cx * inv_os;
        if (g->advance < size * 0.33f)
        {
            g->advance = size * 0.33f;
        }
        g->x0 = 0.0f;
        g->y0 = 0.0f;
        g->x1 = (bloom_f32)extent.cx * inv_os;
        g->y1 = (bloom_f32)tm.tmHeight * inv_os;
        g->u0 = (bloom_f32)draw_x / (bloom_f32)atlas_w;
        g->v0 = (bloom_f32)draw_y / (bloom_f32)atlas_h;
        g->u1 = (bloom_f32)(draw_x + extent.cx) / (bloom_f32)atlas_w;
        g->v1 = (bloom_f32)(draw_y + tm.tmHeight) / (bloom_f32)atlas_h;
    }

    font->glyph_count = glyph_count;
    font->size = size;
    {
        TEXTMETRICA tm;
        GetTextMetricsA(mem_dc, &tm);
        font->ascent = (bloom_f32)tm.tmAscent * inv_os;
        font->descent = (bloom_f32)tm.tmDescent * inv_os;
        font->line_height = (bloom_f32)tm.tmHeight * inv_os;
    }
    font->valid = BLOOM_TRUE;
    ok = BLOOM_TRUE;

cleanup:
    if (!ok)
    {
        free(font->atlas_pixels);
        free(font->glyphs);
        font->atlas_pixels = NULL;
        font->glyphs = NULL;
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
    return bloom_font_build_gdi_ex(font, size, "Segoe UI");
}

static bloom_bool bloom_font_read_file_bytes(const char *path, bloom_u8 **out_data, bloom_u64 *out_size)
{
    FILE *fp;
    long len;
    bloom_u8 *data;
    size_t read_bytes;

    if (!path || !out_data || !out_size)
    {
        return BLOOM_FALSE;
    }

    *out_data = NULL;
    *out_size = 0;

    fp = fopen(path, "rb");
    if (!fp)
    {
        return BLOOM_FALSE;
    }

    if (fseek(fp, 0, SEEK_END) != 0)
    {
        fclose(fp);
        return BLOOM_FALSE;
    }

    len = ftell(fp);
    if (len <= 0 || fseek(fp, 0, SEEK_SET) != 0)
    {
        fclose(fp);
        return BLOOM_FALSE;
    }

    data = (bloom_u8 *)malloc((size_t)len);
    if (!data)
    {
        fclose(fp);
        return BLOOM_FALSE;
    }

    read_bytes = fread(data, 1, (size_t)len, fp);
    fclose(fp);
    if (read_bytes != (size_t)len)
    {
        free(data);
        return BLOOM_FALSE;
    }

    *out_data = data;
    *out_size = (bloom_u64)len;
    return BLOOM_TRUE;
}

static bloom_bool bloom_font_build_default_ttf_win32(bloom_font *font, bloom_f32 size)
{
    char windows_dir[MAX_PATH];
    char path[MAX_PATH * 2];
    const char *candidates[] = {
        "segoeui.ttf",
        "arial.ttf",
        "tahoma.ttf"
    };
    bloom_u32 i;

    if (!GetWindowsDirectoryA(windows_dir, (UINT)sizeof(windows_dir)))
    {
        return BLOOM_FALSE;
    }

    for (i = 0; i < (bloom_u32)(sizeof(candidates) / sizeof(candidates[0])); ++i)
    {
        bloom_u8 *data = NULL;
        bloom_u64 data_size = 0;

        if (snprintf(path, sizeof(path), "%s\\Fonts\\%s", windows_dir, candidates[i]) <= 0)
        {
            continue;
        }

        if (!bloom_font_read_file_bytes(path, &data, &data_size))
        {
            continue;
        }

        if (bloom_font_load_from_memory(font, data, data_size, size))
        {
            free(data);
            return BLOOM_TRUE;
        }

        free(data);
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
    if (bloom_font_build_default_ttf_win32(font, size))
    {
        return BLOOM_TRUE;
    }

    if (bloom_font_build_gdi(font, size))
    {
        return BLOOM_TRUE;
    }
#endif
    return BLOOM_FALSE;
}

bloom_bool bloom_font_load_from_memory(bloom_font *font, const bloom_u8 *data, bloom_u64 size, bloom_f32 pixel_size)
{
#ifdef _WIN32
    char family_name[128];
    DWORD loaded_fonts = 0;
    HANDLE font_res = NULL;
    bloom_bool ok;

    if (!font || !data || size == 0 || pixel_size <= 0.0f)
    {
        return BLOOM_FALSE;
    }

    family_name[0] = '\0';
    if (!bloom_font_extract_ttf_name(data, size, family_name, (int)sizeof(family_name)))
    {
        return BLOOM_FALSE;
    }

    font_res = AddFontMemResourceEx((void *)data, (DWORD)size, NULL, &loaded_fonts);
    if (!font_res || loaded_fonts == 0)
    {
        return BLOOM_FALSE;
    }

    ok = bloom_font_build_gdi_ex(font, pixel_size, family_name);

    RemoveFontMemResourceEx(font_res);
    return ok;
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
            line_width += bloom_font_char_width(font, (bloom_u32)(bloom_u8)*text);
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

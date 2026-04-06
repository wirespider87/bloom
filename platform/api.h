#ifndef BLOOM_PLATFORM_API_H
#define BLOOM_PLATFORM_API_H

#include "core/base/api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bloom_platform_window bloom_platform_window;

enum
{
    BLOOM_PLATFORM_WINDOW_FLAG_RESIZABLE       = (1 << 0),
    BLOOM_PLATFORM_WINDOW_FLAG_TRANSPARENT     = (1 << 1),
    BLOOM_PLATFORM_WINDOW_FLAG_TOPMOST         = (1 << 2),
    BLOOM_PLATFORM_WINDOW_FLAG_BORDERLESS      = (1 << 3),
    BLOOM_PLATFORM_WINDOW_FLAG_TITLEBAR        = (1 << 4),
    BLOOM_PLATFORM_WINDOW_FLAG_MINIMIZE_BUTTON = (1 << 5),
    BLOOM_PLATFORM_WINDOW_FLAG_MAXIMIZE_BUTTON = (1 << 6)
};

#define BLOOM_PLATFORM_WINDOW_FLAG_DEFAULT \
    (BLOOM_PLATFORM_WINDOW_FLAG_RESIZABLE | \
     BLOOM_PLATFORM_WINDOW_FLAG_TITLEBAR | \
     BLOOM_PLATFORM_WINDOW_FLAG_MINIMIZE_BUTTON | \
     BLOOM_PLATFORM_WINDOW_FLAG_MAXIMIZE_BUTTON)

typedef struct bloom_platform_config
{
    const char *title;
    bloom_i32   width;
    bloom_i32   height;
    bloom_u32   flags;
    bloom_f32   opacity;
    bloom_bool  resizable;
    bloom_bool  transparent;
    bloom_bool  topmost;
    bloom_bool  borderless;
} bloom_platform_config;

#if defined(BLOOM_INTERNAL_BUILD) || defined(BLOOM_ENABLE_LEGACY_API)
bloom_platform_window *bloom_platform_create(bloom_platform_config *config);
void bloom_platform_destroy(bloom_platform_window *win);
bloom_bool bloom_platform_poll(bloom_platform_window *win);
void bloom_platform_swap(bloom_platform_window *win);
void bloom_platform_get_size(bloom_platform_window *win, bloom_i32 *w, bloom_i32 *h);
bloom_f32 bloom_platform_get_opacity(bloom_platform_window *win);
void bloom_platform_set_opacity(bloom_platform_window *win, bloom_f32 opacity);
bloom_bool bloom_platform_get_auto_dpi_awareness(void);
void bloom_platform_set_auto_dpi_awareness(bloom_bool enabled);
bloom_f64 bloom_platform_get_time(void);
bloom_bool bloom_platform_get_clipboard_text(char *buffer, bloom_u32 buffer_size);
bloom_bool bloom_platform_set_clipboard_text(const char *text);
#endif

#ifdef __cplusplus
}
#endif

#endif

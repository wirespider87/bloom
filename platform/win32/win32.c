#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dwmapi.h>
#include <GL/gl.h>
#include "platform/api.h"
#include "core/runtime/context/context.h"
#include <stdlib.h>
#include <string.h>

#ifndef WGL_DRAW_TO_WINDOW_ARB
#define WGL_DRAW_TO_WINDOW_ARB   0x2001
#define WGL_SUPPORT_OPENGL_ARB   0x2010
#define WGL_DOUBLE_BUFFER_ARB    0x2011
#define WGL_PIXEL_TYPE_ARB       0x2013
#define WGL_COLOR_BITS_ARB       0x2014
#define WGL_DEPTH_BITS_ARB       0x2022
#define WGL_STENCIL_BITS_ARB     0x2023
#define WGL_ALPHA_BITS_ARB       0x201B
#define WGL_TYPE_RGBA_ARB        0x202B
#define WGL_SAMPLE_BUFFERS_ARB   0x2041
#define WGL_SAMPLES_ARB          0x2042
#endif

typedef BOOL (WINAPI *bloom_wgl_choose_pixel_format_arb_proc)(HDC hdc, const int *attrib_i,
                                                              const FLOAT *attrib_f, UINT max_formats,
                                                              int *formats, UINT *num_formats);
typedef const char *(WINAPI *bloom_wgl_get_extensions_string_arb_proc)(HDC hdc);

struct bloom_platform_window
{
    HWND   hwnd;
    HDC    hdc;
    HGLRC  hglrc;
    bloom_bool should_close;
    bloom_bool borderless;
    bloom_u32  flags;
    bloom_f32  opacity;
    bloom_i32  width;
    bloom_i32  height;
};

static bloom_platform_window *g_platform_win = NULL;
static bloom_bool g_nc_dragging = BLOOM_FALSE;
static POINT g_nc_drag_offset;

static bloom_bool bloom_str_contains(const char *haystack, const char *needle)
{
    return (haystack && needle && strstr(haystack, needle) != NULL) ? BLOOM_TRUE : BLOOM_FALSE;
}

static bloom_u32 bloom_platform_resolve_flags(const bloom_platform_config *config)
{
    bloom_u32 flags;

    if (!config)
    {
        return BLOOM_PLATFORM_WINDOW_FLAG_DEFAULT;
    }

    if (config->flags != 0)
    {
        return config->flags;
    }

    flags = BLOOM_PLATFORM_WINDOW_FLAG_DEFAULT;
    if (!config->resizable)
    {
        flags &= ~BLOOM_PLATFORM_WINDOW_FLAG_RESIZABLE;
        flags &= ~BLOOM_PLATFORM_WINDOW_FLAG_MAXIMIZE_BUTTON;
    }
    if (config->transparent)
    {
        flags |= BLOOM_PLATFORM_WINDOW_FLAG_TRANSPARENT;
    }
    if (config->topmost)
    {
        flags |= BLOOM_PLATFORM_WINDOW_FLAG_TOPMOST;
    }
    if (config->borderless)
    {
        flags |= BLOOM_PLATFORM_WINDOW_FLAG_BORDERLESS;
    }

    return flags;
}

static bloom_f32 bloom_platform_clamp_opacity(bloom_f32 opacity)
{
    if (opacity < 0.0f)
    {
        return 0.0f;
    }
    if (opacity > 1.0f)
    {
        return 1.0f;
    }
    return opacity;
}

static bloom_f32 bloom_platform_default_opacity(const bloom_platform_config *config)
{
    if (!config || config->opacity <= 0.0f)
    {
        return 1.0f;
    }
    return bloom_platform_clamp_opacity(config->opacity);
}

static DWORD bloom_platform_window_style(bloom_u32 flags)
{
    DWORD style = 0;

    if (flags & BLOOM_PLATFORM_WINDOW_FLAG_BORDERLESS)
    {
        return WS_POPUP;
    }

    if (flags & BLOOM_PLATFORM_WINDOW_FLAG_TITLEBAR)
    {
        style |= WS_CAPTION;
    }
    if (flags & (BLOOM_PLATFORM_WINDOW_FLAG_TITLEBAR |
                 BLOOM_PLATFORM_WINDOW_FLAG_MINIMIZE_BUTTON |
                 BLOOM_PLATFORM_WINDOW_FLAG_MAXIMIZE_BUTTON))
    {
        style |= WS_SYSMENU;
    }
    if (flags & BLOOM_PLATFORM_WINDOW_FLAG_RESIZABLE)
    {
        style |= WS_THICKFRAME;
    }
    if (flags & BLOOM_PLATFORM_WINDOW_FLAG_MINIMIZE_BUTTON)
    {
        style |= WS_MINIMIZEBOX;
    }
    if (flags & BLOOM_PLATFORM_WINDOW_FLAG_MAXIMIZE_BUTTON)
    {
        style |= WS_MAXIMIZEBOX;
    }

    if (style == 0)
    {
        style = WS_POPUP;
    }

    return style;
}

static DWORD bloom_platform_window_exstyle(bloom_u32 flags, bloom_f32 opacity)
{
    DWORD exstyle = 0;

    if (flags & BLOOM_PLATFORM_WINDOW_FLAG_TOPMOST)
    {
        exstyle |= WS_EX_TOPMOST;
    }
    if ((flags & BLOOM_PLATFORM_WINDOW_FLAG_TRANSPARENT) || opacity < 1.0f)
    {
        exstyle |= WS_EX_LAYERED;
    }

    return exstyle;
}

static void bloom_platform_apply_opacity(bloom_platform_window *win, bloom_f32 opacity)
{
    LONG_PTR exstyle;
    BYTE alpha;

    if (!win || !win->hwnd)
    {
        return;
    }

    opacity = bloom_platform_clamp_opacity(opacity);
    exstyle = GetWindowLongPtrA(win->hwnd, GWL_EXSTYLE);
    if ((exstyle & WS_EX_LAYERED) == 0)
    {
        SetWindowLongPtrA(win->hwnd, GWL_EXSTYLE, exstyle | WS_EX_LAYERED);
    }

    alpha = (BYTE)(opacity * 255.0f + 0.5f);
    SetLayeredWindowAttributes(win->hwnd, 0, alpha, LWA_ALPHA);
    win->opacity = opacity;
}

static int bloom_choose_multisample_pixel_format(HDC target_hdc, PIXELFORMATDESCRIPTOR *target_pfd)
{
    HINSTANCE instance = GetModuleHandleA(NULL);
    const char *dummy_class_name = "BloomDummyWglClass";
    HWND dummy_hwnd = NULL;
    HDC dummy_hdc = NULL;
    HGLRC dummy_ctx = NULL;
    int pixel_format = 0;
    bloom_wgl_choose_pixel_format_arb_proc choose_pf_arb = NULL;
    bloom_wgl_get_extensions_string_arb_proc get_extensions_arb = NULL;
    WNDCLASSEXA wc;
    PIXELFORMATDESCRIPTOR dummy_pfd;
    const char *extensions = NULL;
    int sample_options[3] = {8, 4, 2};
    int sample_index;

    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = instance;
    wc.lpszClassName = dummy_class_name;
    RegisterClassExA(&wc);

    dummy_hwnd = CreateWindowExA(0, dummy_class_name, "", WS_OVERLAPPED,
                                 0, 0, 1, 1, NULL, NULL, instance, NULL);
    if (!dummy_hwnd)
    {
        return 0;
    }

    dummy_hdc = GetDC(dummy_hwnd);
    memset(&dummy_pfd, 0, sizeof(dummy_pfd));
    dummy_pfd.nSize = sizeof(dummy_pfd);
    dummy_pfd.nVersion = 1;
    dummy_pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    dummy_pfd.iPixelType = PFD_TYPE_RGBA;
    dummy_pfd.cColorBits = 32;
    dummy_pfd.cAlphaBits = 8;
    dummy_pfd.cDepthBits = 24;
    dummy_pfd.cStencilBits = 8;

    pixel_format = ChoosePixelFormat(dummy_hdc, &dummy_pfd);
    if (pixel_format == 0 || !SetPixelFormat(dummy_hdc, pixel_format, &dummy_pfd))
    {
        pixel_format = 0;
        goto cleanup;
    }

    dummy_ctx = wglCreateContext(dummy_hdc);
    if (!dummy_ctx || !wglMakeCurrent(dummy_hdc, dummy_ctx))
    {
        pixel_format = 0;
        goto cleanup;
    }

    choose_pf_arb = (bloom_wgl_choose_pixel_format_arb_proc)wglGetProcAddress("wglChoosePixelFormatARB");
    get_extensions_arb = (bloom_wgl_get_extensions_string_arb_proc)wglGetProcAddress("wglGetExtensionsStringARB");

    if (!choose_pf_arb || !get_extensions_arb)
    {
        pixel_format = 0;
        goto cleanup;
    }

    extensions = get_extensions_arb(dummy_hdc);
    if (!bloom_str_contains(extensions, "WGL_ARB_pixel_format") ||
        !bloom_str_contains(extensions, "WGL_ARB_multisample"))
    {
        pixel_format = 0;
        goto cleanup;
    }

    for (sample_index = 0; sample_index < 3 && pixel_format == 0; ++sample_index)
    {
        int format_attribs[] = {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB, 32,
            WGL_ALPHA_BITS_ARB, 8,
            WGL_DEPTH_BITS_ARB, 24,
            WGL_STENCIL_BITS_ARB, 8,
            WGL_SAMPLE_BUFFERS_ARB, 1,
            WGL_SAMPLES_ARB, sample_options[sample_index],
            0, 0
        };
        UINT num_formats = 0;
        int chosen_format = 0;

        if (choose_pf_arb(target_hdc, format_attribs, NULL, 1, &chosen_format, &num_formats) &&
            num_formats > 0 && chosen_format != 0)
        {
            pixel_format = chosen_format;
            DescribePixelFormat(target_hdc, pixel_format, sizeof(*target_pfd), target_pfd);
        }
    }

cleanup:
    wglMakeCurrent(NULL, NULL);
    if (dummy_ctx)
    {
        wglDeleteContext(dummy_ctx);
    }
    if (dummy_hdc)
    {
        ReleaseDC(dummy_hwnd, dummy_hdc);
    }
    if (dummy_hwnd)
    {
        DestroyWindow(dummy_hwnd);
    }

    return pixel_format;
}

static int bloom_translate_key(WPARAM wp)
{
    switch (wp)
    {
    case VK_TAB:    return BLOOM_KEY_TAB;
    case VK_LEFT:   return BLOOM_KEY_LEFT;
    case VK_RIGHT:  return BLOOM_KEY_RIGHT;
    case VK_UP:     return BLOOM_KEY_UP;
    case VK_DOWN:   return BLOOM_KEY_DOWN;
    case VK_HOME:   return BLOOM_KEY_HOME;
    case VK_END:    return BLOOM_KEY_END;
    case VK_DELETE: return BLOOM_KEY_DELETE;
    case VK_BACK:   return BLOOM_KEY_BACKSPACE;
    case VK_RETURN: return BLOOM_KEY_ENTER;
    case VK_ESCAPE: return BLOOM_KEY_ESCAPE;
    case 'A':       return BLOOM_KEY_A;
    case 'C':       return BLOOM_KEY_C;
    case 'V':       return BLOOM_KEY_V;
    case 'X':       return BLOOM_KEY_X;
    case 'Y':       return BLOOM_KEY_Y;
    case 'Z':       return BLOOM_KEY_Z;
    default:        return BLOOM_KEY_NONE;
    }
}

static bloom_bool bloom_platform_point_in_rect(POINT point, bloom_rect rect)
{
    return point.x >= (LONG)rect.x &&
           point.y >= (LONG)rect.y &&
           point.x < (LONG)(rect.x + rect.w) &&
           point.y < (LONG)(rect.y + rect.h);
}

static bloom_bool bloom_platform_point_over_ui(POINT point)
{
    bloom_context *ctx = bloom_get_context();
    int i;

    if (!ctx)
    {
        return BLOOM_FALSE;
    }

    for (i = ctx->popup_count - 1; i >= 0; --i)
    {
        if (ctx->popups[i].open && bloom_platform_point_in_rect(point, ctx->popups[i].rect))
        {
            return BLOOM_TRUE;
        }
    }

    for (i = ctx->window_count - 1; i >= 0; --i)
    {
        bloom_window *win = &ctx->windows[i];
        if (win->active && bloom_platform_point_in_rect(point, win->rect))
        {
            return BLOOM_TRUE;
        }
    }

    return BLOOM_FALSE;
}

static bloom_bool bloom_platform_point_in_window_title_drag_zone(POINT point)
{
    bloom_context *ctx = bloom_get_context();
    bloom_window *best = NULL;
    bloom_i32 best_order = -2147483647;
    int i;

    if (!ctx)
    {
        return BLOOM_FALSE;
    }

    for (i = 0; i < ctx->window_count; ++i)
    {
        bloom_window *win = &ctx->windows[i];
        if (!win->active || !(win->flags & BLOOM_WINDOW_HOST_MOVE) ||
            (win->flags & BLOOM_WINDOW_NO_TITLE) || !bloom_platform_point_in_rect(point, win->rect))
        {
            continue;
        }
        if (win->order > best_order)
        {
            best = win;
            best_order = win->order;
        }
    }

    if (best)
    {
        bloom_f32 title_h = ctx->style.title_bar_height;
        bloom_f32 control_size = title_h > 14.0f ? title_h - 12.0f : 0.0f;
        bloom_f32 controls_width = 0.0f;
        bloom_rect title_rect = bloom_make_rect(best->rect.x, best->rect.y, best->rect.w, title_h);
        bloom_rect drag_rect = title_rect;

        if (!(best->flags & BLOOM_WINDOW_NO_CLOSE))
        {
            controls_width += control_size + 6.0f;
        }
        if (!(best->flags & BLOOM_WINDOW_NO_MINIMIZE))
        {
            controls_width += control_size + 6.0f;
        }
        if (!(best->flags & BLOOM_WINDOW_NO_COLLAPSE) && !best->minimized)
        {
            controls_width += control_size + 6.0f;
        }
        if (controls_width > 0.0f)
        {
            drag_rect.w -= controls_width + 8.0f;
        }

        return bloom_platform_point_in_rect(point, drag_rect);
    }

    return BLOOM_FALSE;
}

static LRESULT bloom_platform_borderless_hit_test(bloom_platform_window *win, POINT point)
{
    const LONG grip = 8;
    bloom_bool left;
    bloom_bool right;
    bloom_bool top;
    bloom_bool bottom;

    if (!win)
    {
        return HTCLIENT;
    }

    left = point.x >= 0 && point.x < grip;
    right = point.x >= win->width - grip && point.x < win->width;
    top = point.y >= 0 && point.y < grip;
    bottom = point.y >= win->height - grip && point.y < win->height;

    if (win->flags & BLOOM_PLATFORM_WINDOW_FLAG_RESIZABLE)
    {
        if (top && left) return HTTOPLEFT;
        if (top && right) return HTTOPRIGHT;
        if (bottom && left) return HTBOTTOMLEFT;
        if (bottom && right) return HTBOTTOMRIGHT;
        if (left) return HTLEFT;
        if (right) return HTRIGHT;
        if (top) return HTTOP;
        if (bottom) return HTBOTTOM;
    }

    if (bloom_platform_point_in_window_title_drag_zone(point))
    {
        return HTCAPTION;
    }

    if (!bloom_platform_point_over_ui(point))
    {
        return HTCAPTION;
    }

    return HTCLIENT;
}

static LRESULT CALLBACK bloom_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    bloom_context *ctx = bloom_get_context();
    bloom_input *input = ctx ? &ctx->input : NULL;

    switch (msg)
    {
    case WM_NCHITTEST:
        if (g_platform_win && g_platform_win->borderless)
        {
            POINT point;
            point.x = (LONG)(short)LOWORD(lp);
            point.y = (LONG)(short)HIWORD(lp);
            ScreenToClient(hwnd, &point);
            return bloom_platform_borderless_hit_test(g_platform_win, point);
        }
        break;

    case WM_CLOSE:
        if (g_platform_win)
        {
            g_platform_win->should_close = BLOOM_TRUE;
        }
        return 0;

    case WM_SIZE:
        if (g_platform_win)
        {
            g_platform_win->width = LOWORD(lp);
            g_platform_win->height = HIWORD(lp);
        }
        break;

    case WM_NCMOUSEMOVE:
        if (input)
        {
            POINT pt;
            pt.x = (LONG)(short)LOWORD(lp);
            pt.y = (LONG)(short)HIWORD(lp);
            ScreenToClient(hwnd, &pt);
            bloom_input_set_mouse_pos(input, (bloom_f32)pt.x, (bloom_f32)pt.y);
        }
        break;

    case WM_MOUSEMOVE:
        if (input)
        {
            bloom_input_set_mouse_pos(input,
                (bloom_f32)(short)LOWORD(lp), (bloom_f32)(short)HIWORD(lp));
        }
        if (g_nc_dragging)
        {
            POINT cursor;
            GetCursorPos(&cursor);
            SetWindowPos(hwnd, NULL,
                cursor.x - g_nc_drag_offset.x,
                cursor.y - g_nc_drag_offset.y,
                0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        break;

    case WM_LBUTTONDOWN:
        if (input) bloom_input_set_mouse_button(input, BLOOM_MOUSE_LEFT, BLOOM_TRUE);
        SetCapture(hwnd);
        break;
    case WM_LBUTTONUP:
        if (input) bloom_input_set_mouse_button(input, BLOOM_MOUSE_LEFT, BLOOM_FALSE);
        if (g_nc_dragging)
        {
            g_nc_dragging = BLOOM_FALSE;
        }
        ReleaseCapture();
        break;
    case WM_NCLBUTTONDOWN:
    {
        POINT client_pt;
        client_pt.x = (LONG)(short)LOWORD(lp);
        client_pt.y = (LONG)(short)HIWORD(lp);
        ScreenToClient(hwnd, &client_pt);
        if (input)
        {
            bloom_input_set_mouse_pos(input, (bloom_f32)client_pt.x, (bloom_f32)client_pt.y);
            bloom_input_set_mouse_button(input, BLOOM_MOUSE_LEFT, BLOOM_TRUE);
        }
        if (wp == HTCAPTION)
        {
            POINT cursor;
            RECT wr;
            GetCursorPos(&cursor);
            GetWindowRect(hwnd, &wr);
            g_nc_drag_offset.x = cursor.x - wr.left;
            g_nc_drag_offset.y = cursor.y - wr.top;
            g_nc_dragging = BLOOM_TRUE;
            SetCapture(hwnd);
            return 0;
        }
        break;
    }
    case WM_NCLBUTTONUP:
        if (input) bloom_input_set_mouse_button(input, BLOOM_MOUSE_LEFT, BLOOM_FALSE);
        if (g_nc_dragging)
        {
            g_nc_dragging = BLOOM_FALSE;
            ReleaseCapture();
        }
        break;
    case WM_RBUTTONDOWN:
        if (input) bloom_input_set_mouse_button(input, BLOOM_MOUSE_RIGHT, BLOOM_TRUE);
        break;
    case WM_RBUTTONUP:
        if (input) bloom_input_set_mouse_button(input, BLOOM_MOUSE_RIGHT, BLOOM_FALSE);
        break;
    case WM_MBUTTONDOWN:
        if (input) bloom_input_set_mouse_button(input, BLOOM_MOUSE_MIDDLE, BLOOM_TRUE);
        break;
    case WM_MBUTTONUP:
        if (input) bloom_input_set_mouse_button(input, BLOOM_MOUSE_MIDDLE, BLOOM_FALSE);
        break;

    case WM_MOUSEWHEEL:
        if (input)
        {
            bloom_f32 delta = (bloom_f32)GET_WHEEL_DELTA_WPARAM(wp) / (bloom_f32)WHEEL_DELTA;
            bloom_input_set_mouse_wheel(input, delta);
        }
        break;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (input)
        {
            int key = bloom_translate_key(wp);
            if (key != BLOOM_KEY_NONE)
            {
                bloom_input_set_key(input, key, BLOOM_TRUE);
            }
            input->ctrl_held = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            input->shift_held = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            input->alt_held = (GetKeyState(VK_MENU) & 0x8000) != 0;
        }
        break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (input)
        {
            int key = bloom_translate_key(wp);
            if (key != BLOOM_KEY_NONE)
            {
                bloom_input_set_key(input, key, BLOOM_FALSE);
            }
            input->ctrl_held = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            input->shift_held = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            input->alt_held = (GetKeyState(VK_MENU) & 0x8000) != 0;
        }
        break;

    case WM_CHAR:
        if (input && wp >= 32 && wp < 127)
        {
            bloom_input_add_char(input, (char)wp);
        }
        break;
    }

    return DefWindowProcA(hwnd, msg, wp, lp);
}

bloom_platform_window *bloom_platform_create(bloom_platform_config *config)
{
    bloom_platform_window *win = (bloom_platform_window *)calloc(1, sizeof(bloom_platform_window));
    bloom_u32 flags;
    bloom_f32 opacity;
    DWORD style;
    DWORD exstyle;
    if (!win)
    {
        return NULL;
    }

    flags = bloom_platform_resolve_flags(config);
    opacity = bloom_platform_default_opacity(config);

    WNDCLASSEXA wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = bloom_wnd_proc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "BloomWindowClass";
    RegisterClassExA(&wc);

    style = bloom_platform_window_style(flags);
    exstyle = bloom_platform_window_exstyle(flags, opacity);

    RECT rect = {0, 0, config->width, config->height};
    AdjustWindowRectEx(&rect, style, FALSE, exstyle);

    win->hwnd = CreateWindowExA(
        exstyle,
        "BloomWindowClass",
        config->title ? config->title : "Bloom",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, GetModuleHandleA(NULL), NULL);

    if (!win->hwnd)
    {
        free(win);
        return NULL;
    }

    win->hdc = GetDC(win->hwnd);

    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;

    {
        int pf = bloom_choose_multisample_pixel_format(win->hdc, &pfd);
        if (pf == 0)
        {
            pf = ChoosePixelFormat(win->hdc, &pfd);
        }
        SetPixelFormat(win->hdc, pf, &pfd);
    }

    win->hglrc = wglCreateContext(win->hdc);
    wglMakeCurrent(win->hdc, win->hglrc);

    if (flags & BLOOM_PLATFORM_WINDOW_FLAG_TRANSPARENT)
    {
        MARGINS margins = {-1, -1, -1, -1};
        DWM_BLURBEHIND bb = {0};
        bb.dwFlags = DWM_BB_ENABLE;
        bb.fEnable = TRUE;
        DwmExtendFrameIntoClientArea(win->hwnd, &margins);
        DwmEnableBlurBehindWindow(win->hwnd, &bb);
    }

    win->width = config->width;
    win->height = config->height;
    win->should_close = BLOOM_FALSE;
    win->borderless = (flags & BLOOM_PLATFORM_WINDOW_FLAG_BORDERLESS) != 0;
    win->flags = flags;
    win->opacity = 1.0f;

    if ((flags & BLOOM_PLATFORM_WINDOW_FLAG_TRANSPARENT) || opacity < 1.0f)
    {
        bloom_platform_apply_opacity(win, opacity);
    }
    else
    {
        win->opacity = opacity;
    }

    ShowWindow(win->hwnd, SW_SHOW);
    UpdateWindow(win->hwnd);

    g_platform_win = win;
    return win;
}

void bloom_platform_destroy(bloom_platform_window *win)
{
    if (!win)
    {
        return;
    }
    wglMakeCurrent(NULL, NULL);
    if (win->hglrc)
    {
        wglDeleteContext(win->hglrc);
    }
    if (win->hwnd)
    {
        DestroyWindow(win->hwnd);
    }
    if (g_platform_win == win)
    {
        g_platform_win = NULL;
    }
    free(win);
}

bloom_bool bloom_platform_poll(bloom_platform_window *win)
{
    bloom_context *ctx = bloom_get_context();
    if (ctx)
    {
        bloom_input_begin(&ctx->input);
    }

    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return !win->should_close;
}

void bloom_platform_swap(bloom_platform_window *win)
{
    SwapBuffers(win->hdc);
}

void bloom_platform_get_size(bloom_platform_window *win, bloom_i32 *w, bloom_i32 *h)
{
    if (w) *w = win->width;
    if (h) *h = win->height;
}

bloom_f32 bloom_platform_get_opacity(bloom_platform_window *win)
{
    if (!win)
    {
        return 1.0f;
    }
    return win->opacity;
}

void bloom_platform_set_opacity(bloom_platform_window *win, bloom_f32 opacity)
{
    if (!win)
    {
        return;
    }

    bloom_platform_apply_opacity(win, opacity);
}

bloom_f64 bloom_platform_get_time(void)
{
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER counter;
    if (freq.QuadPart == 0)
    {
        QueryPerformanceFrequency(&freq);
    }
    QueryPerformanceCounter(&counter);
    return (bloom_f64)counter.QuadPart / (bloom_f64)freq.QuadPart;
}

bloom_bool bloom_platform_get_clipboard_text(char *buffer, bloom_u32 buffer_size)
{
    HANDLE data;
    char *src;
    bloom_u32 i;

    if (!buffer || buffer_size == 0)
    {
        return BLOOM_FALSE;
    }

    buffer[0] = '\0';
    if (!OpenClipboard(NULL))
    {
        return BLOOM_FALSE;
    }

    data = GetClipboardData(CF_TEXT);
    if (!data)
    {
        CloseClipboard();
        return BLOOM_FALSE;
    }

    src = (char *)GlobalLock(data);
    if (!src)
    {
        CloseClipboard();
        return BLOOM_FALSE;
    }

    i = 0;
    while (src[i] && i + 1 < buffer_size)
    {
        char c = src[i];
        if (c >= 32 && c < 127)
        {
            buffer[i] = c;
            i++;
        }
        else if (c == '\t')
        {
            buffer[i++] = ' ';
        }
        else
        {
            break;
        }
    }
    buffer[i] = '\0';

    GlobalUnlock(data);
    CloseClipboard();
    return BLOOM_TRUE;
}

bloom_bool bloom_platform_set_clipboard_text(const char *text)
{
    HGLOBAL mem;
    char *dst;
    size_t len;

    if (!text)
    {
        return BLOOM_FALSE;
    }

    if (!OpenClipboard(NULL))
    {
        return BLOOM_FALSE;
    }

    EmptyClipboard();
    len = strlen(text);
    mem = GlobalAlloc(GMEM_MOVEABLE, len + 1);
    if (!mem)
    {
        CloseClipboard();
        return BLOOM_FALSE;
    }

    dst = (char *)GlobalLock(mem);
    if (!dst)
    {
        GlobalFree(mem);
        CloseClipboard();
        return BLOOM_FALSE;
    }

    memcpy(dst, text, len + 1);
    GlobalUnlock(mem);
    SetClipboardData(CF_TEXT, mem);
    CloseClipboard();
    return BLOOM_TRUE;
}

#endif

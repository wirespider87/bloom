#ifndef BLOOM_RENDERING_API_H
#define BLOOM_RENDERING_API_H

#include "core/graphics/api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bloom_render_backend bloom_render_backend;

typedef struct bloom_render_backend
{
    bloom_bool (*init)(bloom_render_backend *backend);
    void (*shutdown)(bloom_render_backend *backend);
    void (*render)(bloom_render_backend *backend, bloom_draw_list *dl,
                   bloom_f32 display_w, bloom_f32 display_h);
    bloom_u32 (*create_texture)(bloom_render_backend *backend,
                                 bloom_u32 width, bloom_u32 height,
                                 const bloom_u8 *pixels);
    void (*destroy_texture)(bloom_render_backend *backend, bloom_u32 texture_id);
    void *user_data;
} bloom_render_backend;

#if (defined(BLOOM_INTERNAL_BUILD) || defined(BLOOM_ENABLE_LEGACY_API)) && defined(BLOOM_OPENGL_BACKEND)
bloom_render_backend *bloom_create_opengl_backend(void);
void bloom_destroy_opengl_backend(bloom_render_backend *backend);
#endif

#if (defined(BLOOM_INTERNAL_BUILD) || defined(BLOOM_ENABLE_LEGACY_API)) && defined(BLOOM_D3D11_BACKEND)
#ifdef _WIN32
/* device/context are ID3D11Device* and ID3D11DeviceContext* - passed as void*
    to avoid pulling d3d11.h into every translation unit */

bloom_render_backend *bloom_create_d3d11_backend(void *device, void *device_ctx);
void bloom_destroy_d3d11_backend(bloom_render_backend *backend);
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif

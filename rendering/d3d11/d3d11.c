#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#define COBJMACROS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "rendering/api.h"
#include "core/runtime/context/context.h"

#ifdef BLOOM_D3D11_BACKEND

#include <windows.h>
#include <d3d11.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef HRESULT (WINAPI *PFN_D3DCompile)(
    const void *pSrcData, SIZE_T SrcDataSize, const char *pSourceName,
    const void *pDefines, void *pInclude, const char *pEntrypoint,
    const char *pTarget, UINT Flags1, UINT Flags2,
    ID3DBlob **ppCode, ID3DBlob **ppErrorMsgs);

static const char g_bloom_shader_src[] =
    "cbuffer vertexBuffer : register(b0)\n"
    "{\n"
    "    float4x4 ProjectionMatrix;\n"
    "};\n"
    "struct VS_INPUT\n"
    "{\n"
    "    float2 pos              : POSITION;\n"
    "    float2 center           : CENTER;\n"
    "    float2 half_size        : HALFSIZE;\n"
    "    float2 uv               : TEXCOORD0;\n"
    "    float4 col              : COLOR0;\n"
    "    float4 corner_radii     : CORNERRADII;\n"
    "    float  border_thickness : BORDERTHICKNESS;\n"
    "    uint   elem_type        : ELEMTYPE;\n"
    "};\n"
    "struct PS_INPUT\n"
    "{\n"
    "    float4 pos              : SV_POSITION;\n"
    "    float2 frag_pos         : FRAGPOS;\n"
    "    float2 center           : CENTER;\n"
    "    float2 half_size        : HALFSIZE;\n"
    "    float2 uv               : TEXCOORD0;\n"
    "    float4 col              : COLOR0;\n"
    "    float4 corner_radii     : CORNERRADII;\n"
    "    float  border_thickness : BORDERTHICKNESS;\n"
    "    uint   elem_type        : ELEMTYPE;\n"
    "};\n"
    "PS_INPUT VSMain(VS_INPUT input)\n"
    "{\n"
    "    PS_INPUT output;\n"
    "    output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.0f, 1.0f));\n"
    "    output.frag_pos = input.pos;\n"
    "    output.center = input.center;\n"
    "    output.half_size = input.half_size;\n"
    "    output.uv = input.uv;\n"
    "    output.col = input.col;\n"
    "    output.corner_radii = input.corner_radii;\n"
    "    output.border_thickness = input.border_thickness;\n"
    "    output.elem_type = input.elem_type;\n"
    "    return output;\n"
    "}\n"
    "Texture2D    texture0 : register(t0);\n"
    "SamplerState sampler0 : register(s0);\n"
    "float sdRoundBox(float2 p, float2 b, float4 r)\n"
    "{\n"
    "    float2 s = float2(p.x > 0.0 ? r.y : r.x, p.x > 0.0 ? r.z : r.w);\n"
    "    float corner_r = p.y > 0.0 ? s.y : s.x;\n"
    "    float2 q = abs(p) - b + float2(corner_r, corner_r);\n"
    "    return length(max(q, 0.0f)) + min(max(q.x, q.y), 0.0f) - corner_r;\n"
    "}\n"
    "float median3(float r, float g, float b)\n"
    "{\n"
    "    return max(min(r, g), min(max(r, g), b));\n"
    "}\n"
    "float4 PSMain(PS_INPUT input) : SV_Target\n"
    "{\n"
    "    float4 out_col = input.col;\n"
    "    if (input.elem_type == 0)\n"
    "    {\n"
    "        float d = sdRoundBox(input.frag_pos - input.center, input.half_size, input.corner_radii);\n"
    "        float aa = fwidth(d) * 0.75;\n"
    "        out_col.w *= 1.0 - smoothstep(-aa, aa, d);\n"
    "    }\n"
    "    else if (input.elem_type == 1)\n"
    "    {\n"
    "        float text_alpha = texture0.Sample(sampler0, input.uv).w;\n"
    "        float sharpened_alpha = smoothstep(0.18f, 0.82f, text_alpha);\n"
    "        text_alpha = lerp(text_alpha, sharpened_alpha, 0.35f);\n"
    "        out_col.w *= text_alpha;\n"
    "    }\n"
    "    else if (input.elem_type == 3)\n"
    "    {\n"
    "        float d = sdRoundBox(input.frag_pos - input.center, input.half_size, input.corner_radii);\n"
    "        float bd = abs(d) - input.border_thickness * 0.5;\n"
    "        float aa = fwidth(bd) * 0.75;\n"
    "        out_col.w *= 1.0 - smoothstep(-aa, aa, bd);\n"
    "    }\n"
    "    else if (input.elem_type == 4)\n"
    "    {\n"
    "        float2 pa = input.frag_pos - input.center;\n"
    "        float2 ba = input.half_size - input.center;\n"
    "        float h = saturate(dot(pa, ba) / dot(ba, ba));\n"
    "        float d = length(pa - ba * h) - input.corner_radii.x;\n"
    "        float aa = fwidth(d) * 0.75;\n"
    "        out_col.w *= 1.0 - smoothstep(-aa, aa, d);\n"
    "    }\n"
    "    else\n"
    "    {\n"
    "        out_col.w *= texture0.Sample(sampler0, input.uv).w;\n"
    "    }\n"
    "    return out_col;\n"
    "}\n";

#define BLOOM_D3D11_MAX_TEXTURES 256

typedef struct bloom_d3d11_data
{
    ID3D11Device            *device;
    ID3D11DeviceContext     *ctx;

    ID3D11VertexShader      *vs;
    ID3D11PixelShader       *ps;
    ID3D11InputLayout       *input_layout;
    ID3D11Buffer            *vb;
    ID3D11Buffer            *ib;
    ID3D11Buffer            *cb;
    ID3D11BlendState        *blend_state;
    ID3D11RasterizerState   *raster_state;
    ID3D11DepthStencilState *depth_stencil_state;
    ID3D11SamplerState      *sampler_linear;
    ID3D11SamplerState      *sampler_point;

    bloom_u32                vb_size;
    bloom_u32                ib_size;

    ID3D11ShaderResourceView *white_srv;

    ID3D11ShaderResourceView *srv_table[BLOOM_D3D11_MAX_TEXTURES];
    bloom_u32                 srv_count;
} bloom_d3d11_data;

static ID3D11ShaderResourceView *bloom_d3d11_srv_lookup(bloom_d3d11_data *d, bloom_u32 id)
{
    if (id == 0 || id > d->srv_count) return NULL;
    return d->srv_table[id - 1];
}

static bloom_bool bloom_d3d11_init(bloom_render_backend *backend)
{
    bloom_d3d11_data *d = (bloom_d3d11_data *)backend->user_data;
    HRESULT hr;
    HMODULE d3d_compiler;
    PFN_D3DCompile pfn_compile;
    ID3DBlob *vs_blob = NULL;
    ID3DBlob *ps_blob = NULL;
    ID3DBlob *err_blob = NULL;

    if (!d->device || !d->ctx)
        return BLOOM_FALSE;

    d3d_compiler = LoadLibraryA("d3dcompiler_47.dll");
    if (!d3d_compiler)
        d3d_compiler = LoadLibraryA("d3dcompiler_43.dll");
    if (!d3d_compiler)
        return BLOOM_FALSE;

    pfn_compile = (PFN_D3DCompile)GetProcAddress(d3d_compiler, "D3DCompile");
    if (!pfn_compile) { FreeLibrary(d3d_compiler); return BLOOM_FALSE; }

    hr = pfn_compile(g_bloom_shader_src, sizeof(g_bloom_shader_src) - 1,
                     NULL, NULL, NULL, "VSMain", "vs_4_0", 0, 0, &vs_blob, &err_blob);
    if (err_blob) { ID3D10Blob_Release(err_blob); err_blob = NULL; }
    if (FAILED(hr)) { FreeLibrary(d3d_compiler); return BLOOM_FALSE; }

    hr = pfn_compile(g_bloom_shader_src, sizeof(g_bloom_shader_src) - 1,
                     NULL, NULL, NULL, "PSMain", "ps_4_0", 0, 0, &ps_blob, &err_blob);
    if (err_blob) { ID3D10Blob_Release(err_blob); err_blob = NULL; }
    if (FAILED(hr)) { ID3D10Blob_Release(vs_blob); FreeLibrary(d3d_compiler); return BLOOM_FALSE; }

    hr = ID3D11Device_CreateVertexShader(d->device,
            ID3D10Blob_GetBufferPointer(vs_blob),
            ID3D10Blob_GetBufferSize(vs_blob), NULL, &d->vs);
    if (FAILED(hr)) goto fail_shaders;

    hr = ID3D11Device_CreatePixelShader(d->device,
            ID3D10Blob_GetBufferPointer(ps_blob),
            ID3D10Blob_GetBufferSize(ps_blob), NULL, &d->ps);
    if (FAILED(hr)) goto fail_shaders;

    {
        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION",        0, DXGI_FORMAT_R32G32_FLOAT,   0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "CENTER",          0, DXGI_FORMAT_R32G32_FLOAT,   0, 8,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "HALFSIZE",        0, DXGI_FORMAT_R32G32_FLOAT,   0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",        0, DXGI_FORMAT_R32G32_FLOAT,   0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",           0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "CORNERRADII",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BORDERTHICKNESS", 0, DXGI_FORMAT_R32_FLOAT,      0, 52, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "ELEMTYPE",        0, DXGI_FORMAT_R32_UINT,       0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        hr = ID3D11Device_CreateInputLayout(d->device,
                layout, 8,
                ID3D10Blob_GetBufferPointer(vs_blob),
                ID3D10Blob_GetBufferSize(vs_blob),
                &d->input_layout);
        if (FAILED(hr)) goto fail_shaders;
    }

    ID3D10Blob_Release(vs_blob); vs_blob = NULL;
    ID3D10Blob_Release(ps_blob); ps_blob = NULL;
    FreeLibrary(d3d_compiler);

    {
        D3D11_BUFFER_DESC desc;
        memset(&desc, 0, sizeof(desc));
        desc.ByteWidth = 64;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        hr = ID3D11Device_CreateBuffer(d->device, &desc, NULL, &d->cb);
        if (FAILED(hr)) return BLOOM_FALSE;
    }

    {
        D3D11_BLEND_DESC desc;
        memset(&desc, 0, sizeof(desc));
        desc.AlphaToCoverageEnable = FALSE;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        hr = ID3D11Device_CreateBlendState(d->device, &desc, &d->blend_state);
        if (FAILED(hr)) return BLOOM_FALSE;
    }

    {
        D3D11_RASTERIZER_DESC desc;
        memset(&desc, 0, sizeof(desc));
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_NONE;
        desc.ScissorEnable = TRUE;
        desc.DepthClipEnable = TRUE;
        hr = ID3D11Device_CreateRasterizerState(d->device, &desc, &d->raster_state);
        if (FAILED(hr)) return BLOOM_FALSE;
    }

    {
        D3D11_DEPTH_STENCIL_DESC desc;
        memset(&desc, 0, sizeof(desc));
        desc.DepthEnable = FALSE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        desc.StencilEnable = FALSE;
        hr = ID3D11Device_CreateDepthStencilState(d->device, &desc, &d->depth_stencil_state);
        if (FAILED(hr)) return BLOOM_FALSE;
    }

    {
        D3D11_SAMPLER_DESC desc;
        memset(&desc, 0, sizeof(desc));
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        hr = ID3D11Device_CreateSamplerState(d->device, &desc, &d->sampler_linear);
        if (FAILED(hr)) return BLOOM_FALSE;
    }

    {
        D3D11_SAMPLER_DESC desc;
        memset(&desc, 0, sizeof(desc));
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        hr = ID3D11Device_CreateSamplerState(d->device, &desc, &d->sampler_point);
        if (FAILED(hr)) return BLOOM_FALSE;
    }

    d->vb_size = 0;
    d->ib_size = 0;

    {
        bloom_u8 white_pixel[4] = { 255, 255, 255, 255 };
        D3D11_TEXTURE2D_DESC td;
        D3D11_SUBRESOURCE_DATA sd;
        D3D11_SHADER_RESOURCE_VIEW_DESC svd;
        ID3D11Texture2D *white_tex = NULL;
        memset(&td, 0, sizeof(td));
        td.Width = 1; td.Height = 1; td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        memset(&sd, 0, sizeof(sd));
        sd.pSysMem = white_pixel; sd.SysMemPitch = 4;
        hr = ID3D11Device_CreateTexture2D(d->device, &td, &sd, &white_tex);
        if (SUCCEEDED(hr) && white_tex) {
            memset(&svd, 0, sizeof(svd));
            svd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            svd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            svd.Texture2D.MipLevels = 1;
            ID3D11Device_CreateShaderResourceView(d->device,
                (ID3D11Resource *)white_tex, &svd, &d->white_srv);
            ID3D11Texture2D_Release(white_tex);
        }
    }

    return BLOOM_TRUE;

fail_shaders:
    if (vs_blob) ID3D10Blob_Release(vs_blob);
    if (ps_blob) ID3D10Blob_Release(ps_blob);
    FreeLibrary(d3d_compiler);
    return BLOOM_FALSE;
}

static void bloom_d3d11_shutdown(bloom_render_backend *backend)
{
    bloom_d3d11_data *d = (bloom_d3d11_data *)backend->user_data;
    if (d->white_srv)           { ID3D11ShaderResourceView_Release(d->white_srv);           d->white_srv = NULL; }
    if (d->sampler_point)       { ID3D11SamplerState_Release(d->sampler_point);             d->sampler_point = NULL; }
    if (d->sampler_linear)      { ID3D11SamplerState_Release(d->sampler_linear);            d->sampler_linear = NULL; }
    if (d->depth_stencil_state) { ID3D11DepthStencilState_Release(d->depth_stencil_state);  d->depth_stencil_state = NULL; }
    if (d->raster_state)        { ID3D11RasterizerState_Release(d->raster_state);           d->raster_state = NULL; }
    if (d->blend_state)         { ID3D11BlendState_Release(d->blend_state);                 d->blend_state = NULL; }
    if (d->cb)                  { ID3D11Buffer_Release(d->cb);                              d->cb = NULL; }
    if (d->ib)                  { ID3D11Buffer_Release(d->ib);                              d->ib = NULL; }
    if (d->vb)                  { ID3D11Buffer_Release(d->vb);                              d->vb = NULL; }
    if (d->input_layout)        { ID3D11InputLayout_Release(d->input_layout);               d->input_layout = NULL; }
    if (d->ps)                  { ID3D11PixelShader_Release(d->ps);                         d->ps = NULL; }
    if (d->vs)                  { ID3D11VertexShader_Release(d->vs);                        d->vs = NULL; }
}

static bloom_bool bloom_d3d11_grow_vb(bloom_d3d11_data *d, bloom_u32 needed)
{
    D3D11_BUFFER_DESC desc;
    if (d->vb && d->vb_size >= needed)
        return BLOOM_TRUE;
    if (d->vb) { ID3D11Buffer_Release(d->vb); d->vb = NULL; }
    memset(&desc, 0, sizeof(desc));
    desc.ByteWidth = needed;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(ID3D11Device_CreateBuffer(d->device, &desc, NULL, &d->vb)))
        return BLOOM_FALSE;
    d->vb_size = needed;
    return BLOOM_TRUE;
}

static bloom_bool bloom_d3d11_grow_ib(bloom_d3d11_data *d, bloom_u32 needed)
{
    D3D11_BUFFER_DESC desc;
    if (d->ib && d->ib_size >= needed)
        return BLOOM_TRUE;
    if (d->ib) { ID3D11Buffer_Release(d->ib); d->ib = NULL; }
    memset(&desc, 0, sizeof(desc));
    desc.ByteWidth = needed;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(ID3D11Device_CreateBuffer(d->device, &desc, NULL, &d->ib)))
        return BLOOM_FALSE;
    d->ib_size = needed;
    return BLOOM_TRUE;
}

static void bloom_d3d11_render(bloom_render_backend *backend, bloom_draw_list *dl,
                                bloom_f32 display_w, bloom_f32 display_h)
{
    bloom_d3d11_data *d = (bloom_d3d11_data *)backend->user_data;
    ID3D11DeviceContext *ctx = d->ctx;
    bloom_u32 vb_bytes, ib_bytes, i, idx_offset;
    D3D11_MAPPED_SUBRESOURCE mapped;
    UINT stride, offset_zero;
    D3D11_VIEWPORT vp;
    float blend_factor[4] = { 0, 0, 0, 0 };

    if (!dl || dl->vertex_count == 0 || dl->cmd_count == 0)
        return;

    vb_bytes = dl->vertex_count * (bloom_u32)sizeof(bloom_vertex);
    ib_bytes = dl->index_count  * (bloom_u32)sizeof(bloom_draw_idx);
    if (!bloom_d3d11_grow_vb(d, vb_bytes)) return;
    if (!bloom_d3d11_grow_ib(d, ib_bytes)) return;

    if (FAILED(ID3D11DeviceContext_Map(ctx, (ID3D11Resource *)d->vb, 0,
                                        D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        return;
    memcpy(mapped.pData, dl->vertices, vb_bytes);
    ID3D11DeviceContext_Unmap(ctx, (ID3D11Resource *)d->vb, 0);

    if (FAILED(ID3D11DeviceContext_Map(ctx, (ID3D11Resource *)d->ib, 0,
                                        D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        return;
    memcpy(mapped.pData, dl->indices, ib_bytes);
    ID3D11DeviceContext_Unmap(ctx, (ID3D11Resource *)d->ib, 0);

    {
        float L = 0.0f, R = display_w, T = 0.0f, B = display_h;
        float mvp[16] =
        {
            2.0f / (R - L),     0.0f,               0.0f, 0.0f,
            0.0f,               2.0f / (T - B),     0.0f, 0.0f,
            0.0f,               0.0f,               0.5f, 0.0f,
            (R + L) / (L - R), (T + B) / (B - T),   0.5f, 1.0f,
        };
        if (FAILED(ID3D11DeviceContext_Map(ctx, (ID3D11Resource *)d->cb, 0,
                                            D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
            return;
        memcpy(mapped.pData, mvp, sizeof(mvp));
        ID3D11DeviceContext_Unmap(ctx, (ID3D11Resource *)d->cb, 0);
    }

    stride = (UINT)sizeof(bloom_vertex);
    offset_zero = 0;
    ID3D11DeviceContext_IASetInputLayout(ctx, d->input_layout);
    ID3D11DeviceContext_IASetVertexBuffers(ctx, 0, 1, &d->vb, &stride, &offset_zero);
    ID3D11DeviceContext_IASetIndexBuffer(ctx, d->ib, DXGI_FORMAT_R16_UINT, 0);
    ID3D11DeviceContext_IASetPrimitiveTopology(ctx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D11DeviceContext_VSSetShader(ctx, d->vs, NULL, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(ctx, 0, 1, &d->cb);
    ID3D11DeviceContext_PSSetShader(ctx, d->ps, NULL, 0);

    ID3D11DeviceContext_OMSetBlendState(ctx, d->blend_state, blend_factor, 0xFFFFFFFF);
    ID3D11DeviceContext_OMSetDepthStencilState(ctx, d->depth_stencil_state, 0);
    ID3D11DeviceContext_RSSetState(ctx, d->raster_state);

    memset(&vp, 0, sizeof(vp));
    vp.Width = display_w;
    vp.Height = display_h;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ID3D11DeviceContext_RSSetViewports(ctx, 1, &vp);

    idx_offset = 0;
    for (i = 0; i < dl->cmd_count; i++)
    {
        bloom_draw_cmd *cmd = &dl->commands[i];
        D3D11_RECT scissor;
        ID3D11SamplerState *sampler;

        if (cmd->clip_rect.w > 0.0f && cmd->clip_rect.h > 0.0f)
        {
            scissor.left   = (LONG)cmd->clip_rect.x;
            scissor.top    = (LONG)cmd->clip_rect.y;
            scissor.right  = (LONG)(cmd->clip_rect.x + cmd->clip_rect.w);
            scissor.bottom = (LONG)(cmd->clip_rect.y + cmd->clip_rect.h);
        }
        else if (cmd->clip_rect.x < 0.0f && cmd->clip_rect.y < 0.0f)
        {
            /* no-clip sentinel {-1,-1,-1,-1}: full-screen scissor */
            scissor.left   = 0;
            scissor.top    = 0;
            scissor.right  = (LONG)display_w;
            scissor.bottom = (LONG)display_h;
        }
        else
        {
            /* degenerate clip (w<=0 or h<=0): completely clipped, skip draw */
            idx_offset += cmd->elem_count;
            continue;
        }
        ID3D11DeviceContext_RSSetScissorRects(ctx, 1, &scissor);

        if (cmd->texture_id != 0)
        {
            ID3D11ShaderResourceView *srv = bloom_d3d11_srv_lookup(d, cmd->texture_id);
            sampler = d->sampler_linear;
            ID3D11DeviceContext_PSSetShaderResources(ctx, 0, 1, &srv);
            ID3D11DeviceContext_PSSetSamplers(ctx, 0, 1, &sampler);
        }
        else
        {
            sampler = d->sampler_point;
            ID3D11DeviceContext_PSSetShaderResources(ctx, 0, 1, &d->white_srv);
            ID3D11DeviceContext_PSSetSamplers(ctx, 0, 1, &sampler);
        }

        ID3D11DeviceContext_DrawIndexed(ctx, cmd->elem_count, idx_offset, 0);
        idx_offset += cmd->elem_count;
    }
}

static bloom_u32 bloom_d3d11_create_texture(bloom_render_backend *backend,
                                             bloom_u32 width, bloom_u32 height,
                                             const bloom_u8 *pixels)
{
    bloom_d3d11_data *d = (bloom_d3d11_data *)backend->user_data;
    ID3D11Texture2D *tex = NULL;
    ID3D11ShaderResourceView *srv = NULL;
    D3D11_TEXTURE2D_DESC tex_desc;
    D3D11_SUBRESOURCE_DATA init_data;
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    bloom_u8 *rgba;
    bloom_u32 j;

    rgba = (bloom_u8 *)malloc(width * height * 4);
    if (!rgba) return 0;
    for (j = 0; j < width * height; j++)
    {
        rgba[j * 4 + 0] = 255;
        rgba[j * 4 + 1] = 255;
        rgba[j * 4 + 2] = 255;
        rgba[j * 4 + 3] = pixels[j];
    }

    memset(&tex_desc, 0, sizeof(tex_desc));
    tex_desc.Width = width;
    tex_desc.Height = height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    memset(&init_data, 0, sizeof(init_data));
    init_data.pSysMem = rgba;
    init_data.SysMemPitch = width * 4;

    if (FAILED(ID3D11Device_CreateTexture2D(d->device, &tex_desc, &init_data, &tex)))
    {
        free(rgba);
        return 0;
    }
    free(rgba);

    memset(&srv_desc, 0, sizeof(srv_desc));
    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = 1;

    if (FAILED(ID3D11Device_CreateShaderResourceView(d->device,
                (ID3D11Resource *)tex, &srv_desc, &srv)))
    {
        ID3D11Texture2D_Release(tex);
        return 0;
    }

    ID3D11Texture2D_Release(tex);

    if (d->srv_count >= BLOOM_D3D11_MAX_TEXTURES)
    {
        ID3D11ShaderResourceView_Release(srv);
        return 0;
    }
    d->srv_table[d->srv_count] = srv;
    d->srv_count++;
    return d->srv_count;
}

static void bloom_d3d11_destroy_texture(bloom_render_backend *backend, bloom_u32 texture_id)
{
    bloom_d3d11_data *d = (bloom_d3d11_data *)backend->user_data;
    if (texture_id != 0 && texture_id <= d->srv_count)
    {
        ID3D11ShaderResourceView *srv = d->srv_table[texture_id - 1];
        if (srv) ID3D11ShaderResourceView_Release(srv);
        d->srv_table[texture_id - 1] = NULL;
    }
}

bloom_render_backend *bloom_create_d3d11_backend(void *device, void *device_ctx)
{
    bloom_render_backend *backend;
    bloom_d3d11_data *data;

    if (!device || !device_ctx)
        return NULL;

    backend = (bloom_render_backend *)malloc(sizeof(bloom_render_backend));
    data = (bloom_d3d11_data *)calloc(1, sizeof(bloom_d3d11_data));
    if (!backend || !data)
    {
        free(backend);
        free(data);
        return NULL;
    }

    data->device = (ID3D11Device *)device;
    data->ctx = (ID3D11DeviceContext *)device_ctx;

    backend->init = bloom_d3d11_init;
    backend->shutdown = bloom_d3d11_shutdown;
    backend->render = bloom_d3d11_render;
    backend->create_texture = bloom_d3d11_create_texture;
    backend->destroy_texture = bloom_d3d11_destroy_texture;
    backend->user_data = data;

    return backend;
}

void bloom_destroy_d3d11_backend(bloom_render_backend *backend)
{
    if (backend)
    {
        backend->shutdown(backend);
        free(backend->user_data);
        free(backend);
    }
}

#endif

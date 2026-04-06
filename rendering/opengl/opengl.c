#ifndef BLOOM_INTERNAL_BUILD
#define BLOOM_INTERNAL_BUILD
#endif

#include "rendering/api.h"
#include "core/runtime/context/context.h"

#ifdef BLOOM_OPENGL_BACKEND

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif
#include <GL/gl.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STREAM_DRAW                    0x88E0
#define GL_TEXTURE0                       0x84C0

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

typedef GLuint (APIENTRY *PFNGLCREATESHADERPROC)(GLenum);
typedef void   (APIENTRY *PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const GLchar **, const GLint *);
typedef void   (APIENTRY *PFNGLCOMPILESHADERPROC)(GLuint);
typedef void   (APIENTRY *PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint *);
typedef void   (APIENTRY *PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei *, GLchar *);
typedef GLuint (APIENTRY *PFNGLCREATEPROGRAMPROC)(void);
typedef void   (APIENTRY *PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void   (APIENTRY *PFNGLLINKPROGRAMPROC)(GLuint);
typedef void   (APIENTRY *PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint *);
typedef void   (APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint);
typedef void   (APIENTRY *PFNGLDELETESHADERPROC)(GLuint);
typedef void   (APIENTRY *PFNGLDELETEPROGRAMPROC)(GLuint);
typedef GLint  (APIENTRY *PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const GLchar *);
typedef void   (APIENTRY *PFNGLUNIFORMMATRIX4FVPROC)(GLint, GLsizei, GLboolean, const GLfloat *);
typedef void   (APIENTRY *PFNGLUNIFORM1IPROC)(GLint, GLint);
typedef void   (APIENTRY *PFNGLGENVERTEXARRAYSPROC)(GLsizei, GLuint *);
typedef void   (APIENTRY *PFNGLBINDVERTEXARRAYPROC)(GLuint);
typedef void   (APIENTRY *PFNGLDELETEVERTEXARRAYSPROC)(GLsizei, const GLuint *);
typedef void   (APIENTRY *PFNGLGENBUFFERSPROC)(GLsizei, GLuint *);
typedef void   (APIENTRY *PFNGLBINDBUFFERPROC)(GLenum, GLuint);
typedef void   (APIENTRY *PFNGLBUFFERDATAPROC)(GLenum, GLsizeiptr, const void *, GLenum);
typedef void   (APIENTRY *PFNGLDELETEBUFFERSPROC)(GLsizei, const GLuint *);
typedef void   (APIENTRY *PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint);
typedef void   (APIENTRY *PFNGLVERTEXATTRIBPOINTERPROC)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void *);
typedef void   (APIENTRY *PFNGLVERTEXATTRIBIPOINTERPROC)(GLuint, GLint, GLenum, GLsizei, const void *);
typedef void   (APIENTRY *PFNGLACTIVETEXTUREPROC)(GLenum);

static PFNGLCREATESHADERPROC           pglCreateShader;
static PFNGLSHADERSOURCEPROC           pglShaderSource;
static PFNGLCOMPILESHADERPROC          pglCompileShader;
static PFNGLGETSHADERIVPROC            pglGetShaderiv;
static PFNGLGETSHADERINFOLOGPROC       pglGetShaderInfoLog;
static PFNGLCREATEPROGRAMPROC          pglCreateProgram;
static PFNGLATTACHSHADERPROC           pglAttachShader;
static PFNGLLINKPROGRAMPROC            pglLinkProgram;
static PFNGLGETPROGRAMIVPROC           pglGetProgramiv;
static PFNGLUSEPROGRAMPROC             pglUseProgram;
static PFNGLDELETESHADERPROC           pglDeleteShader;
static PFNGLDELETEPROGRAMPROC          pglDeleteProgram;
static PFNGLGETUNIFORMLOCATIONPROC     pglGetUniformLocation;
static PFNGLUNIFORMMATRIX4FVPROC       pglUniformMatrix4fv;
static PFNGLUNIFORM1IPROC             pglUniform1i;
static PFNGLGENVERTEXARRAYSPROC        pglGenVertexArrays;
static PFNGLBINDVERTEXARRAYPROC        pglBindVertexArray;
static PFNGLDELETEVERTEXARRAYSPROC     pglDeleteVertexArrays;
static PFNGLGENBUFFERSPROC             pglGenBuffers;
static PFNGLBINDBUFFERPROC             pglBindBuffer;
static PFNGLBUFFERDATAPROC             pglBufferData;
static PFNGLDELETEBUFFERSPROC          pglDeleteBuffers;
static PFNGLENABLEVERTEXATTRIBARRAYPROC pglEnableVertexAttribArray;
static PFNGLVERTEXATTRIBPOINTERPROC    pglVertexAttribPointer;
static PFNGLVERTEXATTRIBIPOINTERPROC   pglVertexAttribIPointer;
static PFNGLACTIVETEXTUREPROC          pglActiveTexture;

static bloom_bool bloom_gl_load_functions(void)
{
#ifdef _WIN32
    #define BLOOM_GL_LOAD(name, type) p##name = (type)wglGetProcAddress(#name); if (!p##name) return BLOOM_FALSE
#else
    #define BLOOM_GL_LOAD(name, type) return BLOOM_FALSE
#endif
    BLOOM_GL_LOAD(glCreateShader, PFNGLCREATESHADERPROC);
    BLOOM_GL_LOAD(glShaderSource, PFNGLSHADERSOURCEPROC);
    BLOOM_GL_LOAD(glCompileShader, PFNGLCOMPILESHADERPROC);
    BLOOM_GL_LOAD(glGetShaderiv, PFNGLGETSHADERIVPROC);
    BLOOM_GL_LOAD(glGetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC);
    BLOOM_GL_LOAD(glCreateProgram, PFNGLCREATEPROGRAMPROC);
    BLOOM_GL_LOAD(glAttachShader, PFNGLATTACHSHADERPROC);
    BLOOM_GL_LOAD(glLinkProgram, PFNGLLINKPROGRAMPROC);
    BLOOM_GL_LOAD(glGetProgramiv, PFNGLGETPROGRAMIVPROC);
    BLOOM_GL_LOAD(glUseProgram, PFNGLUSEPROGRAMPROC);
    BLOOM_GL_LOAD(glDeleteShader, PFNGLDELETESHADERPROC);
    BLOOM_GL_LOAD(glDeleteProgram, PFNGLDELETEPROGRAMPROC);
    BLOOM_GL_LOAD(glGetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC);
    BLOOM_GL_LOAD(glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FVPROC);
    BLOOM_GL_LOAD(glUniform1i, PFNGLUNIFORM1IPROC);
    BLOOM_GL_LOAD(glGenVertexArrays, PFNGLGENVERTEXARRAYSPROC);
    BLOOM_GL_LOAD(glBindVertexArray, PFNGLBINDVERTEXARRAYPROC);
    BLOOM_GL_LOAD(glDeleteVertexArrays, PFNGLDELETEVERTEXARRAYSPROC);
    BLOOM_GL_LOAD(glGenBuffers, PFNGLGENBUFFERSPROC);
    BLOOM_GL_LOAD(glBindBuffer, PFNGLBINDBUFFERPROC);
    BLOOM_GL_LOAD(glBufferData, PFNGLBUFFERDATAPROC);
    BLOOM_GL_LOAD(glDeleteBuffers, PFNGLDELETEBUFFERSPROC);
    BLOOM_GL_LOAD(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC);
    BLOOM_GL_LOAD(glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC);
    BLOOM_GL_LOAD(glVertexAttribIPointer, PFNGLVERTEXATTRIBIPOINTERPROC);
    BLOOM_GL_LOAD(glActiveTexture, PFNGLACTIVETEXTUREPROC);
    #undef BLOOM_GL_LOAD
    return BLOOM_TRUE;
}

static const char g_bloom_gl_vs[] =
    "#version 330 core\n"
    "layout(location = 0) in vec2 a_pos;\n"
    "layout(location = 1) in vec2 a_center;\n"
    "layout(location = 2) in vec2 a_half_size;\n"
    "layout(location = 3) in vec2 a_uv;\n"
    "layout(location = 4) in vec4 a_col;\n"
    "layout(location = 5) in float a_corner_radius;\n"
    "layout(location = 6) in float a_border_thickness;\n"
    "layout(location = 7) in uint a_elem_type;\n"
    "uniform mat4 u_proj;\n"
    "out vec2 v_frag_pos;\n"
    "out vec2 v_center;\n"
    "out vec2 v_half_size;\n"
    "out vec2 v_uv;\n"
    "out vec4 v_col;\n"
    "out float v_corner_radius;\n"
    "out float v_border_thickness;\n"
    "flat out uint v_elem_type;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = u_proj * vec4(a_pos, 0.0, 1.0);\n"
    "    v_frag_pos = a_pos;\n"
    "    v_center = a_center;\n"
    "    v_half_size = a_half_size;\n"
    "    v_uv = a_uv;\n"
    "    v_col = a_col;\n"
    "    v_corner_radius = a_corner_radius;\n"
    "    v_border_thickness = a_border_thickness;\n"
    "    v_elem_type = a_elem_type;\n"
    "}\n";

static const char g_bloom_gl_fs[] =
    "#version 330 core\n"
    "in vec2 v_frag_pos;\n"
    "in vec2 v_center;\n"
    "in vec2 v_half_size;\n"
    "in vec2 v_uv;\n"
    "in vec4 v_col;\n"
    "in float v_corner_radius;\n"
    "in float v_border_thickness;\n"
    "flat in uint v_elem_type;\n"
    "uniform sampler2D u_texture;\n"
    "out vec4 frag_color;\n"
    "float sdRoundBox(vec2 p, vec2 b, float r)\n"
    "{\n"
    "    vec2 q = abs(p) - b + vec2(r);\n"
    "    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;\n"
    "}\n"
    "float median3(float r, float g, float b)\n"
    "{\n"
    "    return max(min(r, g), min(max(r, g), b));\n"
    "}\n"
    "void main()\n"
    "{\n"
    "    vec4 out_col = v_col;\n"
    "    if (v_elem_type == 0u)\n"
    "    {\n"
    "        float d = sdRoundBox(v_frag_pos - v_center, v_half_size, v_corner_radius);\n"
    "        float aa = fwidth(d) * 0.75;\n"
    "        out_col.w *= 1.0 - smoothstep(-aa, aa, d);\n"
    "    }\n"
    "    else if (v_elem_type == 1u)\n"
    "    {\n"
    "        out_col.w *= texture(u_texture, v_uv).w;\n"
    "    }\n"
    "    else if (v_elem_type == 3u)\n"
    "    {\n"
    "        float d = sdRoundBox(v_frag_pos - v_center, v_half_size, v_corner_radius);\n"
    "        float bd = abs(d) - v_border_thickness * 0.5;\n"
    "        float aa = fwidth(bd) * 0.75;\n"
    "        out_col.w *= 1.0 - smoothstep(-aa, aa, bd);\n"
    "    }\n"
    "    else\n"
    "    {\n"
    "        out_col.w *= texture(u_texture, v_uv).w;\n"
    "    }\n"
    "    frag_color = out_col;\n"
    "}\n";

typedef struct bloom_gl_data
{
    GLuint program;
    GLuint vao;
    GLuint vbo;
    GLuint ibo;
    GLint  u_proj;
    GLint  u_texture;
    bloom_bool ready;
} bloom_gl_data;

static GLuint bloom_gl_compile_shader(GLenum type, const char *src)
{
    GLuint s = pglCreateShader(type);
    GLint ok = 0;
    pglShaderSource(s, 1, &src, NULL);
    pglCompileShader(s);
    pglGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        pglDeleteShader(s);
        return 0;
    }
    return s;
}

static void bloom_gl_apply_scissor(bloom_rect clip_rect, bloom_f32 display_w, bloom_f32 display_h)
{
    bloom_i32 left;
    bloom_i32 top;
    bloom_i32 right;
    bloom_i32 bottom;
    bloom_i32 width;
    bloom_i32 height;

    if (clip_rect.w <= 0.0f || clip_rect.h <= 0.0f)
    {
        if (clip_rect.x < 0.0f && clip_rect.y < 0.0f)
        {
            /* no-clip sentinel {-1,-1,-1,-1}: full-screen scissor */
            glScissor(0, 0, (int)display_w, (int)display_h);
        }
        else
        {
            /* zero-area intersection: widget scrolled off window, clip everything */
            glScissor(0, 0, 0, 0);
        }
        return;
    }

    left = (bloom_i32)floorf(clip_rect.x);
    top = (bloom_i32)floorf(clip_rect.y);
    right = (bloom_i32)ceilf(clip_rect.x + clip_rect.w);
    bottom = (bloom_i32)ceilf(clip_rect.y + clip_rect.h);

    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > (bloom_i32)display_w) right = (bloom_i32)display_w;
    if (bottom > (bloom_i32)display_h) bottom = (bloom_i32)display_h;

    width = right - left;
    height = bottom - top;
    if (width <= 0 || height <= 0)
    {
        glScissor(0, 0, 0, 0);
        return;
    }

    glScissor(left,
              (int)display_h - bottom,
              width,
              height);
}

static bloom_bool bloom_gl_init(bloom_render_backend *backend)
{
    bloom_gl_data *data = (bloom_gl_data *)backend->user_data;
    GLuint vs, fs;
    GLint ok = 0;

    if (!bloom_gl_load_functions())
        return BLOOM_FALSE;

    vs = bloom_gl_compile_shader(GL_VERTEX_SHADER, g_bloom_gl_vs);
    if (!vs) return BLOOM_FALSE;
    fs = bloom_gl_compile_shader(GL_FRAGMENT_SHADER, g_bloom_gl_fs);
    if (!fs) { pglDeleteShader(vs); return BLOOM_FALSE; }

    data->program = pglCreateProgram();
    pglAttachShader(data->program, vs);
    pglAttachShader(data->program, fs);
    pglLinkProgram(data->program);
    pglGetProgramiv(data->program, GL_LINK_STATUS, &ok);
    pglDeleteShader(vs);
    pglDeleteShader(fs);
    if (!ok)
    {
        pglDeleteProgram(data->program);
        data->program = 0;
        return BLOOM_FALSE;
    }

    data->u_proj = pglGetUniformLocation(data->program, "u_proj");
    data->u_texture = pglGetUniformLocation(data->program, "u_texture");

    pglGenVertexArrays(1, &data->vao);
    pglGenBuffers(1, &data->vbo);
    pglGenBuffers(1, &data->ibo);

    pglBindVertexArray(data->vao);
    pglBindBuffer(GL_ARRAY_BUFFER, data->vbo);
    pglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data->ibo);

    pglEnableVertexAttribArray(0);
    pglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(bloom_vertex), (void *)0);

    pglEnableVertexAttribArray(1);
    pglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(bloom_vertex), (void *)8);

    pglEnableVertexAttribArray(2);
    pglVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(bloom_vertex), (void *)16);

    pglEnableVertexAttribArray(3);
    pglVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(bloom_vertex), (void *)24);

    pglEnableVertexAttribArray(4);
    pglVertexAttribPointer(4, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(bloom_vertex), (void *)32);

    pglEnableVertexAttribArray(5);
    pglVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(bloom_vertex), (void *)36);

    pglEnableVertexAttribArray(6);
    pglVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(bloom_vertex), (void *)40);

    pglEnableVertexAttribArray(7);
    pglVertexAttribIPointer(7, 1, GL_UNSIGNED_INT, sizeof(bloom_vertex), (void *)44);

    pglBindVertexArray(0);
    data->ready = BLOOM_TRUE;
    return BLOOM_TRUE;
}

static void bloom_gl_shutdown(bloom_render_backend *backend)
{
    bloom_gl_data *data = (bloom_gl_data *)backend->user_data;
    if (data->ibo) { pglDeleteBuffers(1, &data->ibo); data->ibo = 0; }
    if (data->vbo) { pglDeleteBuffers(1, &data->vbo); data->vbo = 0; }
    if (data->vao) { pglDeleteVertexArrays(1, &data->vao); data->vao = 0; }
    if (data->program) { pglDeleteProgram(data->program); data->program = 0; }
    data->ready = BLOOM_FALSE;
}

static void bloom_gl_render(bloom_render_backend *backend, bloom_draw_list *dl,
                             bloom_f32 display_w, bloom_f32 display_h)
{
    bloom_gl_data *data = (bloom_gl_data *)backend->user_data;
    bloom_u32 idx_offset;
    bloom_u32 i;

    if (!data->ready || !dl || dl->vertex_count == 0 || dl->cmd_count == 0)
        return;

    glViewport(0, 0, (int)display_w, (int)display_h);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_SCISSOR_TEST);

    pglUseProgram(data->program);

    {
        float L = 0.0f, R = display_w, T = 0.0f, B = display_h;
        float mvp[16] =
        {
            2.0f / (R - L),     0.0f,               0.0f, 0.0f,
            0.0f,               2.0f / (T - B),     0.0f, 0.0f,
            0.0f,               0.0f,              -1.0f, 0.0f,
            (R + L) / (L - R), (T + B) / (B - T),   0.0f, 1.0f,
        };
        pglUniformMatrix4fv(data->u_proj, 1, GL_FALSE, mvp);
    }

    pglActiveTexture(GL_TEXTURE0);
    pglUniform1i(data->u_texture, 0);

    pglBindVertexArray(data->vao);
    pglBindBuffer(GL_ARRAY_BUFFER, data->vbo);
    pglBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(dl->vertex_count * sizeof(bloom_vertex)), dl->vertices, GL_STREAM_DRAW);
    pglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data->ibo);
    pglBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(dl->index_count * sizeof(bloom_draw_idx)), dl->indices, GL_STREAM_DRAW);

    idx_offset = 0;
    for (i = 0; i < dl->cmd_count; i++)
    {
        bloom_draw_cmd *cmd = &dl->commands[i];

        bloom_gl_apply_scissor(cmd->clip_rect, display_w, display_h);

        if (cmd->texture_id != 0)
        {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, cmd->texture_id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glDrawElements(GL_TRIANGLES, (int)cmd->elem_count,
                       GL_UNSIGNED_SHORT, (void *)(uintptr_t)(idx_offset * sizeof(bloom_draw_idx)));
        idx_offset += cmd->elem_count;
    }

    pglBindVertexArray(0);
    pglUseProgram(0);

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
}

static bloom_u32 bloom_gl_create_texture(bloom_render_backend *backend,
                                          bloom_u32 width, bloom_u32 height,
                                          const bloom_u8 *pixels)
{
    (void)backend;
    GLuint tex;
    bloom_u8 *rgba;

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    rgba = (bloom_u8 *)malloc(width * height * 4);
    if (rgba)
    {
        bloom_u32 j;
        for (j = 0; j < width * height; j++)
        {
            rgba[j * 4 + 0] = 255;
            rgba[j * 4 + 1] = 255;
            rgba[j * 4 + 2] = 255;
            rgba[j * 4 + 3] = pixels[j];
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)width, (int)height,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        free(rgba);
    }

    return (bloom_u32)tex;
}

static void bloom_gl_destroy_texture(bloom_render_backend *backend, bloom_u32 texture_id)
{
    (void)backend;
    if (texture_id)
    {
        GLuint tex = (GLuint)texture_id;
        glDeleteTextures(1, &tex);
    }
}

bloom_render_backend *bloom_create_opengl_backend(void)
{
    bloom_render_backend *backend;
    bloom_gl_data *data;

    backend = (bloom_render_backend *)malloc(sizeof(bloom_render_backend));
    data = (bloom_gl_data *)calloc(1, sizeof(bloom_gl_data));
    if (!backend || !data)
    {
        free(backend);
        free(data);
        return NULL;
    }

    backend->init = bloom_gl_init;
    backend->shutdown = bloom_gl_shutdown;
    backend->render = bloom_gl_render;
    backend->create_texture = bloom_gl_create_texture;
    backend->destroy_texture = bloom_gl_destroy_texture;
    backend->user_data = data;

    return backend;
}

void bloom_destroy_opengl_backend(bloom_render_backend *backend)
{
    if (backend)
    {
        backend->shutdown(backend);
        free(backend->user_data);
        free(backend);
    }
}

#endif

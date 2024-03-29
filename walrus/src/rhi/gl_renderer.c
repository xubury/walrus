#include "rhi_p.h"
#include "gl_shader.h"
#include "gl_texture.h"
#include "gl_framebuffer.h"
#include "frame.h"

#include <core/macro.h>
#include <core/log.h>
#include <core/memory.h>
#include <core/string.h>
#include <core/math.h>
#include <core/assert.h>

#include <string.h>
#include <cglm/cglm.h>
#include <cglm/io.h>

GLenum glew_init(void);

void wajs_setup_gl_context(void);

GlRenderer *gl_renderer = NULL;

typedef struct {
    u32 draw_calls;
    u32 compute_calls;
    u32 num_vertices;
    u32 num_indices;
    u32 num_instance;
} RenderStats;

static void clean_stats(RenderStats *stats)
{
    stats->draw_calls    = 0;
    stats->compute_calls = 0;
    stats->num_vertices  = 0;
    stats->num_indices   = 0;
    stats->num_instance  = 0;
}

static void output_stats(RenderStats *stats)
{
    walrus_trace("compute calls: %d draw calls: %d num vertices: %d num indices: %d num instance: %d",
                 stats->compute_calls, stats->draw_calls, stats->num_vertices, stats->num_indices, stats->num_instance);
}

static GLenum const s_attribute_type[WR_RHI_COMPONENT_COUNT] = {
    GL_BYTE,            // Int8
    GL_UNSIGNED_BYTE,   // Uint8
    GL_SHORT,           // Int16
    GL_UNSIGNED_SHORT,  // Uint16
    GL_INT,             // Int32
    GL_UNSIGNED_INT,    // Uint32
    GL_FLOAT,           // Float
};

static GLenum const s_access[] = {
    GL_READ_ONLY,
    GL_WRITE_ONLY,
    GL_READ_WRITE,
};

static GLenum const s_image_format[WR_RHI_FORMAT_COUNT] = {
    GL_ALPHA,  // A8

    GL_R8,        // R8
    GL_R8_SNORM,  // R8S
    GL_R32I,      // R32I
    GL_R32UI,     // R32U
    GL_R16F,      // R16F
    GL_R32F,      // R32F

    GL_RG8,        // RG8
    GL_RG8_SNORM,  // RG8S
    GL_RG32I,      // RG32I
    GL_RG32UI,     // RG32U
    GL_RG16F,      // RG16F
    GL_RG32F,      // RG32F

    GL_RGB8,        // RGB8
    GL_RGB8_SNORM,  // RG8S
    GL_RGB32I,      // RG32I
    GL_RGB32UI,     // RG32U
    GL_RGB16F,      // RG16F
    GL_RGB32F,      // RG32F

    GL_RGBA8,        // BGRA8
    GL_RGBA8_SNORM,  // RGBA8S
    GL_RGBA32I,      // RGBA32I
    GL_RGBA32UI,     // RGBA32U
    GL_RGBA16F,      // RGBA16F
    GL_RGBA32F,      // RGBA32F

    GL_ZERO,  // Depth24
    GL_ZERO,  // Stencil8
    GL_ZERO,  // Depth24Stencil8
};

typedef struct {
    GLenum src;
    GLenum dst;
    bool   is_factor;
} Blend;

static Blend const s_blend[] = {
    {0, 0, false},                                                     // ignored
    {GL_ZERO, GL_ZERO, false},                                         // ZERO
    {GL_ONE, GL_ONE, false},                                           // ONE
    {GL_SRC_COLOR, GL_SRC_COLOR, false},                               // SRC_COLOR
    {GL_ONE_MINUS_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, false},           // INV_SRC_COLOR
    {GL_SRC_ALPHA, GL_SRC_ALPHA, false},                               // SRC_ALPHA
    {GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, false},           // INV_SRC_ALPHA
    {GL_DST_ALPHA, GL_DST_ALPHA, false},                               // DST_ALPHA
    {GL_ONE_MINUS_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, false},           // INV_DST_ALPHA
    {GL_DST_COLOR, GL_DST_COLOR, false},                               // DST_COLOR
    {GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_DST_COLOR, false},           // INV_DST_COLOR
    {GL_SRC_ALPHA_SATURATE, GL_ONE, false},                            // SRC_ALPHA_SAT
    {GL_CONSTANT_COLOR, GL_CONSTANT_COLOR, true},                      // FACTOR
    {GL_ONE_MINUS_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_COLOR, true},  // INV_FACTOR
};

static GLenum const s_cmp_func[] = {
    0,            // ignored
    GL_LESS,      // TEST_LESS
    GL_LEQUAL,    // TEST_LEQUAL
    GL_EQUAL,     // TEST_EQUAL
    GL_GEQUAL,    // TEST_GEQUAL
    GL_GREATER,   // TEST_GREATER
    GL_NOTEQUAL,  // TEST_NOTEQUAL
    GL_NEVER,     // TEST_NEVER
    GL_ALWAYS,    // TEST_ALWAYS
};

static GLenum const s_cmpfunc[] = {
    0,            // ignored
    GL_LESS,      // TEST_LESS
    GL_LEQUAL,    // TEST_LEQUAL
    GL_EQUAL,     // TEST_EQUAL
    GL_GEQUAL,    // TEST_GEQUAL
    GL_GREATER,   // TEST_GREATER
    GL_NOTEQUAL,  // TEST_NOTEQUAL
    GL_NEVER,     // TEST_NEVER
    GL_ALWAYS,    // TEST_ALWAYS
};

static GLenum const s_stencilface[] = {
    GL_FRONT_AND_BACK,
    GL_FRONT,
    GL_BACK,
};

static GLenum const s_stencilop[] = {
    GL_ZERO, GL_KEEP, GL_REPLACE, GL_INCR_WRAP, GL_INCR, GL_DECR_WRAP, GL_DECR, GL_INVERT,
};

static GLenum const s_primitives[] = {GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_LINES};

static u64 s_vao_current_enabled = 0;
static u64 s_vao_pending_disable = 0;
static u64 s_vao_pending_enable  = 0;

void lazy_enable_vertex_attribute(GLuint index)
{
    u64 const mask = UINT64_C(1) << index;
    s_vao_pending_enable |= mask & (~s_vao_current_enabled);
    s_vao_pending_disable &= ~mask;
}

void lazy_disable_vertex_attribute(GLuint index)
{
    u64 const mask = UINT64_C(1) << index;
    s_vao_pending_disable |= mask & s_vao_current_enabled;
    s_vao_pending_enable &= ~mask;
}

void apply_lazy_enabled_vertex_attribute(void)
{
    while (s_vao_pending_disable) {
        u32 index = walrus_u32cnttz(s_vao_pending_disable);
        u64 mask  = ~(UINT64_C(1) << index);
        s_vao_pending_disable &= mask;
        s_vao_current_enabled &= mask;
        glDisableVertexAttribArray(index);
    }

    while (s_vao_pending_enable) {
        u32 index = walrus_u32cnttz(s_vao_pending_enable);
        u64 mask  = UINT64_C(1) << index;
        s_vao_pending_enable &= ~mask;
        s_vao_current_enabled |= mask;
        glEnableVertexAttribArray(index);
    }
}

static void commit(GlProgram const *program)
{
    UniformBuffer *buffer = program->buffer;
    if (buffer == NULL) {
        return;
    }

    uniform_buffer_start(buffer, 0);

    while (true) {
        u64 op = uniform_buffer_read_value(buffer);
        if (op == UNIFORM_BUFFER_END) {
            break;
        }

        Walrus_UniformHandle handle;
        memcpy(&handle, uniform_buffer_read(buffer, sizeof(Walrus_UniformHandle)), sizeof(Walrus_UniformHandle));
        void *data = gl_renderer->uniforms[handle.id];

        Walrus_UniformType type;
        u32                loc;
        u8                 num;
        uniform_decode_op(&type, &loc, &num, op);
        switch (type) {
            case WR_RHI_UNIFORM_BOOL:
            case WR_RHI_UNIFORM_UINT: {
                if (num > 1) {
                    glUniform1uiv(loc, num, (u32 const *)data);
                }
                else {
                    glUniform1ui(loc, *(u32 const *)data);
                }
            } break;
            case WR_RHI_UNIFORM_INT: {
                if (num > 1) {
                    glUniform1iv(loc, num, (i32 const *)data);
                }
                else {
                    glUniform1i(loc, *(i32 const *)data);
                }
            } break;
            case WR_RHI_UNIFORM_FLOAT: {
                glUniform1fv(loc, num, (f32 const *)data);
            } break;
            case WR_RHI_UNIFORM_VEC2: {
                glUniform2fv(loc, num, (f32 const *)data);
            } break;
            case WR_RHI_UNIFORM_VEC3: {
                glUniform3fv(loc, num, (f32 const *)data);
            } break;
            case WR_RHI_UNIFORM_VEC4: {
                glUniform4fv(loc, num, (f32 const *)data);
            } break;
            case WR_RHI_UNIFORM_MAT3: {
                glUniformMatrix3fv(loc, num, GL_FALSE, (f32 const *)data);
            } break;
            case WR_RHI_UNIFORM_MAT4: {
                glUniformMatrix4fv(loc, num, GL_FALSE, (f32 const *)data);
            } break;
            case WR_RHI_UNIFORM_SAMPLER: {
                if (num > 1) {
                    glUniform1iv(loc, num, (i32 const *)data);
                }
                else {
                    glUniform1i(loc, *(i32 const *)data);
                }
            } break;
            case WR_RHI_UNIFORM_COUNT:
                break;
        }
    }
}

static void set_predefineds(GlProgram const *prog, RenderFrame const *frame, RenderView const *view, u32 start_matrix,
                            u32 num_matrices)
{
    for (u8 i = 0; i < prog->num_predefineds; ++i) {
        PredefinedUniform const *u = &prog->predefineds[i];
        switch (u->type) {
            case PREDEFINED_VIEW:
                glUniformMatrix4fv(u->loc, 1, GL_FALSE, &view->view[0][0]);
                break;
            case PREDEFINED_VIEWPROJ: {
                mat4 viewproj;
                glm_mat4_mul((vec4 *)view->projection, (vec4 *)view->view, viewproj);
                glUniformMatrix4fv(u->loc, 1, GL_FALSE, &viewproj[0][0]);
            } break;
            case PREDEFINED_PROJECTION:
                glUniformMatrix4fv(u->loc, 1, GL_FALSE, &view->projection[0][0]);
                break;
            case PREDEFINED_MODEL:
                glUniformMatrix4fv(u->loc, num_matrices, GL_FALSE, &frame->matrix_cache[start_matrix][0][0]);
                break;
            case PREDEFINED_COUNT:
                break;
        }
    }
}

static void create_msaa_fbo(u32 width, u32 height, u8 msaa)
{
    if (gl_renderer->msaa_fbo == 0 && msaa > 1) {
        glGenFramebuffers(1, &gl_renderer->msaa_fbo);
        glGenRenderbuffers(2, gl_renderer->msaa_rbos);

        glBindFramebuffer(GL_FRAMEBUFFER, gl_renderer->msaa_fbo);
        glBindRenderbuffer(GL_RENDERBUFFER, gl_renderer->msaa_rbos[0]);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa, GL_RGBA8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, gl_renderer->msaa_rbos[0]);

        glBindRenderbuffer(GL_RENDERBUFFER, gl_renderer->msaa_rbos[1]);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                  gl_renderer->msaa_rbos[1]);

        walrus_assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    }
}

static void destroy_msaa_fbo(void)
{
    if (gl_renderer->msaa_fbo != 0) {
        glDeleteFramebuffers(1, &gl_renderer->msaa_fbo);
        gl_renderer->msaa_fbo = 0;
        if (gl_renderer->msaa_rbos[0] != 0) {
            glDeleteRenderbuffers(2, gl_renderer->msaa_rbos);
            gl_renderer->msaa_rbos[0] = 0;
            gl_renderer->msaa_rbos[1] = 0;
        }
    }
}

static void set_render_context_size(u32 width, u32 height, u32 flags)
{
    gl_renderer->resolution.width  = width;
    gl_renderer->resolution.height = height;
    gl_renderer->resolution.flags  = flags;

    destroy_msaa_fbo();
    u32 msaa = (flags & WR_RHI_RESOLUTION_MSAA_MASK) >> WR_RHI_RESOLUTION_MSAA_SHIFT;
    msaa     = walrus_min(gl_renderer->max_msaa, msaa == 0 ? 0 : 1 << msaa);
    create_msaa_fbo(width, height, msaa);
}

static void gl_init(Renderer *renderer, Walrus_RhiCreateInfo const *info, Walrus_RhiCapabilities *caps)
{
#if WR_PLATFORM != WR_PLATFORM_WASM
    GLenum err = glew_init();
    if (err != GLEW_OK) {
        return;
    }
#else
    wajs_setup_gl_context();
#endif

    gl_renderer = poly_cast(renderer, GlRenderer);

    gl_renderer->uniform_registry = walrus_hash_table_create(walrus_str_hash, walrus_str_equal);

    for (u32 i = 0; i < walrus_count_of(gl_renderer->buffers); ++i) {
        gl_renderer->buffers[i].id = 0;
    }

    for (u32 i = 0; i < walrus_count_of(gl_renderer->shaders); ++i) {
        gl_renderer->shaders[i] = 0;
    }

    for (u32 i = 0; i < walrus_count_of(gl_renderer->programs); ++i) {
        gl_renderer->programs[i].id = 0;
    }

    for (u32 i = 0; i < walrus_count_of(gl_renderer->textures); ++i) {
        gl_renderer->textures[i].id = 0;
    }

    for (u32 i = 0; i < walrus_count_of(gl_renderer->framebuffers); ++i) {
        gl_renderer->framebuffers[i].fbo[0] = 0;
        gl_renderer->framebuffers[i].fbo[1] = 0;
    }

    GLint var;
    glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &var);
    caps->ssbo_align = var;

    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &var);
    caps->ubo_align = var;

    glGetIntegerv(GL_MAX_SAMPLES, &var);
    gl_renderer->max_msaa = var;
    caps->max_msaa        = var;

    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &var);
    caps->max_texture_unit = var;

    glDepthRangef(0, 1);

    gl_renderer->msaa_fbo = 0;
    memset(gl_renderer->msaa_rbos, 0, sizeof(gl_renderer->msaa_rbos));
    glGenVertexArrays(1, &gl_renderer->vao);

    set_render_context_size(info->resolution.width, info->resolution.height, info->resolution.flags);
}

static void gl_shutdown(void)
{
    walrus_hash_table_destroy(gl_renderer->uniform_registry);
    destroy_msaa_fbo();
    glDeleteVertexArrays(1, &gl_renderer->vao);
}

static void gl_uniform_create(Walrus_UniformHandle handle, const char *name, u32 size)
{
    gl_renderer->uniforms[handle.id]      = walrus_malloc0(size);
    gl_renderer->uniform_names[handle.id] = walrus_str_dup(name);
    walrus_hash_table_insert(gl_renderer->uniform_registry, gl_renderer->uniform_names[handle.id],
                             walrus_val_to_ptr(handle.id));
}

static void gl_uniform_destroy(Walrus_UniformHandle handle)
{
    walrus_hash_table_remove(gl_renderer->uniform_registry, gl_renderer->uniform_names[handle.id]);
    walrus_free(gl_renderer->uniforms[handle.id]);
    walrus_str_free(gl_renderer->uniform_names[handle.id]);

    gl_renderer->uniforms[handle.id]      = NULL;
    gl_renderer->uniform_names[handle.id] = NULL;
}

static void gl_uniform_resize(Walrus_UniformHandle handle, u32 size)
{
    walrus_realloc(gl_renderer->uniforms[handle.id], size);
}

static void gl_uniform_update(Walrus_UniformHandle handle, u32 offset, u32 size, void const *data)
{
    memcpy((u8 *)gl_renderer->uniforms[handle.id] + offset, data, size);
}

static void gl_vertex_layout_create(Walrus_LayoutHandle handle, Walrus_VertexLayout const *layout)
{
    memcpy(&gl_renderer->vertex_layouts[handle.id], layout, sizeof(Walrus_VertexLayout));
}

static void gl_vertex_layout_destroy(Walrus_LayoutHandle handle)
{
    walrus_unused(handle);
}

static void gl_buffer_create(Walrus_BufferHandle handle, void const *data, u64 size, u16 flags)
{
    GLuint vbo    = 0;
    GLenum target = flags & WR_RHI_BUFFER_INDEX ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
    glGenBuffers(1, &vbo);
    glBindBuffer(target, vbo);
    glBufferData(target, size, data, data == NULL ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

    glBindBuffer(target, 0);

    gl_renderer->buffers[handle.id].id     = vbo;
    gl_renderer->buffers[handle.id].size   = size;
    gl_renderer->buffers[handle.id].target = target;
    gl_renderer->buffers[handle.id].flags  = flags;
}

static void gl_buffer_destroy(Walrus_BufferHandle handle)
{
    glDeleteBuffers(1, &gl_renderer->buffers[handle.id].id);
    gl_renderer->buffers[handle.id].id   = 0;
    gl_renderer->buffers[handle.id].size = 0;
}

static void gl_buffer_update(Walrus_BufferHandle handle, u64 offset, u64 size, void const *data)
{
    GLenum target = gl_renderer->buffers[handle.id].target;
    GLint  id     = gl_renderer->buffers[handle.id].id;
    glBindBuffer(target, id);
    glBufferSubData(target, offset, size, data);
    glBindBuffer(target, 0);
}

static void bind_vertex_attributes(Walrus_VertexLayout const *layout, u64 offset)
{
    for (u8 i = 0; i < layout->num_attributes; ++i) {
        u8                     loc;
        u8                     num;
        Walrus_LayoutComponent type;
        bool                   normalized;
        bool                   as_int;
        walrus_vertex_layout_decode(layout, i, &loc, &num, &type, &normalized, &as_int);
        lazy_enable_vertex_attribute(loc);
        glVertexAttribDivisor(loc, layout->instance_strde);
        if (as_int) {
            glVertexAttribIPointer(loc, num, s_attribute_type[type], layout->stride,
                                   (void const *)(offset + layout->offsets[loc]));
        }
        else {
            glVertexAttribPointer(loc, num, s_attribute_type[type], normalized ? GL_TRUE : GL_FALSE, layout->stride,
                                  (void const *)(offset + layout->offsets[loc]));
        }
    }
}

static u32 set_framebuffer(Walrus_FramebufferHandle handle, u32 height, u32 flags)
{
    if (handle.id != WR_INVALID_HANDLE && handle.id != gl_renderer->fbo.id) {
        GlFramebuffer *fb = &gl_renderer->framebuffers[handle.id];
        gl_framebuffer_resolve(fb);
        if (gl_renderer->discards != WR_RHI_CLEAR_NONE) {
            gl_framebuffer_discard(fb, gl_renderer->discards);
            gl_renderer->discards = WR_RHI_CLEAR_NONE;
        }
    }

    if (handle.id != WR_INVALID_HANDLE) {
        GlFramebuffer *fb        = &gl_renderer->framebuffers[handle.id];
        gl_renderer->current_fbo = fb->fbo[0];
        height                   = fb->height;
    }
    else {
        gl_renderer->current_fbo = gl_renderer->msaa_fbo;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, gl_renderer->current_fbo);

    gl_renderer->fbo      = handle;
    gl_renderer->discards = flags;

    return height;
}

static void update_resolution(Walrus_Resolution *res)
{
    if (res->width != gl_renderer->resolution.width || res->height != gl_renderer->resolution.height ||
        res->flags != gl_renderer->resolution.flags) {
        set_render_context_size(res->width, res->height, res->flags);

        for (u32 i = 0; i < walrus_count_of(gl_renderer->framebuffers); ++i) {
            gl_framebuffer_post_reset(&gl_renderer->framebuffers[i]);
        }
        gl_renderer->current_fbo = 0;

        glBindFramebuffer(GL_FRAMEBUFFER, gl_renderer->current_fbo);
    }
}

static void submit(RenderFrame *frame)
{
    update_resolution(&frame->resolution);

    if (frame->vbo_offset > 0) {
        Walrus_TransientBuffer *vb = frame->transient_vb;
        gl_buffer_update(vb->handle, 0, frame->vbo_offset, vb->data);
    }
    if (frame->ibo_offset > 0) {
        Walrus_TransientBuffer *ib = frame->transient_ib;
        gl_buffer_update(ib->handle, 0, frame->ibo_offset, ib->data);
    }

    u32 resolution_height = frame->resolution.height;

    Sortkey sortkey;
    frame_sort(frame);

    glBindVertexArray(gl_renderer->vao);

    u16             view_id      = UINT16_MAX;
    const ViewRect *view_scissor = NULL;

    Walrus_FramebufferHandle fbh      = (Walrus_FramebufferHandle){WR_RHI_MAX_FRAMEBUFFERS};
    u16                      discards = WR_RHI_CLEAR_NONE;

    Walrus_ProgramHandle current_prog = {WR_INVALID_HANDLE};

    bool was_compute = false;

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    RenderDraw current_state;
    draw_clear(&current_state, WR_RHI_DISCARD_ALL);
    current_state.state_flags = WR_RHI_STATE_NONE;
    current_state.stencil     = pack_stencil(WR_RHI_STENCIL_NONE, WR_RHI_STENCIL_NONE);
    u32 blend_factor          = 0;

    RenderBind current_bind;
    bind_clear(&current_bind, WR_RHI_DISCARD_ALL);

    GLenum primitive = GL_TRIANGLES;

    RenderStats stats;

    clean_stats(&stats);

    for (u32 item = 0; item < frame->num_render_items; ++item) {
        u64 const  key_val    = frame->sortkeys[item];
        bool const is_compute = sortkey_decode(&sortkey, key_val, frame->view_map);

        u32 const         item_id     = frame->sortvalues[item];
        RenderItem const *render_item = &frame->render_items[item_id];
        RenderBind const *render_bind = &frame->render_binds[item_id];
        RenderDraw const *draw        = &render_item->draw;

        bool const        view_changed = sortkey.view_id != view_id;
        RenderView const *view         = &frame->views[sortkey.view_id];

        if (view_changed) {
            view_id = sortkey.view_id;

            if (frame->views[view_id].fb.id != fbh.id) {
                fbh               = frame->views[view_id].fb;
                resolution_height = frame->resolution.height;
                resolution_height = set_framebuffer(fbh, resolution_height, discards);
            }

            RenderClear const *clear    = &view->clear;
            ViewRect const    *viewport = &view->viewport;

            glViewport(viewport->x, resolution_height - viewport->height - viewport->y, viewport->width,
                       viewport->height);
            if (!viewrect_zero(&view->scissor)) {
                view_scissor = &view->scissor;
            }
            discards = view->clear.flags & WR_RHI_CLEAR_DISCARD_MASK;

            GLbitfield clear_flags = 0;
            if (WR_RHI_CLEAR_COLOR & clear->flags) {
                glClearColor(clear->index[0] / 255.f, clear->index[1] / 255.f, clear->index[2] / 255.f,
                             clear->index[3] / 255.f);
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                clear_flags |= GL_COLOR_BUFFER_BIT;
            }
            if (WR_RHI_CLEAR_DEPTH & clear->flags) {
                glClearDepth(clear->depth);
                glDepthMask(GL_TRUE);
                clear_flags |= GL_DEPTH_BUFFER_BIT;
            }
            if (WR_RHI_CLEAR_STENCIL & clear->flags) {
                glClearStencil(clear->stencil);
                clear_flags |= GL_STENCIL_BUFFER_BIT;
            }
            if (clear_flags != 0) {
                glScissor(viewport->x, resolution_height - viewport->height - viewport->y, viewport->width,
                          viewport->height);
                glEnable(GL_SCISSOR_TEST);
                glClear(clear_flags);
                glDisable(GL_SCISSOR_TEST);
            }

            glDisable(GL_STENCIL_TEST);
            glDisable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glDisable(GL_CULL_FACE);
            glDisable(GL_BLEND);
        }

        if (is_compute) {
            if (!was_compute) {
                was_compute = true;
            }
            RenderCompute const *compute = &render_item->compute;
            GlProgram           *program = &gl_renderer->programs[sortkey.program.id];
            glUseProgram(program->id);

            u32 const  max_compute_bindings = WR_RHI_MAX_TEXTURE_SAMPLERS;
            GLbitfield barrier              = 0;
            for (u32 unit = 0; unit < max_compute_bindings; ++unit) {
                Binding const *bind = &render_bind->bindings[unit];
                if (bind->id != WR_INVALID_HANDLE) {
                    GlTexture *texture = &gl_renderer->textures[bind->id];
                    switch (bind->type) {
                        case WR_RHI_BIND_TEXTURE: {
                            glBindTextureUnit(unit, texture->id);
                        } break;
                        case WR_RHI_BIND_IMAGE: {
                            glBindImageTexture(unit, texture->id, bind->mip, GL_FALSE, 0, s_access[bind->access],
                                               s_image_format[bind->format]);
                            barrier |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
                        } break;
                        default:
                            break;
                    }
                }
            }
            for (u32 binding = 0; binding < WR_RHI_MAX_UNIFORM_BINDINGS; ++binding) {
                BlockBinding const *bind = &render_bind->block_bindings[binding];
                if (bind->handle.id != WR_INVALID_HANDLE) {
                    GlBuffer *buffer = &gl_renderer->buffers[bind->handle.id];
                    if (buffer->flags & WR_RHI_BUFFER_UNIFORM_BLOCK) {
                        glBindBufferRange(GL_UNIFORM_BUFFER, binding, buffer->id, bind->offset, bind->size);
                    }
                    else {
                        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding, buffer->id, bind->offset, bind->size);
                        barrier |= GL_SHADER_STORAGE_BARRIER_BIT;
                    }
                }
            }
            if (barrier != 0) {
                renderer_uniform_updates(frame->uniforms, draw->uniform_begin, draw->uniform_end);
                const bool constantsChanged = compute->uniform_begin < compute->uniform_end;
                if (constantsChanged) {
                    commit(program);
                }
                set_predefineds(program, frame, view, compute->start_matrix, compute->num_matrices);
                glDispatchCompute(compute->num_x, compute->num_y, compute->num_z);
                glMemoryBarrier(barrier);

                ++stats.compute_calls;
            }
            continue;
        }

        bool const reset_state = view_changed || was_compute;
        if (was_compute) {
            was_compute = false;
        }

        u64 const new_flags       = draw->state_flags;
        u64       changed_flags   = current_state.state_flags ^ draw->state_flags;
        current_state.state_flags = new_flags;

        u64 const new_stencil     = draw->stencil;
        u64       changed_stencil = current_state.stencil ^ draw->stencil;
        current_state.stencil     = new_stencil;

        if (reset_state) {
            draw_clear(&current_state, WR_RHI_DISCARD_ALL);
            changed_flags             = WR_RHI_STATE_MASK;
            changed_stencil           = pack_stencil(WR_RHI_STENCIL_MASK, WR_RHI_STENCIL_MASK);
            current_state.state_flags = new_flags;
            current_state.stencil     = new_stencil;
            bind_clear(&current_bind, WR_RHI_DISCARD_ALL);
        }
        if (!viewrect_equal(&current_state.scissor, &draw->scissor)) {
            current_state.scissor = draw->scissor;
            if (viewrect_zero_area(&current_state.scissor)) {
                if (view_scissor) {
                    glScissor(view_scissor->x, resolution_height - view_scissor->height - view_scissor->y,
                              view_scissor->width, view_scissor->height);
                    glEnable(GL_SCISSOR_TEST);
                }
                else {
                    glDisable(GL_SCISSOR_TEST);
                }
            }
            else {
                ViewRect scissor_rect = current_state.scissor;
                if (view_scissor) {
                    viewrect_intersect(&scissor_rect, view_scissor);
                }
                glScissor(scissor_rect.x, resolution_height - scissor_rect.height - scissor_rect.y, scissor_rect.width,
                          scissor_rect.height);
                glEnable(GL_SCISSOR_TEST);
            }
        }

        if (WR_RHI_STATE_BLEND_MASK & changed_flags || draw->blend_factor != blend_factor) {
            u32 const blend   = (u32)((new_flags & WR_RHI_STATE_BLEND_MASK) >> WR_RHI_STATE_BLEND_SHIFT);
            u32 const src_rgb = (blend)&0xf;
            u32 const dst_rgb = (blend >> 4) & 0xf;
            u32 const src_a   = (blend >> 8) & 0xf;
            u32 const dst_a   = (blend >> 12) & 0xf;
            if (src_rgb != 0 && dst_rgb != 0 && src_a != 0 && dst_a != 0) {
                glBlendFuncSeparate(s_blend[src_rgb].src, s_blend[dst_rgb].dst, s_blend[src_a].src, s_blend[dst_a].dst);
                if ((s_blend[src_rgb].is_factor || s_blend[dst_rgb].is_factor) && blend_factor != draw->blend_factor) {
                    u32 const rgba = draw->blend_factor;

                    GLclampf rr = ((rgba >> 24)) / 255.0f;
                    GLclampf gg = ((rgba >> 16) & 0xff) / 255.0f;
                    GLclampf bb = ((rgba >> 8) & 0xff) / 255.0f;
                    GLclampf aa = (rgba & 0xff) / 255.0f;

                    glBlendColor(rr, gg, bb, aa);
                    blend_factor = draw->blend_factor;
                }
                glEnable(GL_BLEND);
            }
            else {
                glDisable(GL_BLEND);
            }
        }
        if (WR_RHI_STATE_DEPTH_TEST_MASK & changed_flags) {
            u32 const func = (new_flags & WR_RHI_STATE_DEPTH_TEST_MASK) >> WR_RHI_STATE_DEPTH_TEST_SHIFT;
            if (func != 0) {
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(s_cmp_func[func]);
            }
            else {
                if (new_flags & WR_RHI_STATE_WRITE_Z) {
                    glEnable(GL_DEPTH_TEST);
                    glDepthFunc(GL_ALWAYS);
                }
                else {
                    glDisable(GL_DEPTH_TEST);
                }
            }
        }
        if (WR_RHI_STATE_WRITE_MASK & changed_flags) {
            glColorMask(!!(new_flags & WR_RHI_STATE_WRITE_R), !!(new_flags & WR_RHI_STATE_WRITE_G),
                        !!(new_flags & WR_RHI_STATE_WRITE_B), !!(new_flags & WR_RHI_STATE_WRITE_A));
            glDepthMask(!!(new_flags & WR_RHI_STATE_WRITE_Z));
        }

        if (WR_RHI_STATE_CULL_MASK & changed_flags) {
            if (WR_RHI_STATE_CULL_CCW & new_flags) {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_FRONT);
            }
            else if (WR_RHI_STATE_CULL_CW & new_flags) {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
            }
            else {
                glDisable(GL_CULL_FACE);
            }
        }

        if (changed_stencil != 0) {
            if (new_stencil != 0) {
                glEnable(GL_STENCIL_TEST);

                u32 fstencil     = unpack_stencil(0, new_stencil);
                u32 bstencil     = unpack_stencil(1, new_stencil);
                u8  frontAndBack = bstencil != WR_RHI_STENCIL_NONE && bstencil != fstencil;

                for (u8 i = 0; i < frontAndBack + 1; ++i) {
                    u32    stencil = unpack_stencil(i, new_stencil);
                    u32    changed = unpack_stencil(i, changed_stencil);
                    GLenum face    = s_stencilface[i];
                    if ((WR_RHI_STENCIL_TEST_MASK | WR_RHI_STENCIL_FUNC_REF_MASK | WR_RHI_STENCIL_FUNC_RMASK_MASK) &
                        changed) {
                        u32 ref  = (stencil & WR_RHI_STENCIL_FUNC_REF_MASK) >> WR_RHI_STENCIL_FUNC_REF_SHIFT;
                        u32 mask = (stencil & WR_RHI_STENCIL_FUNC_RMASK_MASK) >> WR_RHI_STENCIL_FUNC_RMASK_SHIFT;
                        u32 func = (stencil & WR_RHI_STENCIL_TEST_MASK) >> WR_RHI_STENCIL_TEST_SHIFT;
                        glStencilFuncSeparate(face, s_cmpfunc[func], ref, mask);
                    }
                    if ((WR_RHI_STENCIL_OP_FAIL_S_MASK | WR_RHI_STENCIL_OP_FAIL_Z_MASK |
                         WR_RHI_STENCIL_OP_PASS_Z_MASK) &
                        changed) {
                        u32 sfail = (stencil & WR_RHI_STENCIL_OP_FAIL_S_MASK) >> WR_RHI_STENCIL_OP_FAIL_S_SHIFT;
                        u32 zfail = (stencil & WR_RHI_STENCIL_OP_FAIL_Z_MASK) >> WR_RHI_STENCIL_OP_FAIL_Z_SHIFT;
                        u32 zpass = (stencil & WR_RHI_STENCIL_OP_PASS_Z_MASK) >> WR_RHI_STENCIL_OP_PASS_Z_SHIFT;
                        glStencilOpSeparate(face, s_stencilop[sfail], s_stencilop[zfail], s_stencilop[zpass]);
                    }
                }
            }
            else {
                glDisable(GL_STENCIL_TEST);
            }
        }

        if (WR_RHI_STATE_DRAW_MASK & changed_flags) {
            primitive = s_primitives[((new_flags & WR_RHI_STATE_DRAW_MASK) >> WR_RHI_STATE_DRAW_SHIFT)];
        }

        renderer_uniform_updates(frame->uniforms, draw->uniform_begin, draw->uniform_end);
        bool const program_changed = current_prog.id != sortkey.program.id;
        if (program_changed) {
            current_prog = sortkey.program;
            if (current_prog.id != WR_INVALID_HANDLE) {
                glUseProgram(gl_renderer->programs[current_prog.id].id);
            }
            else {
                glUseProgram(0);
            }
        }

        if (current_prog.id != WR_INVALID_HANDLE) {
            GlProgram const *program = &gl_renderer->programs[current_prog.id];

            bool const constants_changed = draw->uniform_begin < draw->uniform_end;
            if (program_changed || constants_changed) {
                commit(program);
            }
            set_predefineds(program, frame, view, draw->start_matrix, draw->num_matrices);
            GLbitfield barrier = 0;
            for (u32 unit = 0; unit < WR_RHI_MAX_TEXTURE_SAMPLERS; ++unit) {
                Binding const *bind    = &render_bind->bindings[unit];
                Binding       *current = &current_bind.bindings[unit];
                if (program_changed || bind->id != current->id || bind->type != current->type ||
                    bind->sampler_flags != current->sampler_flags) {
                    if (bind->id != WR_INVALID_HANDLE) {
                        GlTexture const *texture = &gl_renderer->textures[bind->id];
                        switch (bind->type) {
                            case WR_RHI_BIND_TEXTURE: {
                                glActiveTexture(GL_TEXTURE0 + unit);
                                glBindTexture(texture->target, texture->id);
                            } break;
                            case WR_RHI_BIND_IMAGE: {
                                glBindImageTexture(unit, texture->id, bind->mip, GL_FALSE, 0, s_access[bind->access],
                                                   s_image_format[bind->format]);
                                barrier |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
                            } break;
                            default:
                                break;
                        }
                    }
                    *current = *bind;
                }
            }
            for (u32 binding = 0; binding < WR_RHI_MAX_UNIFORM_BINDINGS; ++binding) {
                BlockBinding const *bind = &render_bind->block_bindings[binding];
                if (bind->handle.id != WR_INVALID_HANDLE) {
                    GlBuffer *buffer = &gl_renderer->buffers[bind->handle.id];
                    if (buffer->flags & WR_RHI_BUFFER_UNIFORM_BLOCK) {
                        glBindBufferRange(GL_UNIFORM_BUFFER, binding, buffer->id, bind->offset, bind->size);
                    }
                    else {
                        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding, buffer->id, bind->offset, bind->size);
                        barrier |= GL_SHADER_STORAGE_BARRIER_BIT;
                    }
                }
            }
            walrus_unused(barrier);

            bool bind_attributes = false;
            if (draw->stream_mask != UINT16_MAX) {
                for (u32 id = 0, stream_mask = draw->stream_mask; 0 != stream_mask; stream_mask >>= 1, ++id) {
                    u32 const ntz = walrus_u32cnttz(stream_mask);
                    stream_mask >>= ntz;
                    id += ntz;

                    VertexStream const *stream = &draw->streams[id];
                    if (current_state.streams[id].handle.id != stream->handle.id) {
                        bind_attributes = true;
                    }
                    if (current_state.streams[id].layout_handle.id != stream->layout_handle.id) {
                        bind_attributes = true;
                    }
                    if (current_state.streams[id].offset != stream->offset) {
                        bind_attributes = true;
                    }
                }
            }

            if (program_changed || current_state.stream_mask != draw->stream_mask ||
                current_state.instance_buffer.id != draw->instance_buffer.id ||
                current_state.instance_layout.id != draw->instance_layout.id ||
                current_state.num_instances != draw->num_instances ||
                current_state.instance_offset != draw->instance_offset) {
                current_state.stream_mask     = draw->stream_mask;
                current_state.instance_buffer = draw->instance_buffer;
                current_state.instance_layout = draw->instance_layout;
                current_state.instance_offset = draw->instance_offset;

                bind_attributes = true;
            }

            if (current_state.index_buffer.id != draw->index_buffer.id) {
                current_state.index_buffer = draw->index_buffer;

                if (draw->index_buffer.id != WR_INVALID_HANDLE) {
                    GlBuffer const *ib = &gl_renderer->buffers[draw->index_buffer.id];
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib->id);
                }
                else {
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
                }
            }

            u32 num_vertices  = draw->num_vertices;
            u32 num_instances = draw->num_instances;
            if (num_vertices == UINT32_MAX) {
                for (u32 id = 0, stream_mask = draw->stream_mask; 0 != stream_mask; stream_mask >>= 1, ++id) {
                    u32 const ntz = walrus_u32cnttz(stream_mask);
                    stream_mask >>= ntz;
                    id += ntz;

                    VertexStream const *stream = &draw->streams[id];

                    if (stream->handle.id != WR_INVALID_HANDLE) {
                        GlBuffer const *vb = &gl_renderer->buffers[stream->handle.id];

                        Walrus_LayoutHandle const layout_handle = stream->layout_handle;
                        if (layout_handle.id != WR_INVALID_HANDLE) {
                            Walrus_VertexLayout const *layout = &gl_renderer->vertex_layouts[layout_handle.id];

                            num_vertices = walrus_min(num_vertices, vb->size / layout->stride);
                        }
                    }
                }
            }
            bool const instance_valid =
                draw->instance_buffer.id != WR_INVALID_HANDLE && draw->instance_layout.id != WR_INVALID_HANDLE;
            if (instance_valid && num_instances == UINT32_MAX) {
                Walrus_VertexLayout const *layout          = &gl_renderer->vertex_layouts[draw->instance_layout.id];
                GlBuffer const            *instance_buffer = &gl_renderer->buffers[draw->instance_buffer.id];
                num_instances = walrus_min(num_instances, instance_buffer->size / layout->stride);
            }
            if (bind_attributes && draw->stream_mask != UINT16_MAX) {
                for (u8 i = 0; i < WR_RHI_MAX_VERTEX_ATTRIBUTES; ++i) {
                    lazy_disable_vertex_attribute(i);
                    glVertexAttribDivisor(i, 0);
                }
                for (u32 id = 0, stream_mask = draw->stream_mask; 0 != stream_mask; stream_mask >>= 1, ++id) {
                    u32 const ntz = walrus_u32cnttz(stream_mask);
                    stream_mask >>= ntz;
                    id += ntz;
                    VertexStream const *stream = &draw->streams[id];
                    if (stream->handle.id != WR_INVALID_HANDLE) {
                        GlBuffer const *vb = &gl_renderer->buffers[stream->handle.id];

                        Walrus_LayoutHandle const layout_handle = stream->layout_handle;
                        if (layout_handle.id != WR_INVALID_HANDLE) {
                            glBindBuffer(GL_ARRAY_BUFFER, vb->id);
                            Walrus_VertexLayout const *layout = &gl_renderer->vertex_layouts[layout_handle.id];
                            bind_vertex_attributes(layout, stream->offset);
                            glBindBuffer(GL_ARRAY_BUFFER, 0);
                        }
                    }
                }
                if (instance_valid) {
                    GlBuffer const *instance_buffer = &gl_renderer->buffers[draw->instance_buffer.id];
                    glBindBuffer(GL_ARRAY_BUFFER, instance_buffer->id);

                    Walrus_VertexLayout const *layout = &gl_renderer->vertex_layouts[draw->instance_layout.id];
                    bind_vertex_attributes(layout, draw->instance_offset);
                    glBindBuffer(GL_ARRAY_BUFFER, 0);
                }
                apply_lazy_enabled_vertex_attribute();
            }

            if (draw->index_buffer.id != WR_INVALID_HANDLE) {
                static GLenum const index_type[5] = {GL_ZERO, GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_ZERO,
                                                     GL_UNSIGNED_INT};

                GlBuffer const *ib = &gl_renderer->buffers[draw->index_buffer.id];

                u32 num_indices = draw->num_indices;
                if (num_indices == UINT32_MAX) {
                    num_indices = ib->size / draw->index_size;
                }
                glDrawElementsInstanced(primitive, num_indices, index_type[draw->index_size],
                                        (void *)draw->index_offset, num_instances);

                ++stats.draw_calls;
                stats.num_vertices += num_vertices;
                stats.num_indices += num_indices;
            }
            else if (num_vertices != UINT32_MAX) {
                glDrawArraysInstanced(primitive, 0, num_vertices, num_instances);

                ++stats.draw_calls;
                stats.num_vertices += num_vertices;
            }
        }
    }

    if (frame->debug_flags & WR_RHI_DEBUG_STATS) {
        output_stats(&stats);
    }

    glBindVertexArray(0);
    if (gl_renderer->msaa_fbo != 0) {
        glDisable(GL_SCISSOR_TEST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, gl_renderer->msaa_fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, gl_renderer->resolution.width, gl_renderer->resolution.height, 0, 0,
                          gl_renderer->resolution.width, gl_renderer->resolution.height, GL_COLOR_BUFFER_BIT,
                          GL_LINEAR);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
}

POLY_DEFINE_DERIVED(Renderer, GlRenderer, gl_create, POLY_IMPL(init, gl_init), POLY_IMPL(shutdown, gl_shutdown),
                    POLY_IMPL(submit, submit), POLY_IMPL(create_shader, gl_shader_create),
                    POLY_IMPL(destroy_shader, gl_shader_destroy), POLY_IMPL(create_program, gl_program_create),
                    POLY_IMPL(destroy_program, gl_program_destroy), POLY_IMPL(create_uniform, gl_uniform_create),
                    POLY_IMPL(destroy_uniform, gl_uniform_destroy), POLY_IMPL(resize_uniform, gl_uniform_resize),
                    POLY_IMPL(update_uniform, gl_uniform_update),
                    POLY_IMPL(create_vertex_layout, gl_vertex_layout_create),
                    POLY_IMPL(destroy_vertex_layout, gl_vertex_layout_destroy),
                    POLY_IMPL(create_buffer, gl_buffer_create), POLY_IMPL(destroy_buffer, gl_buffer_destroy),
                    POLY_IMPL(update_buffer, gl_buffer_update), POLY_IMPL(create_texture, gl_texture_create),
                    POLY_IMPL(destroy_texture, gl_texture_destroy), POLY_IMPL(resize_texture, gl_texture_resize),
                    POLY_IMPL(create_framebuffer, gl_framebuffer_create),
                    POLY_IMPL(destroy_framebuffer, gl_framebuffer_destroy))

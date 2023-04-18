#include "rhi_p.h"
#include "gl_shader.h"
#include "gl_texture.h"
#include "frame.h"

#include <core/macro.h>
#include <core/log.h>
#include <core/memory.h>
#include <core/string.h>
#include <core/math.h>

#include <string.h>
#include <cglm/cglm.h>
#include <cglm/io.h>

GLenum glew_init(void);

void wajs_setup_gl_context(void);

GlContext *g_ctx = NULL;

static GLenum const s_gl_attribute_type[WR_RHI_ATTR_COUNT] = {
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
        void *data = g_ctx->uniforms[handle.id];

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

static void submit(RenderFrame *frame)
{
    u32 const resolution_height = frame->resolution.height;

    glBindVertexArray(g_ctx->vao);

    u16 view_id = UINT16_MAX;

    Walrus_ProgramHandle current_prog = {WR_INVALID_HANDLE};

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    RenderDraw current_state;
    draw_clear(&current_state, WR_RHI_DISCARD_ALL);
    u32 blend_factor = 0;

    RenderBind current_bind;
    bind_clear(&current_bind, WR_RHI_DISCARD_ALL);

    for (u32 i = 0; i < frame->num_render_items; ++i) {
        RenderItem const          *render_item = &frame->render_items[i];
        RenderBind const          *render_bind = &frame->render_binds[i];
        RenderDraw const          *draw        = &render_item->draw;
        RenderCompute const       *compute     = &render_item->compute;
        Walrus_ProgramHandle const prog        = frame->program[i];
        RenderView const          *view        = &frame->views[frame->view_ids[i]];

        bool const view_changed = frame->view_ids[i] != view_id;

        if (view_changed) {
            view_id = frame->view_ids[i];

            Walrus_RenderClear const *clear    = &view->clear;
            Walrus_Rect const        *viewport = &view->viewport;

            glViewport(viewport->x, resolution_height - viewport->height - viewport->y, viewport->width,
                       viewport->height);

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
                glClear(clear_flags);
            }
        }

        bool const program_changed = prog.id != WR_INVALID_HANDLE && current_prog.id != prog.id;
        if (program_changed) {
            current_prog = prog;
            glUseProgram(g_ctx->programs[current_prog.id].id);
        }
        u64 const new_flags       = draw->state_flags;
        u64       changed_flags   = current_state.state_flags ^ draw->state_flags;
        current_state.state_flags = new_flags;
        const bool resetState     = view_changed;

        if (resetState) {
            draw_clear(&current_state, WR_RHI_DISCARD_ALL);
            changed_flags = WR_RHI_STATE_MASK;
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

        renderer_uniform_updates(frame->uniforms, draw->uniform_begin, draw->uniform_end);

        bool const constants_changed = draw->uniform_begin < draw->uniform_end;
        if (current_prog.id != WR_INVALID_HANDLE) {
            GlProgram const *program = &g_ctx->programs[current_prog.id];
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
                        GlTexture const *texture = &g_ctx->textures[bind->id];
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
            walrus_unused(barrier);

            bool bind_attributes = false;
            if (draw->stream_mask != 0) {
                for (u32 id = 0, stream_mask = draw->stream_mask; 0 != stream_mask; stream_mask >>= 1, ++id) {
                    u32 const ntz = walrus_u32cnttz(stream_mask);
                    stream_mask >>= ntz;
                    id += ntz;

                    VertexStream const *stream = &draw->streams[id];
                    if (current_state.streams[id].handle.id != stream->handle.id) {
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
                current_state.index_buffer.id = draw->index_buffer.id;

                if (draw->index_buffer.id != WR_INVALID_HANDLE) {
                    GlBuffer const *ib = &g_ctx->buffers[draw->index_buffer.id];
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib->id);
                }
                else {
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
                }
            }

            u32 num_vertices  = draw->num_vertices;
            u32 num_instances = draw->num_instances;
            if (draw->stream_mask != 0 && num_vertices == UINT32_MAX) {
                for (u32 id = 0, stream_mask = draw->stream_mask; 0 != stream_mask; stream_mask >>= 1, ++id) {
                    u32 const ntz = walrus_u32cnttz(stream_mask);
                    stream_mask >>= ntz;
                    id += ntz;

                    VertexStream const *stream = &draw->streams[id];
                    if (current_state.streams[id].handle.id != stream->handle.id) {
                        bind_attributes = true;
                    }
                    if (current_state.streams[id].offset != stream->offset) {
                        bind_attributes = true;
                    }

                    if (stream->handle.id != WR_INVALID_HANDLE) {
                        GlBuffer const vb = g_ctx->buffers[stream->handle.id];

                        Walrus_LayoutHandle const layout_handle = stream->layout_handle;
                        if (layout_handle.id != WR_INVALID_HANDLE) {
                            Walrus_VertexLayout const *layout = &g_ctx->vertex_layouts[layout_handle.id];

                            num_vertices = walrus_min(num_vertices, vb.size / layout->stride);
                        }
                    }
                }
            }
            bool const instance_valid =
                draw->instance_buffer.id != WR_INVALID_HANDLE && draw->instance_layout.id != WR_INVALID_HANDLE;
            if (instance_valid && num_instances == UINT32_MAX) {
                Walrus_VertexLayout const *layout          = &g_ctx->vertex_layouts[draw->instance_layout.id];
                GlBuffer const             instance_buffer = g_ctx->buffers[draw->instance_buffer.id];
                num_instances = walrus_min(num_instances, instance_buffer.size / layout->stride);
            }
            if (bind_attributes) {
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
                        GlBuffer const vb = g_ctx->buffers[stream->handle.id];

                        Walrus_LayoutHandle const layout_handle = stream->layout_handle;
                        if (layout_handle.id != WR_INVALID_HANDLE) {
                            glBindBuffer(GL_ARRAY_BUFFER, vb.id);
                            Walrus_VertexLayout const *layout = &g_ctx->vertex_layouts[layout_handle.id];
                            for (u8 i = 0; i < layout->num_attributes; ++i) {
                                Walrus_Attribute type;
                                u8               attr_id;
                                u8               num;
                                bool             normalized;
                                bool             as_int;
                                walrus_vertex_layout_decode(layout, i, &attr_id, &num, &type, &normalized, &as_int);
                                lazy_enable_vertex_attribute(attr_id);
                                if (as_int) {
                                    glVertexAttribIPointer(attr_id, num, s_gl_attribute_type[type], layout->stride,
                                                           (void const *)(layout->offsets[i] + stream->offset));
                                }
                                else {
                                    glVertexAttribPointer(attr_id, num, s_gl_attribute_type[type],
                                                          normalized ? GL_TRUE : GL_FALSE, layout->stride,
                                                          (void const *)(layout->offsets[i] + stream->offset));
                                }
                            }
                            glBindBuffer(GL_ARRAY_BUFFER, 0);
                        }
                    }
                }
                if (instance_valid) {
                    GlBuffer const instance_buffer = g_ctx->buffers[draw->instance_buffer.id];
                    glBindBuffer(GL_ARRAY_BUFFER, instance_buffer.id);

                    Walrus_VertexLayout const *layout = &g_ctx->vertex_layouts[draw->instance_layout.id];
                    for (u8 i = 0; i < layout->num_attributes; ++i) {
                        Walrus_Attribute type;
                        u8               attr_id;
                        u8               num;
                        bool             normalized;
                        bool             as_int;
                        walrus_vertex_layout_decode(layout, i, &attr_id, &num, &type, &normalized, &as_int);
                        lazy_enable_vertex_attribute(attr_id);
                        glVertexAttribDivisor(attr_id, layout->instance_strde);
                        if (as_int) {
                            glVertexAttribIPointer(attr_id, num, s_gl_attribute_type[type], layout->stride,
                                                   (void const *)(layout->offsets[i] + draw->instance_offset));
                        }
                        else {
                            glVertexAttribPointer(attr_id, num, s_gl_attribute_type[type],
                                                  normalized ? GL_TRUE : GL_FALSE, layout->stride,
                                                  (void const *)(layout->offsets[i] + draw->instance_offset));
                        }
                    }
                    glBindBuffer(GL_ARRAY_BUFFER, 0);
                }
                apply_lazy_enabled_vertex_attribute();
            }

            if (draw->index_buffer.id != WR_INVALID_HANDLE) {
                static GLenum const gl_index_type[5] = {GL_ZERO, GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_ZERO,
                                                        GL_UNSIGNED_INT};

                GlBuffer const *ib = &g_ctx->buffers[draw->index_buffer.id];

                u32 num_indices = draw->num_indices;
                if (num_indices == UINT32_MAX) {
                    num_indices = ib->size / draw->index_size;
                }
                glDrawElementsInstanced(GL_TRIANGLES, num_indices, gl_index_type[draw->index_size],
                                        (void *)draw->index_offset, num_instances);
            }
            else if (num_vertices != UINT32_MAX) {
                glDrawArraysInstanced(GL_TRIANGLES, 0, num_vertices, num_instances);
            }
        }

        walrus_unused(compute);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

static void init_ctx(RhiContext *rhi)
{
#if WR_PLATFORM != WR_PLATFORM_WASM
    GLenum err = glew_init();
    if (err != GLEW_OK) {
        rhi->err     = WR_RHI_INIT_GLEW_ERROR;
        rhi->err_msg = (char const *)glewGetErrorString(err);

        return;
    }
#else
    wajs_setup_gl_context();
#endif
    g_ctx = walrus_malloc(sizeof(GlContext));
    if (g_ctx == NULL) {
        rhi->err     = WR_RHI_ALLOC_ERROR;
        rhi->err_msg = WR_RHI_GL_ALLOC_FAIL_STR;
        return;
    }

    g_ctx->uniform_registry = walrus_hash_table_create(walrus_str_hash, walrus_str_equal);

    glGenVertexArrays(1, &g_ctx->vao);
}

static void gl_uniform_create(Walrus_UniformHandle handle, const char *name, u32 size)
{
    g_ctx->uniforms[handle.id]      = walrus_malloc0(size);
    g_ctx->uniform_names[handle.id] = walrus_str_dup(name);
    walrus_hash_table_insert(g_ctx->uniform_registry, g_ctx->uniform_names[handle.id], walrus_u32_to_ptr(handle.id));
}

static void gl_uniform_destroy(Walrus_UniformHandle handle)
{
    walrus_hash_table_remove(g_ctx->uniform_registry, g_ctx->uniform_names[handle.id]);
    walrus_free(g_ctx->uniforms[handle.id]);
    walrus_str_free(g_ctx->uniform_names[handle.id]);

    g_ctx->uniforms[handle.id]      = NULL;
    g_ctx->uniform_names[handle.id] = NULL;
}

static void gl_uniform_update(Walrus_UniformHandle handle, u32 offset, u32 size, void const *data)
{
    memcpy((u8 *)g_ctx->uniforms[handle.id] + offset, data, size);
}

static void gl_vertex_layout_create(Walrus_LayoutHandle handle, Walrus_VertexLayout const *layout)
{
    memcpy(&g_ctx->vertex_layouts[handle.id], layout, sizeof(Walrus_VertexLayout));
}

static void gl_vertex_layout_destroy(Walrus_LayoutHandle handle)
{
    walrus_unused(handle);
}

static void gl_buffer_create(Walrus_BufferHandle handle, void const *data, u64 size, u16 flags)
{
    walrus_unused(flags);

    GLuint vbo    = 0;
    GLenum target = flags & WR_RHI_BUFFER_INDEX ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
    glGenBuffers(1, &vbo);
    glBindBuffer(target, vbo);
    glBufferData(target, size, data, GL_STATIC_DRAW);

    glBindBuffer(target, 0);

    g_ctx->buffers[handle.id].id     = vbo;
    g_ctx->buffers[handle.id].size   = size;
    g_ctx->buffers[handle.id].target = target;
}

static void gl_buffer_destroy(Walrus_BufferHandle handle)
{
    glDeleteBuffers(1, &g_ctx->buffers[handle.id].id);
    g_ctx->buffers[handle.id].id   = 0;
    g_ctx->buffers[handle.id].size = 0;
}

static void gl_buffer_update(Walrus_BufferHandle handle, u64 offset, u64 size, void const *data)
{
    GLenum target = g_ctx->buffers[handle.id].target;
    GLint  id     = g_ctx->buffers[handle.id].id;
    glBindBuffer(target, id);
    glBufferSubData(target, offset, size, data);
    glBindBuffer(target, 0);
}

static void init_api(RhiVTable *vtable)
{
    vtable->submit_fn = submit;

    vtable->shader_create_fn  = gl_shader_create;
    vtable->shader_destroy_fn = gl_shader_destroy;

    vtable->program_create_fn  = gl_program_create;
    vtable->program_destroy_fn = gl_program_destroy;

    vtable->uniform_create_fn  = gl_uniform_create;
    vtable->uniform_destroy_fn = gl_uniform_destroy;
    vtable->uniform_update_fn  = gl_uniform_update;

    vtable->vertex_layout_create_fn  = gl_vertex_layout_create;
    vtable->vertex_layout_destroy_fn = gl_vertex_layout_destroy;

    vtable->buffer_create_fn  = gl_buffer_create;
    vtable->buffer_destroy_fn = gl_buffer_destroy;
    vtable->buffer_update_fn  = gl_buffer_update;

    vtable->texture_create_fn  = gl_texture_create;
    vtable->texture_destroy_fn = gl_texture_destroy;
}

void gl_backend_init(RhiContext *rhi, RhiVTable *vtable)
{
    init_ctx(rhi);
    init_api(vtable);
}

void gl_backend_shutdown(void)
{
    if (g_ctx) {
        walrus_hash_table_destroy(g_ctx->uniform_registry);
        glDeleteVertexArrays(1, &g_ctx->vao);
        walrus_free(g_ctx);
        g_ctx = NULL;
    }
}

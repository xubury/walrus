#include "rhi_p.h"
#include <core/assert.h>
#include <core/math.h>
#include <core/memory.h>
#include <core/string.h>

#include <math.h>
#include <string.h>
#include <cglm/mat4.h>

typedef struct {
    Walrus_UniformType type;
    char const*        name;
    u8                 num;
} UniformAttribute;

static UniformAttribute const s_predefineds[PREDEFINED_COUNT] = {
    {WR_RHI_UNIFORM_MAT4, "u_view", 1},
    {WR_RHI_UNIFORM_MAT4, "u_viewproj", 1},
    {WR_RHI_UNIFORM_MAT4, "u_projection", 1},
    {WR_RHI_UNIFORM_MAT4, "u_model", 1},
};

static char const* no_backend_str = "No render backend specifed";

static RhiContext*  s_ctx      = NULL;
static RhiRenderer* s_renderer = NULL;

static void handles_init(void)
{
    s_ctx->shaders        = walrus_handle_create(WR_RHI_MAX_SHADERS);
    s_ctx->programs       = walrus_handle_create(WR_RHI_MAX_PROGRAMS);
    s_ctx->uniforms       = walrus_handle_create(WR_RHI_MAX_UNIFORMS);
    s_ctx->vertex_layouts = walrus_handle_create(WR_RHI_MAX_VERTEX_LAYOUTS);
    s_ctx->buffers        = walrus_handle_create(WR_RHI_MAX_BUFFERS);
    s_ctx->textures       = walrus_handle_create(WR_RHI_MAX_TEXTURES);
}

static void handles_shutdown(void)
{
    walrus_handle_destroy(s_ctx->textures);
    walrus_handle_destroy(s_ctx->buffers);
    walrus_handle_destroy(s_ctx->vertex_layouts);
    walrus_handle_destroy(s_ctx->uniforms);
    walrus_handle_destroy(s_ctx->programs);
    walrus_handle_destroy(s_ctx->shaders);
}

static void discard(u8 flags)
{
    draw_clear(&s_ctx->draw, flags);
    bind_clear(&s_ctx->bind, flags);
}

static void view_reset(RenderView* view)
{
    view->viewport = (ViewRect){0, 0, 1, 1};
    view->scissor  = (ViewRect){0, 0, 0, 0};

    view->mode = WR_RHI_VIEWMODE_DEFAULT;

    walrus_rhi_decompose_rgba(0, view->clear.index, view->clear.index + 1, view->clear.index + 2,
                              view->clear.index + 3);
    view->clear.flags   = WR_RHI_CLEAR_NONE;
    view->clear.depth   = 0;
    view->clear.stencil = 0;
}

void renderer_uniform_updates(UniformBuffer* uniform, u32 begin, u32 end)
{
    uniform_buffer_start(uniform, begin);
    while (uniform->pos < end) {
        u64 op = uniform_buffer_read_value(uniform);
        if (op == UNIFORM_BUFFER_END) {
            break;
        }

        Walrus_UniformType   type;
        Walrus_UniformHandle handle;

        uniform_decode_op(&type, &handle.id, NULL, op);
        u32         offset = uniform_buffer_read_value(uniform);
        u32         size   = uniform_buffer_read_value(uniform);
        void const* data   = uniform_buffer_read(uniform, size);
        if (type < WR_RHI_UNIFORM_COUNT) {
            s_renderer->uniform_update_fn(handle, offset, size, data);
        }
    }
}

u8 get_predefined_type(char const* name)
{
    for (u8 i = 0; i < PREDEFINED_COUNT; ++i) {
        if (strcmp(s_predefineds[i].name, name) == 0) {
            return i;
        }
    }
    return PREDEFINED_COUNT;
}

char const* get_glsl_header(void)
{
#if WR_PLATFORM == WR_PLATFORM_WASM
    return "#version 300 es\n precision mediump float;\nprecision mediump sampler2DArray;\n";
#else
    return "#version 430 core\n";
#endif
}

static void compute_texture_size_from_ratio(Walrus_BackBufferRatio ratio, u32* width, u32* height)
{
    switch (ratio) {
        case WR_RHI_RATIO_DOUBLE:
            *width *= 2;
            *height *= 2;
            break;
        case WR_RHI_RATIO_HALF:
            *width /= 2;
            *height /= 2;
            break;
        case WR_RHI_RATIO_QUARTER:
            *width /= 4;
            *height /= 4;
            break;
        case WR_RHI_RATIO_EIGHTH:
            *width /= 8;
            *height /= 8;
            break;
        case WR_RHI_RATIO_SIXTEENTH:
            *width /= 16;
            *height /= 16;
            break;
        default:
            break;
    }
    *width  = walrus_max(*width, 1);
    *height = walrus_max(*height, 1);
}

void renderer_create(Walrus_RhiCreateInfo* info)
{
    s_renderer = walrus_malloc(sizeof(RhiRenderer));

    if (info->flags & WR_RHI_FLAG_OPENGL) {
        gl_backend_init(s_ctx, s_renderer);
    }
    else {
        walrus_assert_msg(false, no_backend_str);
    }
}

void renderer_destroy(void)
{
    walrus_free(s_renderer);

    if (s_ctx->info.flags & WR_RHI_FLAG_OPENGL) {
        gl_backend_shutdown();
    }

    s_renderer = NULL;
}

static CommandBuffer* get_command_buffer(Command cmd)
{
    CommandBuffer* cmdbuf = cmd < COMMAND_END ? &s_ctx->submit_frame->cmd_pre : &s_ctx->submit_frame->cmd_post;
    if (cmd >= COMMAND_END) {
        walrus_assert(cmd != COMMAND_RENDERER_INIT);
    }
    command_buffer_write(cmdbuf, Command, &cmd);
    return cmdbuf;
}

void render_sem_post(void)
{
    if (!s_ctx->info.single_thread) {
        walrus_semaphore_post(s_ctx->render_sem, 1);
    }
}

static bool render_sem_wait(i32 ms)
{
    if (!s_ctx->info.single_thread) {
        return walrus_semaphore_wait(s_ctx->render_sem, ms);
    }
    return true;
}

static void api_sem_post(void)
{
    if (!s_ctx->info.single_thread) {
        walrus_semaphore_post(s_ctx->api_sem, 1);
    }
}

static bool api_sem_wait(i32 ms)
{
    if (!s_ctx->info.single_thread) {
        return walrus_semaphore_wait(s_ctx->api_sem, ms);
    }
    return true;
}

#define queue_free(alloc, q)                   \
    for (u16 i = 0; i < q.num; ++i) {          \
        walrus_handle_free(alloc, q.queue[i]); \
    }

static void free_all_handles(RenderFrame* frame)
{
    queue_free(s_ctx->buffers, frame->queue_buffer);
    queue_free(s_ctx->vertex_layouts, frame->queue_layout);
    queue_free(s_ctx->shaders, frame->queue_shader);
    queue_free(s_ctx->programs, frame->queue_program);
    queue_free(s_ctx->uniforms, frame->queue_uniform);
}

static void frame_swap(void)
{
    RenderFrame* frame = s_ctx->submit_frame;

    frame->resolution = s_ctx->resolution;
    memcpy(frame->view_map, s_ctx->view_map, sizeof(s_ctx->view_map));
    memcpy(frame->views, s_ctx->views, sizeof(s_ctx->views));

    free_all_handles(frame);
    frame_reset_all_free_handles(frame);
    frame_finish(frame);

    if (!s_ctx->info.single_thread) {
        s_ctx->submit_frame = s_ctx->render_frame;
        s_ctx->render_frame = frame;
    }

    s_ctx->uniform_begin = 0;
    s_ctx->uniform_end   = 0;

    if (s_ctx->info.single_thread) {
        walrus_rhi_render_frame(-1);
    }

    frame_start(s_ctx->submit_frame);

    memset(s_ctx->seqs, 0, sizeof(s_ctx->seqs));
}

static void frame_no_render_wait(void)
{
    frame_swap();
    api_sem_post();
}

static void render_exec_command(CommandBuffer* buffer)
{
    command_buffer_reset(buffer);
    bool end = false;
    if (s_renderer == NULL) {
        Command cmd;
        command_buffer_read(buffer, Command, &cmd);
        switch (cmd) {
            case COMMAND_RENDERER_SHUTDOWN_END:
                walrus_trace("Exec Command: RendererShutdownEnd");
                s_ctx->exit = true;
                break;
            case COMMAND_END:
                return;
            case COMMAND_RENDERER_INIT: {
                walrus_assert_msg(!s_ctx->initialized, "Unexpected command: Renderer already initialized!");
                walrus_trace("Exec Command: RendererInit");
                Walrus_RhiCreateInfo info;
                command_buffer_read(buffer, Walrus_RhiCreateInfo, &info);

                renderer_create(&info);
                s_ctx->initialized = s_renderer != NULL;
                s_ctx->err         = WR_RHI_SUCCESS;
                if (!s_ctx->initialized) {
                    command_buffer_read(buffer, Command, &cmd);
                    walrus_assert_msg(cmd == COMMAND_END, "Unexpected command!");
                    return;
                }
            } break;
            default:
                break;
        }
    }
    do {
        Command cmd;
        command_buffer_read(buffer, Command, &cmd);
        switch (cmd) {
            default:
                walrus_assert_msg(false, "Unexpected command");
                break;
            case COMMAND_RENDERER_SHUTDOWN_BEGIN:
                walrus_assert_msg(s_ctx->initialized, "This shouldn't happen! Bad synchronization?");
                walrus_trace("Exec Command: RendererShutdownBegin");
                s_ctx->initialized = false;
                break;
            case COMMAND_RENDERER_SHUTDOWN_END:
                walrus_assert_msg(!s_ctx->initialized && !s_ctx->exit, "This shouldn't happen! Bad synchronization?");
                walrus_trace("Exec Command: RendererShutdownEnd");

                renderer_destroy();
                s_ctx->exit = true;
                break;
            case COMMAND_END:
                end = true;
                break;
            case COMMAND_CREATE_VERTEX_LAYOUT: {
                Walrus_LayoutHandle handle;
                command_buffer_read(buffer, Walrus_LayoutHandle, &handle);
                Walrus_VertexLayout layout;
                command_buffer_read(buffer, Walrus_VertexLayout, &layout);

                s_renderer->vertex_layout_create_fn(handle, &layout);
            } break;
            case COMMAND_DESTROY_VERTEX_LAYOUT: {
                Walrus_LayoutHandle handle;
                command_buffer_read(buffer, Walrus_LayoutHandle, &handle);

                s_renderer->vertex_layout_destroy_fn(handle);
            } break;
            case COMMAND_CREATE_BUFFER: {
                Walrus_BufferHandle handle;
                command_buffer_read(buffer, Walrus_BufferHandle, &handle);
                void* data;
                command_buffer_read(buffer, void*, &data);
                u64 size;
                command_buffer_read(buffer, u64, &size);
                u16 flags;
                command_buffer_read(buffer, u16, &flags);

                s_renderer->buffer_create_fn(handle, data, size, flags);

                if (data) {
                    walrus_free(data);
                }
            } break;
            case COMMAND_DESTROY_BUFFER: {
                Walrus_BufferHandle handle;

                command_buffer_read(buffer, Walrus_BufferHandle, &handle);
                s_renderer->buffer_destroy_fn(handle);
            } break;
            case COMMAND_UPDATE_BUFFER: {
                Walrus_BufferHandle handle;
                command_buffer_read(buffer, Walrus_BufferHandle, &handle);
                u64 offset;
                command_buffer_read(buffer, u64, &offset);
                u64 size;
                command_buffer_read(buffer, u64, &size);
                void* data;
                command_buffer_read(buffer, void*, &data);

                s_renderer->buffer_update_fn(handle, offset, size, data);

                if (data) {
                    walrus_free(data);
                }

            } break;
            case COMMAND_CREATE_SHADER: {
                Walrus_ShaderHandle handle;
                command_buffer_read(buffer, Walrus_ShaderHandle, &handle);
                Walrus_ShaderType type;
                command_buffer_read(buffer, Walrus_ShaderType, &type);
                char* source;
                command_buffer_read(buffer, char*, &source);

                s_renderer->shader_create_fn(type, handle, source);

                if (source) {
                    walrus_free(source);
                }
            } break;
            case COMMAND_DESTROY_SHADER: {
                Walrus_ShaderHandle handle;
                command_buffer_read(buffer, Walrus_ShaderHandle, &handle);

                s_renderer->shader_destroy_fn(handle);
            } break;
            case COMMAND_CREATE_PROGRAM: {
                Walrus_ProgramHandle handle;
                command_buffer_read(buffer, Walrus_ProgramHandle, &handle);
                u32 num;
                command_buffer_read(buffer, u32, &num);
                Walrus_ShaderHandle* shaders = walrus_new(Walrus_ShaderHandle, num);
                command_buffer_read_data(buffer, shaders, sizeof(Walrus_ShaderHandle) * num);

                s_renderer->program_create_fn(handle, shaders, num);

                walrus_free(shaders);
            } break;
            case COMMAND_DESTROY_PROGRAM: {
                Walrus_ProgramHandle handle;
                command_buffer_read(buffer, Walrus_ProgramHandle, &handle);

                s_renderer->program_destroy_fn(handle);
            } break;
            case COMMAND_CREATE_TEXTURE: {
                Walrus_TextureHandle handle;
                command_buffer_read(buffer, Walrus_TextureHandle, &handle);
                Walrus_TextureCreateInfo info;
                command_buffer_read(buffer, Walrus_TextureCreateInfo, &info);
                void* data;
                command_buffer_read(buffer, void*, &data);

                s_renderer->texture_create_fn(handle, &info, data);

                if (data) {
                    walrus_free(data);
                }
            } break;
            case COMMAND_DESTROY_TEXTURE: {
                Walrus_TextureHandle handle;
                command_buffer_read(buffer, Walrus_TextureHandle, &handle);

                s_renderer->texture_destroy_fn(handle);
            } break;
            case COMMAND_RESIZE_TEXTURE: {
                Walrus_TextureHandle handle;
                command_buffer_read(buffer, Walrus_TextureHandle, &handle);
                u32 width;
                command_buffer_read(buffer, u32, &width);
                u32 height;
                command_buffer_read(buffer, u32, &height);
                u32 depth;
                command_buffer_read(buffer, u32, &depth);
                u8 num_mipmaps;
                command_buffer_read(buffer, u8, &num_mipmaps);
                u8 num_layers;
                command_buffer_read(buffer, u8, &num_layers);

                s_renderer->texture_resize_fn(handle, width, height, depth, num_layers, num_mipmaps);
            } break;
            case COMMAND_CREATE_UNIFORM: {
                Walrus_UniformHandle handle;
                command_buffer_read(buffer, Walrus_UniformHandle, &handle);
                u32 size;
                command_buffer_read(buffer, u32, &size);
                u8 len;
                command_buffer_read(buffer, u8, &len);
                char const* name = command_buffer_skip_data(buffer, len);

                s_renderer->uniform_create_fn(handle, name, size);
            } break;
            case COMMAND_DESTROY_UNIFORM: {
                Walrus_UniformHandle handle;
                command_buffer_read(buffer, Walrus_UniformHandle, &handle);

                s_renderer->uniform_destroy_fn(handle);
            } break;
            case COMMAND_RESIZE_UNIFORM: {
                Walrus_UniformHandle handle;
                command_buffer_read(buffer, Walrus_UniformHandle, &handle);
                u32 size;
                command_buffer_read(buffer, u32, &size);

                s_renderer->uniform_resize_fn(handle, size);
            } break;
        }
    } while (!end);
}

static Walrus_TransientBuffer* create_transient_buffer(u64 size, u16 flags)
{
    Walrus_BufferHandle     handle = {walrus_handle_alloc(s_ctx->buffers)};
    Walrus_TransientBuffer* tvb    = NULL;
    if (handle.id == WR_INVALID_HANDLE) {
        s_ctx->err = WR_RHI_ALLOC_HADNLE_ERROR;
        return tvb;
    }

    CommandBuffer* cmdbuf = get_command_buffer(COMMAND_CREATE_BUFFER);
    void*          data   = NULL;
    command_buffer_write(cmdbuf, Walrus_BufferHandle, &handle);
    command_buffer_write(cmdbuf, void const*, &data);
    command_buffer_write(cmdbuf, u64, &size);
    command_buffer_write(cmdbuf, u16, &flags);

    u64 const tvb_size = walrus_align_up(sizeof(Walrus_TransientBuffer), 16) + walrus_align_up(size, 16);
    tvb                = (Walrus_TransientBuffer*)walrus_malloc(tvb_size);
    tvb->data          = (uint8_t*)tvb + walrus_align_up(sizeof(Walrus_TransientBuffer), 16);
    tvb->size          = size;
    tvb->offset        = 0;
    tvb->stride        = 0;
    tvb->handle        = handle;
    return tvb;
}

static void destroy_transient_buffer(Walrus_TransientBuffer* buffer)
{
    walrus_assert(free_handle_queue(s_ctx->submit_frame->queue_buffer, buffer->handle));

    CommandBuffer* cmdbuf = get_command_buffer(COMMAND_DESTROY_BUFFER);
    command_buffer_write(cmdbuf, Walrus_BufferHandle, &buffer->handle);

    walrus_free(buffer);
}

static void init_resources(void)
{
    frame_init(s_ctx->submit_frame, WR_RHI_MIN_RESOURCE_COMMAND_BUFFER_SIZE, WR_RHI_MIN_TRANSIENT_BUFFER_SIZE,
               WR_RHI_MIN_TRANSIENT_INDEX_BUFFER_SIZE);
    if (!s_ctx->info.single_thread) {
        frame_init(s_ctx->render_frame, WR_RHI_MIN_RESOURCE_COMMAND_BUFFER_SIZE, WR_RHI_MIN_TRANSIENT_BUFFER_SIZE,
                   WR_RHI_MIN_TRANSIENT_INDEX_BUFFER_SIZE);
    }
    if (!s_ctx->info.single_thread) {
        s_ctx->api_sem    = walrus_semaphore_create();
        s_ctx->render_sem = walrus_semaphore_create();
    }

    handles_init();

    s_ctx->shader_map          = walrus_hash_table_create(walrus_str_hash, walrus_str_equal);
    s_ctx->uniform_map         = walrus_hash_table_create(walrus_str_hash, walrus_str_equal);
    s_ctx->vertex_layout_table = walrus_hash_table_create(walrus_direct_hash, walrus_direct_equal);
}

static void shutdown_resources(void)
{
    if (!s_ctx->info.single_thread) {
        walrus_semaphore_destroy(s_ctx->api_sem);
        walrus_semaphore_destroy(s_ctx->render_sem);
    }

    walrus_hash_table_destroy(s_ctx->shader_map);
    walrus_hash_table_destroy(s_ctx->uniform_map);
    walrus_hash_table_destroy(s_ctx->vertex_layout_table);

    handles_shutdown();
    frame_shutdown(s_ctx->submit_frame);
    if (!s_ctx->info.single_thread) {
        frame_shutdown(s_ctx->render_frame);
    }
}

Walrus_RhiError walrus_rhi_init(Walrus_RhiCreateInfo* info)
{
    s_ctx = walrus_malloc(sizeof(RhiContext));

    s_ctx->info         = *info;
    s_ctx->exit         = false;
    s_ctx->initialized  = false;
    s_ctx->api_sem      = NULL;
    s_ctx->render_sem   = NULL;
    s_ctx->submit_frame = &s_ctx->frames[0];
    s_ctx->render_frame = info->single_thread ? &s_ctx->frames[0] : &s_ctx->frames[1];

    s_ctx->uniform_begin = 0;
    s_ctx->uniform_end   = 0;

    init_resources();

    for (u32 i = 0; i < WR_RHI_MAX_UNIFORMS; ++i) {
        s_ctx->uniform_refs[i].ref_count = 0;
    }
    for (u32 i = 0; i < WR_RHI_MAX_VERTEX_LAYOUTS; ++i) {
        s_ctx->vertex_layout_ref[i].ref_count = 0;
    }
    for (u32 i = 0; i < WR_RHI_MAX_SHADERS; ++i) {
        s_ctx->shader_refs[i].ref_count = 0;
    }

    for (u32 i = 0; i < walrus_array_len(s_ctx->views); ++i) {
        view_reset(&s_ctx->views[i]);
    }

    for (u32 i = 0; i < walrus_array_len(s_ctx->view_map); ++i) {
        s_ctx->view_map[i] = i;
    }

    discard(WR_RHI_DISCARD_ALL);

    CommandBuffer* buf = get_command_buffer(COMMAND_RENDERER_INIT);
    command_buffer_write(buf, Walrus_RhiCreateInfo, info);

    frame_no_render_wait();

    walrus_rhi_frame();

    if (!s_ctx->initialized) {
        get_command_buffer(COMMAND_RENDERER_SHUTDOWN_END);
        walrus_rhi_frame();
        walrus_rhi_frame();

        shutdown_resources();

        return WR_RHI_INIT_ERROR;
    }

    s_ctx->submit_frame->transient_vb =
        create_transient_buffer(s_ctx->submit_frame->max_transient_vb, WR_RHI_BUFFER_NONE);
    s_ctx->submit_frame->transient_ib =
        create_transient_buffer(s_ctx->submit_frame->max_transient_ib, WR_RHI_BUFFER_INDEX);
    walrus_rhi_frame();

    if (!info->single_thread) {
        s_ctx->submit_frame->transient_vb =
            create_transient_buffer(s_ctx->submit_frame->max_transient_vb, WR_RHI_BUFFER_NONE);
        s_ctx->submit_frame->transient_ib =
            create_transient_buffer(s_ctx->submit_frame->max_transient_ib, WR_RHI_BUFFER_INDEX);
        walrus_rhi_frame();
    }

    return s_ctx->err;
}

void walrus_rhi_shutdown(void)
{
    get_command_buffer(COMMAND_RENDERER_SHUTDOWN_BEGIN);
    walrus_rhi_frame();

    destroy_transient_buffer(s_ctx->submit_frame->transient_vb);
    destroy_transient_buffer(s_ctx->submit_frame->transient_ib);
    walrus_rhi_frame();

    if (!s_ctx->info.single_thread) {
        destroy_transient_buffer(s_ctx->submit_frame->transient_vb);
        destroy_transient_buffer(s_ctx->submit_frame->transient_ib);
        walrus_rhi_frame();
    }

    walrus_rhi_frame();

    get_command_buffer(COMMAND_RENDERER_SHUTDOWN_END);
    walrus_rhi_frame();

    render_sem_wait(-1);  // Waiting for RenderShutdown to finish

    shutdown_resources();

    walrus_free(s_ctx);
    s_ctx = NULL;
}

char const* walrus_rhi_error_msg(void)
{
    if (s_ctx == NULL) {
        return WR_RHI_ALLOC_FAIL_STR;
    }
    else {
        return s_ctx->err_msg;
    }
}

void walrus_rhi_set_resolution(u32 width, u32 height)
{
    s_ctx->resolution.width  = width;
    s_ctx->resolution.height = height;
}

void walrus_rhi_frame(void)
{
    render_sem_wait(-1);
    frame_no_render_wait();
}

Walrus_RenderResult walrus_rhi_render_frame(i32 ms)
{
    if (s_ctx == NULL) {
        return WR_RHI_RENDER_NO_CONTEXT;
    }
    if (api_sem_wait((ms))) {
        render_exec_command(&s_ctx->render_frame->cmd_pre);
        {
            if (s_ctx->initialized) {
                s_renderer->submit_fn(s_ctx->render_frame);
            }
        }
        render_exec_command(&s_ctx->render_frame->cmd_post);
        render_sem_post();
    }
    else {
        return WR_RHI_RENDER_TIMEOUT;
    }
    Walrus_RenderResult res = s_ctx->exit ? WR_RHI_RENDER_EXITING : WR_RHI_RENDER_FRAME;
    return res;
}

void walrus_rhi_submit(u16 view_id, Walrus_ProgramHandle program, u32 depth, u8 flags)
{
    RenderFrame* frame = s_ctx->submit_frame;

    u32 const render_item_id = frame->num_render_items;
    frame->num_render_items  = walrus_min(WR_RHI_MAX_DRAW_CALLS, walrus_u32satadd(frame->num_render_items, 1));

    s_ctx->uniform_end        = frame->uniforms->pos;
    s_ctx->draw.uniform_begin = s_ctx->uniform_begin;
    s_ctx->draw.uniform_end   = s_ctx->uniform_end;

    s_ctx->key.view_id = view_id;
    s_ctx->key.program = program;

    SortKeyType type;
    switch (s_ctx->views[view_id].mode) {
        case WR_RHI_VIEWMODE_SEQUENTIAL:
            s_ctx->key.sequence = s_ctx->seqs[view_id]++;
            type                = SORT_SEQUENCE;
            break;
        case WR_RHI_VIEWMODE_DEPTH_ASCENDING:
            s_ctx->key.depth = depth;
            type             = SORT_DEPTH;
            break;
        case WR_RHI_VIEWMODE_DEPTH_DESCENDING:
            s_ctx->key.depth = UINT32_MAX - depth;
            type             = SORT_DEPTH;
            break;
        default:
            s_ctx->key.depth = depth;
            type             = SORT_PROGRAM;
            break;
    }
    u64 key_val = sortkey_encode_draw(&s_ctx->key, type);

    frame->sortkeys[render_item_id]   = key_val;
    frame->sortvalues[render_item_id] = render_item_id;

    u16 stream_mask = s_ctx->draw.stream_mask;
    if (stream_mask != UINT16_MAX) {
        u32 num_vertices = UINT32_MAX;

        for (u32 id = 0; 0 != stream_mask; stream_mask >>= 1, ++id) {
            u32 const ntz = walrus_u32cnttz(stream_mask);
            stream_mask >>= ntz;
            id += ntz;

            num_vertices = walrus_min(num_vertices, s_ctx->num_vertices[id]);
        }
        s_ctx->draw.num_vertices = num_vertices;
    }
    else {
        // set_vertex_count
        s_ctx->draw.num_vertices = s_ctx->num_vertices[0];
    }

    frame->render_items[render_item_id].draw = s_ctx->draw;
    frame->render_binds[render_item_id]      = s_ctx->bind;

    draw_clear(&s_ctx->draw, flags);
    bind_clear(&s_ctx->bind, flags);

    if (flags & WR_RHI_DISCARD_STATE) {
        s_ctx->uniform_begin = s_ctx->uniform_end;
    }
}

u32 walrus_rhi_compose_rgba(u8 r, u8 g, u8 b, u8 a)
{
    return (u32)(r) << 24 | (u32)(g) << 16 | (u32)(b) << 8 | (u32)(a) << 0;
}

void walrus_rhi_decompose_rgba(u32 rgba, u8* r, u8* g, u8* b, u8* a)
{
    *r = (u8)(rgba >> 24);
    *g = (u8)(rgba >> 16);
    *b = (u8)(rgba >> 8);
    *a = (u8)(rgba >> 0);
}

void walrus_rhi_set_state(u64 state, u32 rgba)
{
    u8 const blend = ((state & WR_RHI_STATE_BLEND_MASK) >> WR_RHI_STATE_BLEND_SHIFT) & 0xff;

    // Transparency sort order table:
    //
    //                            +----------------------------------------- WR_RHI_STATE_BLEND_ZERO
    //                            |  +-------------------------------------- WR_RHI_STATE_BLEND_ONE
    //                            |  |  +----------------------------------- WR_RHI_STATE_BLEND_SRC_COLOR
    //                            |  |  |  +-------------------------------- WR_RHI_STATE_BLEND_INV_SRC_COLOR
    //                            |  |  |  |  +----------------------------- WR_RHI_STATE_BLEND_SRC_ALPHA
    //                            |  |  |  |  |  +-------------------------- WR_RHI_STATE_BLEND_INV_SRC_ALPHA
    //                            |  |  |  |  |  |  +----------------------- WR_RHI_STATE_BLEND_DST_ALPHA
    //                            |  |  |  |  |  |  |  +-------------------- WR_RHI_STATE_BLEND_INV_DST_ALPHA
    //                            |  |  |  |  |  |  |  |  +----------------- WR_RHI_STATE_BLEND_DST_COLOR
    //                            |  |  |  |  |  |  |  |  |  +-------------- WR_RHI_STATE_BLEND_INV_DST_COLOR
    //                            |  |  |  |  |  |  |  |  |  |  +----------- WR_RHI_STATE_BLEND_SRC_ALPHA_SAT
    //                            |  |  |  |  |  |  |  |  |  |  |  +-------- WR_RHI_STATE_BLEND_FACTOR
    //                            |  |  |  |  |  |  |  |  |  |  |  |  +----- WR_RHI_STATE_BLEND_INV_FACTOR
    //                            |  |  |  |  |  |  |  |  |  |  |  |  |
    //                            x  |  |  |  |  |  |  |  |  |  |  |  |  |  x  x  x  x  x
    s_ctx->key.blend         = "\x0\x2\x2\x3\x3\x2\x3\x2\x3\x2\x2\x2\x2\x2\x2\x2\x2\x2\x2"[((blend)&0xf) + (!!blend)];
    s_ctx->draw.state_flags  = state;
    s_ctx->draw.blend_factor = rgba;
}

void walrus_rhi_set_stencil(u32 fstencil, u32 bstencil)
{
    s_ctx->draw.stencil = pack_stencil(fstencil, bstencil);
}

void walrus_rhi_set_view_rect(u16 view_id, i32 x, i32 y, u32 width, u32 height)
{
    width            = walrus_max(width, 1);
    height           = walrus_max(height, 1);
    RenderView* view = &s_ctx->views[view_id];

    view->viewport.x      = x;
    view->viewport.y      = y;
    view->viewport.width  = width;
    view->viewport.height = height;
}

void walrus_rhi_set_view_clear(u16 view_id, u16 flags, u32 rgba, f32 depth, u8 stencil)
{
    RenderClear* clear = &s_ctx->views[view_id].clear;

    walrus_rhi_decompose_rgba(rgba, &clear->index[0], &clear->index[1], &clear->index[2], &clear->index[3]);
    clear->flags   = flags;
    clear->depth   = depth;
    clear->stencil = stencil;
}

void walrus_rhi_set_view_transform(u16 view_id, mat4 view, mat4 projection)
{
    RenderView* v = &s_ctx->views[view_id];
    if (view) {
        glm_mat4_copy(view, v->view);
    }
    if (projection) {
        glm_mat4_copy(projection, v->projection);
    }
}

void walrus_rhi_set_view_mode(u16 view_id, Walrus_ViewMode mode)
{
    s_ctx->views[view_id].mode = mode;
}

void walrus_rhi_screen_to_clip(u16 view_id, vec2 const screen, vec2 clip)
{
    RenderView const* v    = &s_ctx->views[view_id];
    ViewRect const*   rect = &v->viewport;
    clip[0]                = 2.f * (screen[0] - rect->x) / rect->width - 1.f;
    clip[1]                = 1.f - 2.f * (screen[1] - rect->y) / rect->height;
}

void walrus_rhi_screen_to_world(u16 view_id, vec2 const screen, vec3 world)
{
    RenderView const* v = &s_ctx->views[view_id];

    vec4 clip = {0, 0, -1, 1.0};
    walrus_rhi_screen_to_clip(view_id, screen, clip);

    mat4 inv_vp;
    glm_mat4_mul((vec4*)v->projection, (vec4*)v->view, inv_vp);
    glm_mat4_inv(inv_vp, inv_vp);

    glm_mat4_mulv(inv_vp, clip, clip);
    glm_vec3_scale(clip, 1.0 / clip[3], world);
}

void walrus_rhi_screen_to_world_dir(u16 view_id, vec2 const screen, vec3 world_dir)
{
    RenderView const* v = &s_ctx->views[view_id];

    vec4 near_clip = {0, 0, -1, 1.0};
    vec4 far_clip  = {0, 0, 1, 1.0};
    walrus_rhi_screen_to_clip(view_id, screen, near_clip);
    walrus_rhi_screen_to_clip(view_id, screen, far_clip);

    mat4 inv_vp;
    glm_mat4_mul((vec4*)v->projection, (vec4*)v->view, inv_vp);
    glm_mat4_inv(inv_vp, inv_vp);

    glm_mat4_mulv(inv_vp, near_clip, near_clip);
    glm_mat4_mulv(inv_vp, far_clip, far_clip);

    glm_vec4_scale(near_clip, 1.0 / near_clip[3], near_clip);
    glm_vec4_scale(far_clip, 1.0 / far_clip[3], far_clip);

    glm_vec3_sub(far_clip, near_clip, world_dir);
    glm_vec3_normalize(world_dir);
}

void walrus_rhi_set_transform(mat4 const transform)
{
    u32 num                  = 1;
    s_ctx->draw.start_matrix = frame_add_matrices(s_ctx->submit_frame, transform, &num);
    s_ctx->draw.num_matrices = num;
}

static void shader_inc_ref(Walrus_ShaderHandle handle)
{
    ++s_ctx->shader_refs[handle.id].ref_count;
}

static void shader_dec_ref(Walrus_ShaderHandle handle)
{
    ShaderRef* ref = &s_ctx->shader_refs[handle.id];
    if (ref->ref_count > 0) {
        --s_ctx->shader_refs[handle.id].ref_count;
        if (s_ctx->shader_refs[handle.id].ref_count == 0) {
            walrus_assert(free_handle_queue(s_ctx->submit_frame->queue_shader, handle));

            CommandBuffer* cmdbuf = get_command_buffer(COMMAND_DESTROY_SHADER);
            command_buffer_write(cmdbuf, Walrus_ShaderHandle, &handle);

            walrus_hash_table_remove(s_ctx->shader_map, s_ctx->shader_refs[handle.id].source);

            walrus_str_free(s_ctx->shader_refs[handle.id].source);
        }
    }
}

Walrus_ShaderHandle walrus_rhi_create_shader(Walrus_ShaderType type, char const* source)
{
    Walrus_ShaderHandle handle;

    if (walrus_hash_table_contains(s_ctx->shader_map, source)) {
        handle.id = walrus_ptr_to_val(walrus_hash_table_lookup(s_ctx->shader_map, source));
        shader_inc_ref(handle);
    }
    else {
        handle = (Walrus_ShaderHandle){walrus_handle_alloc(s_ctx->shaders)};
        if (handle.id == WR_INVALID_HANDLE) {
            s_ctx->err = WR_RHI_ALLOC_HADNLE_ERROR;
            return handle;
        }

        CommandBuffer* cmdbuf = get_command_buffer(COMMAND_CREATE_SHADER);
        command_buffer_write(cmdbuf, Walrus_ShaderHandle, &handle);
        command_buffer_write(cmdbuf, Walrus_ShaderType, &type);
        char* mem = walrus_memdup(source, walrus_str_len(source) + 1);
        command_buffer_write(cmdbuf, char*, &mem);

        ShaderRef* ref = &s_ctx->shader_refs[handle.id];
        ref->ref_count = 1;
        ref->source    = walrus_str_dup(source);
        walrus_hash_table_insert(s_ctx->shader_map, ref->source, walrus_val_to_ptr(handle.id));
    }

    return handle;
}

void walrus_rhi_destroy_shader(Walrus_ShaderHandle handle)
{
    if (handle.id == WR_INVALID_HANDLE) {
        return;
    }

    shader_dec_ref(handle);
}

Walrus_ProgramHandle walrus_rhi_create_program(Walrus_ShaderHandle* shaders, u32 num, bool destroy_shader)
{
    Walrus_ProgramHandle handle = {walrus_handle_alloc(s_ctx->programs)};
    if (handle.id == WR_INVALID_HANDLE) {
        s_ctx->err = WR_RHI_ALLOC_HADNLE_ERROR;
        return handle;
    }
    for (u32 i = 0; i < num; ++i) {
        shader_inc_ref(shaders[i]);
    }

    CommandBuffer* cmdbuf = get_command_buffer(COMMAND_CREATE_PROGRAM);
    command_buffer_write(cmdbuf, Walrus_ProgramHandle, &handle);
    command_buffer_write(cmdbuf, u32, &num);
    command_buffer_write_data(cmdbuf, shaders, num * sizeof(Walrus_ShaderHandle));

    if (destroy_shader) {
        for (u32 i = 0; i < num; ++i) {
            shader_dec_ref(shaders[i]);
        }
    }

    return handle;
}

void walrus_rhi_destroy_program(Walrus_ProgramHandle handle)
{
    if (handle.id == WR_INVALID_HANDLE) {
        return;
    }
    walrus_assert(free_handle_queue(s_ctx->submit_frame->queue_program, handle));

    CommandBuffer* cmdbuf = get_command_buffer(COMMAND_DESTROY_PROGRAM);
    command_buffer_write(cmdbuf, Walrus_ProgramHandle, &handle);
}

static u16 get_uniform_size(Walrus_UniformType type)
{
    switch (type) {
        case WR_RHI_UNIFORM_SAMPLER:
        case WR_RHI_UNIFORM_INT:
        case WR_RHI_UNIFORM_UINT:
        case WR_RHI_UNIFORM_BOOL:
            return sizeof(int32_t);
        case WR_RHI_UNIFORM_FLOAT:
            return sizeof(float);
        case WR_RHI_UNIFORM_VEC2:
            return 2 * sizeof(float);
        case WR_RHI_UNIFORM_VEC3:
            return 3 * sizeof(float);
        case WR_RHI_UNIFORM_VEC4:
            return 4 * sizeof(float);
        case WR_RHI_UNIFORM_MAT3:
            return 3 * 3 * sizeof(float);
        case WR_RHI_UNIFORM_MAT4:
            return 4 * 4 * sizeof(float);
        case WR_RHI_UNIFORM_COUNT:
            return 0;
    }
    return 0;
}

Walrus_UniformHandle walrus_rhi_create_uniform(char const* name, Walrus_UniformType type, i8 num)
{
    Walrus_UniformHandle handle = {WR_INVALID_HANDLE};

    u32 const size = num * get_uniform_size(type);
    if (walrus_hash_table_contains(s_ctx->uniform_map, name)) {
        handle.id       = walrus_ptr_to_val(walrus_hash_table_lookup(s_ctx->uniform_map, name));
        UniformRef* ref = &s_ctx->uniform_refs[handle.id];
        if (ref->size < size) {
            CommandBuffer* cmdbuf = get_command_buffer(COMMAND_RESIZE_UNIFORM);
            command_buffer_write(cmdbuf, Walrus_UniformHandle, &handle);
            command_buffer_write(cmdbuf, u32, &size);
            ref->size = size;
        }

        ++ref->ref_count;
    }
    else {
        handle = (Walrus_UniformHandle){walrus_handle_alloc(s_ctx->uniforms)};
        if (handle.id == WR_INVALID_HANDLE) {
            s_ctx->err = WR_RHI_ALLOC_HADNLE_ERROR;
            return handle;
        }

        UniformRef* ref = &s_ctx->uniform_refs[handle.id];
        ref->name       = walrus_str_dup(name);
        ref->type       = type;
        ref->size       = size;
        ref->ref_count  = 1;
        walrus_hash_table_insert(s_ctx->uniform_map, ref->name, walrus_val_to_ptr(handle.id));

        CommandBuffer* cmdbuf = get_command_buffer(COMMAND_CREATE_UNIFORM);
        command_buffer_write(cmdbuf, Walrus_UniformHandle, &handle);
        command_buffer_write(cmdbuf, u32, &size);
        u8 len = walrus_str_len(name) + 1;
        command_buffer_write(cmdbuf, u8, &len);
        command_buffer_write_data(cmdbuf, name, len);
    }

    return handle;
}

void walrus_rhi_destroy_uniform(Walrus_UniformHandle handle)
{
    if (handle.id == WR_INVALID_HANDLE) {
        return;
    }

    UniformRef* ref = &s_ctx->uniform_refs[handle.id];
    if (ref->ref_count > 0) {
        --ref->ref_count;
        if (ref->ref_count == 0) {
            walrus_assert(walrus_hash_table_remove(s_ctx->uniform_map, ref->name));

            walrus_str_free(ref->name);
            walrus_assert(free_handle_queue(s_ctx->submit_frame->queue_uniform, handle));

            CommandBuffer* cmdbuf = get_command_buffer(COMMAND_DESTROY_UNIFORM);
            command_buffer_write(cmdbuf, Walrus_UniformHandle, &handle);
        }
    }
}

void walrus_rhi_set_uniform(Walrus_UniformHandle handle, u32 offset, u32 size, void const* data)
{
    UniformRef* ref = &s_ctx->uniform_refs[handle.id];
    if (ref->ref_count > 0) {
        uniform_buffer_update(&s_ctx->submit_frame->uniforms, 64 << 10, 1 << 20);
        uniform_buffer_write_uniform(s_ctx->submit_frame->uniforms, ref->type, handle, offset, size, data);
    }
    else {
        walrus_error("Cannot find valid uniform!");
    }
}

static Walrus_LayoutHandle find_or_create_vertex_layout(Walrus_VertexLayout const* layout)
{
    Walrus_LayoutHandle handle;
    if (walrus_hash_table_contains(s_ctx->vertex_layout_table, walrus_val_to_ptr(layout->hash))) {
        handle.id =
            walrus_ptr_to_val(walrus_hash_table_lookup(s_ctx->vertex_layout_table, walrus_val_to_ptr(layout->hash)));
        ++s_ctx->vertex_layout_ref[handle.id].ref_count;

        return handle;
    }

    handle = (Walrus_LayoutHandle){walrus_handle_alloc(s_ctx->vertex_layouts)};
    if (handle.id == WR_INVALID_HANDLE) {
        s_ctx->err = WR_RHI_ALLOC_HADNLE_ERROR;

        return handle;
    }

    CommandBuffer* cmdbuf = get_command_buffer(COMMAND_CREATE_VERTEX_LAYOUT);
    command_buffer_write(cmdbuf, Walrus_LayoutHandle, &handle);
    command_buffer_write(cmdbuf, Walrus_VertexLayout, layout);

    walrus_hash_table_insert(s_ctx->vertex_layout_table, walrus_val_to_ptr(layout->hash), walrus_val_to_ptr(handle.id));
    s_ctx->vertex_layout_ref[handle.id].hash      = layout->hash;
    s_ctx->vertex_layout_ref[handle.id].ref_count = 1;

    return handle;
}

Walrus_LayoutHandle walrus_rhi_create_vertex_layout(Walrus_VertexLayout const* layout)
{
    Walrus_LayoutHandle handle = find_or_create_vertex_layout(layout);
    if (handle.id == WR_INVALID_HANDLE) {
        s_ctx->err = WR_RHI_ALLOC_HADNLE_ERROR;

        return handle;
    }

    return handle;
}

void walrus_rhi_destroy_vertex_layout(Walrus_LayoutHandle handle)
{
    if (handle.id == WR_INVALID_HANDLE) {
        return;
    }

    if (s_ctx->vertex_layout_ref[handle.id].ref_count == 0) {
        return;
    }

    --s_ctx->vertex_layout_ref[handle.id].ref_count;
    if (s_ctx->vertex_layout_ref[handle.id].ref_count == 0) {
        walrus_hash_table_remove(s_ctx->vertex_layout_table,
                                 walrus_val_to_ptr(s_ctx->vertex_layout_ref[handle.id].hash));
        walrus_assert(free_handle_queue(s_ctx->submit_frame->queue_layout, handle));

        CommandBuffer* cmdbuf = get_command_buffer(COMMAND_DESTROY_VERTEX_LAYOUT);
        command_buffer_write(cmdbuf, Walrus_LayoutHandle, &handle);
    }
}

Walrus_BufferHandle walrus_rhi_create_buffer(void const* data, u64 size, u16 flags)
{
    Walrus_BufferHandle handle = {walrus_handle_alloc(s_ctx->buffers)};
    if (handle.id == WR_INVALID_HANDLE) {
        s_ctx->err = WR_RHI_ALLOC_HADNLE_ERROR;
        return handle;
    }

    CommandBuffer* cmdbuf = get_command_buffer(COMMAND_CREATE_BUFFER);
    command_buffer_write(cmdbuf, Walrus_BufferHandle, &handle);
    void* new_data = walrus_memdup(data, size);
    command_buffer_write(cmdbuf, void const*, &new_data);
    command_buffer_write(cmdbuf, u64, &size);
    command_buffer_write(cmdbuf, u16, &flags);

    return handle;
}

void walrus_rhi_destroy_buffer(Walrus_BufferHandle handle)
{
    if (handle.id == WR_INVALID_HANDLE) {
        return;
    }

    walrus_assert(free_handle_queue(s_ctx->submit_frame->queue_buffer, handle));

    CommandBuffer* cmdbuf = get_command_buffer(COMMAND_DESTROY_BUFFER);
    command_buffer_write(cmdbuf, Walrus_BufferHandle, &handle);
}

void walrus_rhi_update_buffer(Walrus_BufferHandle handle, u64 offset, u64 size, void const* data)
{
    if (handle.id == WR_INVALID_HANDLE) {
        return;
    }

    CommandBuffer* cmdbuf = get_command_buffer(COMMAND_UPDATE_BUFFER);
    command_buffer_write(cmdbuf, Walrus_BufferHandle, &handle);
    command_buffer_write(cmdbuf, u64, &offset);
    command_buffer_write(cmdbuf, u64, &size);
    void* new_data = walrus_memdup(data, size);
    command_buffer_write(cmdbuf, void*, &new_data);
}

static bool set_stream_bit(RenderDraw* draw, u8 stream, Walrus_BufferHandle handle)
{
    u16 const bit     = 1 << stream;
    u16 const mask    = draw->stream_mask & ~bit;
    u16 const tmp     = handle.id != WR_INVALID_HANDLE ? bit : 0;
    draw->stream_mask = mask | tmp;
    return tmp != 0;
}

void walrus_rhi_set_vertex_count(u32 num_vertices)
{
    walrus_assert_msg(0 == s_ctx->draw.stream_mask, "set_vertex_buffer was already called for this draw call.");
    s_ctx->draw.stream_mask  = UINT16_MAX;
    VertexStream* stream     = &s_ctx->draw.streams[0];
    stream->offset           = 0;
    stream->handle.id        = WR_INVALID_HANDLE;
    stream->layout_handle.id = WR_INVALID_HANDLE;
    s_ctx->num_vertices[0]   = num_vertices;
}

void walrus_rhi_set_vertex_buffer(u8 stream_id, Walrus_BufferHandle handle, Walrus_LayoutHandle layout_handle,
                                  u32 offset, u32 num_vertices)
{
    walrus_assert(handle.id != WR_INVALID_HANDLE);
    if (set_stream_bit(&s_ctx->draw, stream_id, handle)) {
        VertexStream* stream           = &s_ctx->draw.streams[stream_id];
        stream->offset                 = offset;
        stream->handle                 = handle;
        stream->layout_handle          = layout_handle;
        s_ctx->num_vertices[stream_id] = num_vertices;
    }
}

void walrus_rhi_set_transient_buffer(u8 stream_id, Walrus_TransientBuffer* buffer, Walrus_LayoutHandle layout_handle,
                                     u32 offset, u32 num_vertices)
{
    walrus_assert(buffer->handle.id != WR_INVALID_HANDLE);
    if (set_stream_bit(&s_ctx->draw, stream_id, buffer->handle)) {
        VertexStream* stream           = &s_ctx->draw.streams[stream_id];
        stream->offset                 = offset + buffer->offset;
        stream->handle                 = buffer->handle;
        stream->layout_handle          = layout_handle;
        s_ctx->num_vertices[stream_id] = walrus_clamp(0, (buffer->size - offset) / buffer->stride, num_vertices);
    }
}
void walrus_rhi_set_instance_buffer(Walrus_BufferHandle handle, Walrus_LayoutHandle layout_handle, u32 offset,
                                    u32 num_instance)
{
    walrus_assert(handle.id != WR_INVALID_HANDLE);

    s_ctx->draw.instance_buffer = handle;
    s_ctx->draw.instance_layout = layout_handle;
    s_ctx->draw.instance_offset = offset;
    s_ctx->draw.num_instances   = num_instance;
}

void walrus_rhi_set_transient_instance_buffer(Walrus_TransientBuffer* buffer, Walrus_LayoutHandle layout_handle,
                                              u32 offset, u32 num_instance)
{
    walrus_assert(buffer->handle.id != WR_INVALID_HANDLE);

    s_ctx->draw.instance_buffer = buffer->handle;
    s_ctx->draw.instance_layout = layout_handle;
    s_ctx->draw.instance_offset = offset + buffer->offset;
    s_ctx->draw.num_instances   = walrus_clamp(0, (buffer->size - offset) / buffer->stride, num_instance);
}

void walrus_rhi_set_index_buffer(Walrus_BufferHandle handle, u32 offset, u32 num_indices)
{
    s_ctx->draw.index_buffer = handle;
    s_ctx->draw.index_size   = sizeof(u16);
    s_ctx->draw.index_offset = offset;
    s_ctx->draw.num_indices  = num_indices;
}

void walrus_rhi_set_index32_buffer(Walrus_BufferHandle handle, u32 offset, u32 num_indices)
{
    s_ctx->draw.index_buffer = handle;
    s_ctx->draw.index_size   = sizeof(u32);
    s_ctx->draw.index_offset = offset;
    s_ctx->draw.num_indices  = num_indices;
}

void walrus_rhi_set_transient_index_buffer(Walrus_TransientBuffer* buffer, u32 offset, u32 num_indices)
{
    s_ctx->draw.index_buffer = buffer->handle;
    s_ctx->draw.index_size   = buffer->stride;
    s_ctx->draw.index_offset = offset + buffer->offset;
    s_ctx->draw.num_indices  = walrus_clamp(0, (buffer->size - offset) / buffer->stride, num_indices);
}

static u8 compute_mipmap(u32 width, u32 height)
{
    return 1 + floor(log2(walrus_max(width, height)));
}

static void resize_texture(Walrus_TextureHandle handle, u32 width, u32 height, u32 depth, u8 num_mipmaps, u8 num_layers)
{
    TextureRef* ref = &s_ctx->texture_refs[handle.id];
    if (ref->ratio != WR_RHI_RATIO_COUNT) {
        compute_texture_size_from_ratio(ref->ratio, &width, &height);
    }
    if (num_mipmaps == 0) {
        num_mipmaps = compute_mipmap(width, height);
    }
    ref->width       = width;
    ref->height      = height;
    ref->depth       = depth;
    ref->num_layers  = num_layers;
    ref->num_mipmaps = num_mipmaps;

    CommandBuffer* cmdbuf = get_command_buffer(COMMAND_UPDATE_TEXTURE);
    command_buffer_write(cmdbuf, Walrus_TextureHandle, &handle);
    command_buffer_write(cmdbuf, u32, &width);
    command_buffer_write(cmdbuf, u32, &height);
    command_buffer_write(cmdbuf, u32, &depth);
    command_buffer_write(cmdbuf, u8, &num_mipmaps);
    command_buffer_write(cmdbuf, u8, &num_layers);
}

static const u32 pixel_size[WR_RHI_FORMAT_COUNT] = {
    1,  // ALPHA8

    1,  // R8
    1,  // R8S
    4,  // R32I
    4,  // R32UI
    2,  // R16F
    4,  // R32F

    2,  // RG8
    2,  // RG8S
    8,  // RG32I
    8,  // RG32UI
    4,  // RG16F
    8,  // RG32F

    3,   // RGB8
    3,   // RGB8S
    12,  // RG32I
    12,  // RG32UI
    6,   // RG16F
    12,  // RG32F

    4,   // RGBA8
    4,   // RGBA8S
    16,  // RGBA32I
    16,  // RGBA32UI
    8,   // RGBA16F
    16,  // RGBA32F

    3,  // DEPTH24
    1,  // STENCIL8
    4,  // DEPTH24STENCIL8
};

Walrus_TextureHandle walrus_rhi_create_texture(Walrus_TextureCreateInfo const* info, void const* data)
{
    Walrus_TextureHandle handle = (Walrus_TextureHandle){walrus_handle_alloc(s_ctx->textures)};
    if (handle.id == WR_INVALID_HANDLE) {
        s_ctx->err = WR_RHI_ALLOC_HADNLE_ERROR;
        return handle;
    }

    Walrus_TextureCreateInfo _info = *info;

    if (_info.ratio != WR_RHI_RATIO_COUNT) {
        compute_texture_size_from_ratio(_info.ratio, &_info.width, &_info.height);
    }

    // if no mip mode is selected, force num_mipmaps to be 1
    u32 const mip = (_info.flags & WR_RHI_SAMPLER_MIP_MASK) >> WR_RHI_SAMPLER_MIP_SHIFT;
    if (mip == 0) {
        _info.num_mipmaps = 1;
    }

    _info.num_mipmaps = _info.num_mipmaps == 0 ? compute_mipmap(_info.width, _info.height) : _info.num_mipmaps;

    u64   size     = _info.width * _info.height * _info.depth * _info.num_layers * pixel_size[_info.format];
    void* new_data = walrus_memdup(data, size);

    CommandBuffer* cmdbuf = get_command_buffer(COMMAND_CREATE_TEXTURE);
    command_buffer_write(cmdbuf, Walrus_TextureHandle, &handle);
    command_buffer_write(cmdbuf, Walrus_TextureCreateInfo, &_info);
    command_buffer_write(cmdbuf, void*, &new_data);

    TextureRef* ref  = &s_ctx->texture_refs[handle.id];
    ref->ratio       = _info.ratio;
    ref->width       = _info.width;
    ref->height      = _info.height;
    ref->depth       = _info.depth;
    ref->num_layers  = _info.num_layers;
    ref->num_mipmaps = _info.num_mipmaps;

    return handle;
}

Walrus_TextureHandle walrus_rhi_create_texture2d(u32 width, u32 height, Walrus_PixelFormat format, u8 mipmaps,
                                                 u64 flags, void const* data)
{
    Walrus_TextureCreateInfo info;
    info.width       = width;
    info.height      = height;
    info.depth       = 1;
    info.ratio       = WR_RHI_RATIO_COUNT;
    info.num_mipmaps = mipmaps;
    info.num_layers  = 1;
    info.format      = format;
    info.flags       = flags;
    info.cube_map    = false;
    return walrus_rhi_create_texture(&info, data);
}

Walrus_TextureHandle walrus_rhi_create_texture2d_ratio(Walrus_BackBufferRatio ratio, Walrus_PixelFormat format,
                                                       u8 mipmaps, u64 flags, void const* data)
{
    Walrus_TextureCreateInfo info;
    info.width       = 1;
    info.height      = 1;
    info.depth       = 1;
    info.ratio       = ratio;
    info.num_mipmaps = mipmaps;
    info.num_layers  = 1;
    info.format      = format;
    info.flags       = flags;
    info.cube_map    = false;

    return walrus_rhi_create_texture(&info, data);
}

void walrus_rhi_destroy_texture(Walrus_TextureHandle handle)
{
    if (handle.id == WR_INVALID_HANDLE) {
        s_ctx->err = WR_RHI_HANDLE_INVALID_ERROR;
        return;
    }
    walrus_assert(free_handle_queue(s_ctx->submit_frame->queue_texture, handle));

    CommandBuffer* cmdbuf = get_command_buffer(COMMAND_DESTROY_TEXTURE);
    command_buffer_write(cmdbuf, Walrus_TextureHandle, &handle);
}

void walrus_rhi_set_texture(u8 unit, Walrus_TextureHandle texture)
{
    if (unit >= WR_RHI_MAX_TEXTURE_SAMPLERS) {
        s_ctx->err = WR_RHI_TEXTURE_UNIT_ERROR;
        return;
    }

    Binding* bind = &s_ctx->bind.bindings[unit];
    bind->type    = WR_RHI_BIND_TEXTURE;
    bind->id      = texture.id;
}

void walrus_rhi_set_image(uint8_t unit, Walrus_TextureHandle handle, u8 mip, Walrus_DataAccess access,
                          Walrus_PixelFormat format)
{
    Binding* bind = &s_ctx->bind.bindings[unit];
    bind->type    = WR_RHI_BIND_IMAGE;
    bind->id      = handle.id;
    bind->mip     = (uint8_t)(mip);
    bind->format  = format;
    bind->access  = (uint8_t)(access);
}

u32 walrus_rhi_avail_transient_buffer(u32 num, u32 stride)
{
    return frame_avail_transient_vb_size(s_ctx->submit_frame, num, stride);
}

bool walrus_rhi_alloc_transient_buffer(Walrus_TransientBuffer* buffer, u32 num, u32 stride)
{
    if (num == walrus_rhi_avail_transient_buffer(num, stride)) {
        walrus_assert_msg(buffer != NULL, "buffer can't be NULL!");
        walrus_assert_msg(num > 0, "num must be greater than 0!");

        const uint32_t offset = frame_alloc_transient_vb(s_ctx->submit_frame, &num, stride);

        Walrus_TransientBuffer const* tvb = s_ctx->submit_frame->transient_vb;
        buffer->data                      = &tvb->data[offset];
        buffer->size                      = num * stride;
        buffer->offset                    = offset;
        buffer->stride                    = stride;
        buffer->handle                    = tvb->handle;
        return true;
    }
    return false;
}

u32 walrus_rhi_avail_transient_index_buffer(u32 num, u32 stride)
{
    return frame_avail_transient_ib_size(s_ctx->submit_frame, num, stride);
}

bool walrus_rhi_alloc_transient_index_buffer(Walrus_TransientBuffer* buffer, u32 num, u32 stride)
{
    if (num == walrus_rhi_avail_transient_index_buffer(num, stride)) {
        walrus_assert_msg(buffer != NULL, "buffer can't be NULL!");
        walrus_assert_msg(num > 0, "num must be greater than 0!");

        const uint32_t offset = frame_alloc_transient_ib(s_ctx->submit_frame, &num, stride);

        Walrus_TransientBuffer const* tib = s_ctx->submit_frame->transient_ib;
        buffer->data                      = &tib->data[offset];
        buffer->size                      = num * stride;
        buffer->offset                    = offset;
        buffer->stride                    = stride;
        buffer->handle                    = tib->handle;
        return true;
    }
    return false;
}

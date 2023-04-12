#include "rhi_p.h"
#include <core/macro.h>
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

static UniformAttribute const s_predefineds[] = {
    {WR_RHI_UNIFORM_MAT4, "u_view", 1}, {WR_RHI_UNIFORM_MAT4, "u_viewproj", 1}, {WR_RHI_UNIFORM_MAT4, "u_model", 1}};

static char const* no_backend_str = "No render backend specifed";

static RhiContext* s_ctx   = NULL;
static RhiVTable*  s_table = NULL;

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
            s_table->uniform_update_fn(handle, offset, size, data);
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

Walrus_RhiError walrus_rhi_init(Walrus_RhiFlag flags)
{
    s_ctx = walrus_malloc(sizeof(RhiContext));
    if (s_ctx == NULL) {
        return WR_RHI_ALLOC_ERROR;
    }

    s_table = walrus_malloc(sizeof(RhiVTable));
    if (s_table == NULL) {
        return WR_RHI_ALLOC_ERROR;
    }

    s_ctx->flags        = flags;
    s_ctx->submit_frame = &s_ctx->frames;

    s_ctx->uniform_begin = 0;
    s_ctx->uniform_end   = 0;

    if (s_ctx->flags & WR_RHI_FLAG_OPENGL) {
        gl_backend_init(s_ctx, s_table);
    }
    else {
        walrus_assert_msg(false, no_backend_str);
    }

    frame_init(s_ctx->submit_frame);
    handles_init();

    s_ctx->uniform_map             = walrus_hash_table_create(walrus_str_hash, walrus_str_equal);
    s_ctx->vertex_layout_ref.table = walrus_hash_table_create(walrus_direct_hash, walrus_direct_equal);

    for (u32 i = 0; i < WR_RHI_MAX_UNIFORMS; ++i) {
        s_ctx->uniform_refs[i].ref_count = 0;
    }
    for (u32 i = 0; i < WR_RHI_MAX_VERTEX_LAYOUTS; ++i) {
        s_ctx->vertex_layout_ref.ref_count[i] = 0;
    }

    discard(WR_RHI_DISCARD_ALL);

    return s_ctx->err;
}

void walrus_rhi_shutdown(void)
{
    walrus_hash_table_destroy(s_ctx->uniform_map);

    handles_shutdown();
    frame_shutdown(s_ctx->submit_frame);

    gl_backend_shutdown();

    walrus_free(s_ctx);
    walrus_free(s_table);

    s_ctx   = NULL;
    s_table = NULL;
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
    s_ctx->submit_frame->resolution.width  = width;
    s_ctx->submit_frame->resolution.height = height;
}

void walrus_rhi_frame(void)
{
    RenderFrame* frame = s_ctx->submit_frame;
    memcpy(frame->views, s_ctx->views, sizeof(s_ctx->views));

    frame_finish(frame);

    s_table->submit_fn(frame);

    s_ctx->uniform_begin = 0;
    s_ctx->uniform_end   = 0;

    frame_start(frame);
}

void walrus_rhi_submit(u16 view_id, Walrus_ProgramHandle program, u8 flags)
{
    RenderFrame* frame = s_ctx->submit_frame;

    u32 const render_item_id = frame->num_render_items;
    frame->num_render_items  = walrus_min(WR_RHI_MAX_DRAW_CALLS, walrus_u32satadd(frame->num_render_items, 1));

    s_ctx->uniform_end        = frame->uniforms->pos;
    s_ctx->draw.uniform_begin = s_ctx->uniform_begin;
    s_ctx->draw.uniform_end   = s_ctx->uniform_end;

    frame->program[render_item_id]  = program;
    frame->view_ids[render_item_id] = view_id;

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
    return (u32)(r) | (u32)(g) << 8 | (u32)(b) << 16 | (u32)(a) << 24;
}

void walrus_rhi_decompose_rgba(u32 rgba, u8* r, u8* g, u8* b, u8* a)
{
    *r = (u8)(rgba >> 0);
    *g = (u8)(rgba >> 8);
    *b = (u8)(rgba >> 16);
    *a = (u8)(rgba >> 24);
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
    Walrus_RenderClear* clear = &s_ctx->views[view_id].clear;

    walrus_rhi_decompose_rgba(rgba, &clear->index[0], &clear->index[1], &clear->index[2], &clear->index[3]);
    clear->flags   = flags;
    clear->depth   = depth;
    clear->stencil = stencil;
}

void walrus_rhi_set_view_transform(u16 view_id, mat4 view, mat4 projection)
{
    RenderView* v = &s_ctx->views[view_id];
    glm_mat4_copy(view, v->view);
    glm_mat4_copy(projection, v->projection);
}

void walrus_rhi_set_transform(mat4 const transform)
{
    u32 num                  = 1;
    s_ctx->draw.start_matrix = frame_add_matrices(s_ctx->submit_frame, transform, &num);
    s_ctx->draw.num_matrices = num;
}

Walrus_ShaderHandle walrus_rhi_create_shader(Walrus_ShaderType type, char const* source)
{
    Walrus_ShaderHandle handle = {walrus_handle_alloc(s_ctx->shaders)};
    if (handle.id == WR_INVALID_HANDLE) {
        s_ctx->err = WR_RHI_ALLOC_HADNLE_ERROR;
        return handle;
    }

    s_table->shader_create_fn(type, handle, source);

    return handle;
}

void walrus_rhi_destroy_shader(Walrus_ShaderHandle handle)
{
    if (handle.id == WR_INVALID_HANDLE) {
        return;
    }

    s_table->shader_destroy_fn(handle);

    walrus_handle_free(s_ctx->shaders, handle.id);
}

Walrus_ProgramHandle walrus_rhi_create_program(Walrus_ShaderHandle vs, Walrus_ShaderHandle fs)
{
    Walrus_ProgramHandle handle = {walrus_handle_alloc(s_ctx->programs)};
    if (handle.id == WR_INVALID_HANDLE) {
        s_ctx->err = WR_RHI_ALLOC_HADNLE_ERROR;
        return handle;
    }

    s_table->program_create_fn(handle, vs, fs, (Walrus_ShaderHandle){WR_INVALID_HANDLE});

    return handle;
}
void walrus_rhi_destroy_program(Walrus_ProgramHandle handle)
{
    if (handle.id == WR_INVALID_HANDLE) {
        return;
    }

    s_table->program_destroy_fn(handle);
    walrus_handle_free(s_ctx->programs, handle.id);
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
        handle.id       = walrus_ptr_to_u32(walrus_hash_table_lookup(s_ctx->uniform_map, name));
        UniformRef* ref = &s_ctx->uniform_refs[handle.id];
        if (ref->size < size) {
            // resize uniform
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
        walrus_hash_table_insert(s_ctx->uniform_map, ref->name, walrus_u32_to_ptr(handle.id));

        s_table->uniform_create_fn(handle, name, size);
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
            walrus_handle_free(s_ctx->uniforms, handle.id);

            s_table->uniform_destroy_fn(handle);
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

static Walrus_LayoutHandle find_or_create_vertex_layout(Walrus_VertexLayout const* layout, bool ref_on_create)
{
    Walrus_LayoutHandle handle;
    if (walrus_hash_table_contains(s_ctx->vertex_layout_ref.table, walrus_u32_to_ptr(layout->hash))) {
        handle.id = walrus_ptr_to_u32(
            walrus_hash_table_lookup(s_ctx->vertex_layout_ref.table, walrus_u32_to_ptr(layout->hash)));
        return handle;
    }

    handle = (Walrus_LayoutHandle){walrus_handle_alloc(s_ctx->vertex_layouts)};
    if (handle.id == WR_INVALID_HANDLE) {
        s_ctx->err = WR_RHI_ALLOC_HADNLE_ERROR;
        return handle;
    }

    s_table->vertex_layout_create_fn(handle, layout);

    if (ref_on_create) {
        walrus_hash_table_insert(s_ctx->vertex_layout_ref.table, walrus_u32_to_ptr(layout->hash),
                                 walrus_u32_to_ptr(handle.id));
        ++s_ctx->vertex_layout_ref.ref_count[handle.id];
    }

    return handle;
}

Walrus_LayoutHandle walrus_rhi_create_vertex_layout(Walrus_VertexLayout const* layout)
{
    Walrus_LayoutHandle handle = find_or_create_vertex_layout(layout, false);
    if (handle.id == WR_INVALID_HANDLE) {
        s_ctx->err = WR_RHI_ALLOC_HADNLE_ERROR;
        return handle;
    }

    walrus_hash_table_insert(s_ctx->vertex_layout_ref.table, walrus_u32_to_ptr(layout->hash),
                             walrus_u32_to_ptr(handle.id));
    ++s_ctx->vertex_layout_ref.ref_count[handle.id];

    return handle;
}

void walrus_rhi_destroy_vertex_layout(Walrus_LayoutHandle handle)
{
    s_table->vertex_layout_destroy_fn(handle);
}

Walrus_BufferHandle walrus_rhi_create_buffer(void const* data, u64 size, u16 flags)
{
    Walrus_BufferHandle handle = {walrus_handle_alloc(s_ctx->buffers)};
    if (handle.id == WR_INVALID_HANDLE) {
        s_ctx->err = WR_RHI_ALLOC_HADNLE_ERROR;
        return handle;
    }

    s_table->buffer_create_fn(handle, data, size, flags);

    return handle;
}

void walrus_rhi_destroy_buffer(Walrus_BufferHandle handle)
{
    if (handle.id == WR_INVALID_HANDLE) {
        return;
    }

    s_table->buffer_destroy_fn(handle);

    walrus_handle_free(s_ctx->buffers, handle.id);
}

static bool set_stream_bit(RenderDraw* draw, u8 stream, Walrus_BufferHandle handle)
{
    u16 const bit     = 1 << stream;
    u16 const mask    = draw->stream_mask & ~bit;
    u16 const tmp     = handle.id != WR_INVALID_HANDLE ? bit : 0;
    draw->stream_mask = mask | tmp;
    return tmp != 0;
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

static u8 calculteMipmap(u16 width, u16 height)
{
    return 1 + floor(log2(walrus_max(width, height)));
}

Walrus_TextureHandle walrus_rhi_create_texture(Walrus_TextureCreateInfo const* info)
{
    Walrus_TextureHandle handle = (Walrus_TextureHandle){walrus_handle_alloc(s_ctx->textures)};
    if (handle.id == WR_INVALID_HANDLE) {
        s_ctx->err = WR_RHI_ALLOC_HADNLE_ERROR;
        return handle;
    }

    Walrus_TextureCreateInfo _info = *info;
    // if no mip mode is selected, force num_mipmaps to be 1
    u32 const mip = (_info.flags & WR_RHI_SAMPLER_MIP_MASK) >> WR_RHI_SAMPLER_MIP_SHIFT;
    if (mip == 0) {
        _info.num_mipmaps = 1;
    }

    _info.num_mipmaps = _info.num_mipmaps == 0 ? calculteMipmap(_info.width, _info.height) : _info.num_mipmaps;

    s_table->texture_create_fn(handle, &_info);
    return handle;
}

Walrus_TextureHandle walrus_rhi_create_texture2d(u32 width, u32 height, Walrus_PixelFormat format, u8 mipmaps,
                                                 u64 flags, void const* data, u64 size)
{
    Walrus_TextureCreateInfo info;
    info.width       = width;
    info.height      = height;
    info.depth       = 1;
    info.ratio       = WR_RHI_RATIO_COUNT;
    info.num_mipmaps = mipmaps;
    info.num_layers  = 1;
    info.format      = format;
    info.data        = data;
    info.size        = size;
    info.flags       = flags;
    return walrus_rhi_create_texture(&info);
}

Walrus_TextureHandle walrus_rhi_create_texture2d_ratio(Walrus_BackBufferRatio ratio, Walrus_PixelFormat format,
                                                       u8 mipmaps, u64 flags, void const* data, u64 size)
{
    Walrus_TextureCreateInfo info;
    info.width       = 1;
    info.height      = 1;
    info.depth       = 1;
    info.ratio       = ratio;
    info.num_mipmaps = mipmaps;
    info.num_layers  = 1;
    info.format      = format;
    info.data        = data;
    info.size        = size;
    info.flags       = flags;

    return walrus_rhi_create_texture(&info);
}

void walrus_rhi_destroy_texture(Walrus_TextureHandle handle)
{
    if (handle.id == WR_INVALID_HANDLE) {
        s_ctx->err = WR_RHI_HANDLE_INVALID_ERROR;
        return;
    }

    s_table->texture_destroy_fn(handle);
}

void walrus_rhi_set_texture(u8 unit, Walrus_UniformHandle sampler, Walrus_TextureHandle texture)
{
    if (unit >= WR_RHI_MAX_TEXTURE_SAMPLERS) {
        s_ctx->err = WR_RHI_TEXTURE_UNIT_ERROR;
        return;
    }

    if (sampler.id != WR_INVALID_HANDLE) {
        u32 _unit = unit;
        walrus_rhi_set_uniform(sampler, 0, sizeof(_unit), &unit);
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

#pragma once

#include <rhi/rhi_defines.h>
#include <rhi/type.h>

#include <cglm/types.h>

Walrus_RhiError walrus_rhi_init(Walrus_RhiCreateInfo* info);
void            walrus_rhi_shutdown(void);

char const* walrus_rhi_error_msg(void);

void walrus_rhi_set_resolution(u32 width, u32 height);

void walrus_rhi_frame(void);

Walrus_RenderResult walrus_rhi_render_frame(i32 ms);

void walrus_rhi_touch(u16 view_id);
void walrus_rhi_submit(u16 view_id, Walrus_ProgramHandle program, u32 depth, u8 flags);

u32  walrus_rhi_compose_rgba(u8 r, u8 g, u8 b, u8 a);
void walrus_rhi_decompose_rgba(u32 rgba, u8* r, u8* g, u8* b, u8* a);

void walrus_rhi_set_state(u64 state, u32 rgba);
void walrus_rhi_set_stencil(u32 fstencil, u32 bstencil);

void walrus_rhi_set_scissor(i32 x, i32 y, u32 width, u32 height);

void walrus_rhi_set_view_rect(u16 view_id, i32 x, i32 y, u32 width, u32 height);
void walrus_rhi_set_view_clear(u16 view_id, u16 flags, u32 rgba, f32 depth, u8 stencil);
void walrus_rhi_set_view_transform(u16 view_id, mat4 view, mat4 projection);
void walrus_rhi_set_view_mode(u16 view_id, Walrus_ViewMode mode);

void walrus_rhi_screen_to_clip(u16 view_id, vec2 const screen, vec2 clip);
void walrus_rhi_screen_to_world(u16 view_id, vec2 const screen, vec3 world);
void walrus_rhi_screen_to_world_dir(u16 view_id, vec2 const screen, vec3 world_dir);

void walrus_rhi_set_transform(mat4 const transform);

Walrus_ShaderHandle walrus_rhi_create_shader(Walrus_ShaderType type, char const* source);
void                walrus_rhi_destroy_shader(Walrus_ShaderHandle handle);

Walrus_ProgramHandle walrus_rhi_create_program(Walrus_ShaderHandle* shaders, u32 num, bool destroy_shader);
void                 walrus_rhi_destroy_program(Walrus_ProgramHandle handle);

Walrus_UniformHandle walrus_rhi_create_uniform(char const* name, Walrus_UniformType type, i8 num);
void                 walrus_rhi_destroy_uniform(Walrus_UniformHandle handle);

void walrus_rhi_set_uniform(Walrus_UniformHandle handle, u32 offset, u32 size, void const* data);

Walrus_LayoutHandle walrus_rhi_create_vertex_layout(Walrus_VertexLayout const* layout);
void                walrus_rhi_destroy_vertex_layout(Walrus_LayoutHandle handle);

Walrus_BufferHandle walrus_rhi_create_buffer(void const* data, u64 size, u16 flags);
void                walrus_rhi_destroy_buffer(Walrus_BufferHandle handle);
void                walrus_rhi_update_buffer(Walrus_BufferHandle handle, u64 offset, u64 size, void const* data);

void walrus_rhi_set_vertex_count(u32 num_vertices);
void walrus_rhi_set_vertex_buffer(u8 stream_id, Walrus_BufferHandle handle, Walrus_LayoutHandle layout_handle,
                                  u32 offset, u32 num_vertices);
void walrus_rhi_set_transient_buffer(u8 stream_id, Walrus_TransientBuffer* buffer, Walrus_LayoutHandle layout_handle,
                                     u32 offset, u32 num_vertices);
void walrus_rhi_set_instance_buffer(Walrus_BufferHandle handle, Walrus_LayoutHandle layout_handle, u32 offset,
                                    u32 num_instance);
void walrus_rhi_set_transient_instance_buffer(Walrus_TransientBuffer* buffer, Walrus_LayoutHandle layout_handle,
                                              u32 offset, u32 num_instance);

void walrus_rhi_set_index_buffer(Walrus_BufferHandle handle, u32 offset, u32 num_indices);
void walrus_rhi_set_index32_buffer(Walrus_BufferHandle handle, u32 offset, u32 num_indices);
void walrus_rhi_set_transient_index_buffer(Walrus_TransientBuffer* buffer, u32 offset, u32 num_indices);

Walrus_TextureHandle walrus_rhi_create_texture(Walrus_TextureCreateInfo const* info, void const* data);
Walrus_TextureHandle walrus_rhi_create_texture2d(u32 width, u32 height, Walrus_PixelFormat format, u8 mipmaps,
                                                 u64 flags, void const* data);
Walrus_TextureHandle walrus_rhi_create_texture2d_ratio(Walrus_BackBufferRatio ratio, Walrus_PixelFormat format,
                                                       u8 mipmaps, u64 flags, void const* data);
void                 walrus_rhi_destroy_texture(Walrus_TextureHandle handle);

void walrus_rhi_set_texture(u8 unit, Walrus_TextureHandle texture);

void walrus_rhi_set_image(uint8_t unit, Walrus_TextureHandle handle, u8 mip, Walrus_DataAccess access,
                          Walrus_PixelFormat format);

u32  walrus_rhi_avail_transient_buffer(u32 num, u32 stride);
bool walrus_rhi_alloc_transient_buffer(Walrus_TransientBuffer* buffer, u32 num, u32 stride);
u32  walrus_rhi_avail_transient_index_buffer(u32 num, u32 stride);
bool walrus_rhi_alloc_transient_index_buffer(Walrus_TransientBuffer* buffer, u32 num, u32 stride);

Walrus_FramebufferHandle walrus_rhi_create_framebuffer(Walrus_Attachment *attachments, u8 num);
void                     walrus_rhi_destroy_framebuffer(Walrus_FramebufferHandle handle);

void walrus_rhi_set_framebuffer(u16 view_id, Walrus_FramebufferHandle handle);

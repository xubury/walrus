#pragma once

#include <core/type.h>
#include <rhi/rhi.h>
#include <cglm/types.h>

void walrus_batch_render_init(void);

void walrus_batch_render_shutdown(void);

void walrus_batch_render_begin(u16 view_id, u64 state);

void warlus_batch_render_quad(vec3 pos, versor rot, vec2 size, u32 color, f32 thickness, u32 boarder_color, f32 fade);
void warlus_batch_render_subtexture(Walrus_TextureHandle texture, vec2 uv0, vec2 uv1, vec3 pos, versor rot, vec2 size,
                                    u32 color, f32 thickness, u32 boarder_color, f32 fade);
void warlus_batch_render_texture(Walrus_TextureHandle texture, vec3 pos, versor rot, vec2 size, u32 color,
                                 f32 thickness, u32 boarder_color, f32 fade);

void warlus_batch_render_circle(vec3 pos, versor rot, f32 radius, u32 color, f32 thickness, u32 boarder_color,
                                f32 fade);

void walrus_batch_render_end(void);

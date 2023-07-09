#pragma once

#include <core/type.h>
#include <core/transform.h>
#include <engine/camera.h>
#include <engine/model.h>
#include <engine/renderer_mesh.h>

typedef struct {
    bool record;
    u32  draw_calls;
    u64  vertices;
    u64  indices;
} Walrus_RendererSubmitStats;

typedef struct {
    Walrus_FramebufferHandle framebuffer;

    Walrus_Camera *camera;

    bool active;
    i16  x;
    i16  y;
    u16  width;
    u16  height;

    Walrus_RendererSubmitStats stats;
} Walrus_DeferredRenderer;

void walrus_deferred_renderer_init(u8 msaa);

void walrus_deferreed_renderer_init(Walrus_DeferredRenderer *renderer);

void walrus_deferred_renderer_set_camera(Walrus_DeferredRenderer *renderer, Walrus_Camera *camera);

void walrus_deferred_renderer_submit_mesh(Walrus_DeferredRenderer *renderer, mat4 const world, Walrus_Mesh *mesh,
                                          Walrus_TransientBuffer weights);

void walrus_forward_renderer_submit_mesh(Walrus_DeferredRenderer *renderer, mat4 const world, Walrus_Mesh *mesh,
                                         Walrus_TransientBuffer weights);

void walrus_forward_renderer_submit_skinned_mesh(Walrus_DeferredRenderer *renderer, mat4 const world, Walrus_Mesh *mesh,
                                                 Walrus_TransientBuffer joints, Walrus_TransientBuffer weights);

void walrus_deferred_renderer_submit_skinned_mesh(Walrus_DeferredRenderer *renderer, mat4 const world,
                                                  Walrus_Mesh *mesh, Walrus_TransientBuffer joints,
                                                  Walrus_TransientBuffer weights);

void walrus_deferred_renderer_lighting(void);

void walrus_renderer_submit_backbuffer(void);

void walrus_deferred_renderer_start_record(Walrus_DeferredRenderer *renderer);
void walrus_deferred_renderer_end_record(Walrus_DeferredRenderer *renderer);

void walrus_deferred_renderer_log_stats(Walrus_DeferredRenderer *renderer, char *buffer, u32 size);

#pragma once

#include <core/type.h>
#include <core/transform.h>
#include <engine/camera.h>
#include <engine/model.h>
#include <engine/renderer_mesh.h>

typedef struct {
    Walrus_FramebufferHandle framebuffer;

    bool active;
    i16  x;
    i16  y;
    u16  width;
    u16  height;

} Walrus_DeferredRenderer;

void walrus_deferred_renderer_init(u8 msaa);

void walrus_deferred_renderer_set_camera(Walrus_DeferredRenderer const *renderer, Walrus_Camera const *camera);

void walrus_deferred_renderer_submit_mesh(mat4 const world, Walrus_Mesh *mesh, Walrus_TransientBuffer const *weights);

void walrus_deferred_renderer_submit_skinned_mesh(mat4 const world, Walrus_Mesh *mesh,
                                                  Walrus_TransientBuffer const *joints,
                                                  Walrus_TransientBuffer const *weights);

void walrus_deferred_renderer_submit_mesh_ablend(mat4 const world, Walrus_Mesh *mesh,
                                                 Walrus_TransientBuffer const *weights);

void walrus_deferred_renderer_submit_skinned_mesh_ablend(mat4 const world, Walrus_Mesh *mesh,
                                                         Walrus_TransientBuffer const *joints,
                                                         Walrus_TransientBuffer const *weights);

void walrus_deferred_renderer_lighting(void);

void walrus_renderer_submit_backbuffer(void);

void walrus_renderer_submit_hdr(void);

void walrus_renderer_submit_quad(u16 view_id, Walrus_ProgramHandle shader);

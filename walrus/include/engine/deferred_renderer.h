#pragma once

#include <core/type.h>
#include <core/transform.h>
#include <engine/camera.h>
#include <engine/model.h>
#include <engine/renderer_mesh.h>

typedef struct {
    Walrus_FramebufferHandle framebuffer;
    i16                      x;
    i16                      y;
    u16                      width;
    u16                      height;
} Walrus_DeferredRenderer;

void walrus_deferred_renderer_init_uniforms(void);

void walrus_deferred_renderer_set_camera(Walrus_DeferredRenderer *renderer, Walrus_Camera *camera);

void walrus_deferred_renderer_submit_mesh(Walrus_Transform const *transform, Walrus_StaticMesh *mesh);

void walrus_deferred_renderer_submit_skinned_mesh(Walrus_Transform const *transform, Walrus_SkinnedMesh *mesh);

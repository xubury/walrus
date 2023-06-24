#pragma once

#include <core/type.h>
#include <core/transform.h>
#include <engine/camera.h>
#include <engine/model.h>
#include <engine/animator.h>

typedef struct {
    Walrus_FramebufferHandle framebuffer;
    i16                      x;
    i16                      y;
    u16                      width;
    u16                      height;
} Walrus_DeferredRenderer;

void walrus_deferred_renderer_init_uniforms(void);

void walrus_deferred_renderer_set_camera(Walrus_DeferredRenderer *renderer, Walrus_Camera *camera);

void walrus_deferred_renderer_submit(Walrus_Transform *transform, Walrus_Model *model, Walrus_Animator const *animator);

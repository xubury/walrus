#pragma once

#include <core/type.h>
#include <core/array.h>
#include <engine/model.h>

typedef struct {
    u32  prev;  // prev frame
    vec4 data;
} Walrus_AnimatorKey;

typedef struct {
    Walrus_Model const     *model;
    Walrus_Animation const *animation;
    Walrus_Array           *keys;
    Walrus_Array           *worlds;
    Walrus_Array           *locals;
    f32                     timestamp;
    bool                    repeat;
    bool                    playing;
} Walrus_Animator;

void walrus_animator_init(Walrus_Animator *animator);

void walrus_animator_shutdown(Walrus_Animator *animator);

void walrus_animator_bind(Walrus_Animator *animator, Walrus_Model const *model);

void walrus_animator_play(Walrus_Animator *animator, u32 index);

void walrus_animator_tick(Walrus_Animator *animator, f32 dt);

void walrus_animator_transform(Walrus_Animator *animator, Walrus_ModelNode *node, mat4 transform);
#pragma once

#include <engine/model.h>

typedef struct {
    Walrus_Mesh *mesh;

    bool culled;
} Walrus_RenderMesh;

typedef struct {
    Walrus_ModelNode      *node;
    Walrus_TransientBuffer weight_buffer;
} Walrus_WeightResource;

typedef struct {
    vec3 min;
    vec3 max;

    Walrus_ModelSkin      *skin;
    Walrus_TransientBuffer joint_buffer;
} Walrus_SkinResource;

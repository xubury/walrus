#pragma once

#include <engine/model.h>

typedef struct {
    Walrus_Mesh      *mesh;
    Walrus_ModelNode *node;

    bool culled;
} Walrus_StaticMesh;

typedef struct {
    Walrus_Mesh      *mesh;
    Walrus_ModelNode *node;

    bool culled;
} Walrus_SkinnedMesh;

typedef struct {
    Walrus_Mesh           *mesh;
    Walrus_TransientBuffer weight_buffer;
} Walrus_MeshResource;

typedef struct {
    vec3 min;
    vec3 max;

    Walrus_ModelSkin      *skin;
    Walrus_TransientBuffer joint_buffer;
} Walrus_SkinResource;

#pragma once

#include <engine/model.h>

typedef struct {
    Walrus_Mesh           *mesh;
    Walrus_ModelNode      *node;
    Walrus_TransientBuffer weight_buffer;
} Walrus_StaticMesh;

typedef struct {
    Walrus_Mesh           *mesh;
    Walrus_ModelSkin      *skin;
    Walrus_ModelNode      *node;
    Walrus_TransientBuffer joint_buffer;
    Walrus_TransientBuffer weight_buffer;
} Walrus_SkinnedMesh;

#pragma once

#include <rhi/type.h>

typedef struct {
    Walrus_BufferHandle buffer;
    Walrus_LayoutHandle layout_handle;
    u32                 offset;
    u32                 num_vertices;
} Walrus_PrimitiveStream;

typedef struct {
    Walrus_BufferHandle buffer;
    u32                 offset;
    u32                 num_indices;
    bool                index32;
} Walrus_MeshIndices;

typedef struct {
    Walrus_PrimitiveStream streams[WR_RHI_MAX_VERTEX_STREAM];
    u32                    num_streams;

    Walrus_MeshIndices indices;
} Walrus_MeshPrimitive;

typedef struct {
    Walrus_MeshPrimitive *primitives;
    u32                   num_primitives;
} Walrus_Mesh;

typedef struct {
    Walrus_BufferHandle *buffers;
    u32                  num_buffers;

    Walrus_TextureHandle *textures;
    u32                   num_textures;

    Walrus_Mesh *meshes;
    u32          num_meshes;
} Walrus_Model;

typedef enum {
    WR_MODEL_SUCCESS = 0,
    WR_MODEL_GLTF_ERROR,
    WR_MODEL_BUFFER_ERROR,
    WR_MODEL_IMAGE_ERROR,

    WR_MODEL_UNKNOWN_ERROR = -1
} Walrus_ModelResult;

Walrus_ModelResult walrus_model_load_from_file(Walrus_Model *model, char const *filename);

void walrus_model_shutdown(Walrus_Model *model);

#pragma once

#include <rhi/type.h>
#include <core/transform.h>

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
    Walrus_TextureHandle handle;
    bool                 srgb;
} Walrus_Texture;

typedef enum {
    WR_ALPHA_MODE_OPAQUE,
    WR_ALPHA_MODE_MASK,
    WR_ALPHA_MODE_BLEND,
} Walrus_AlphaMode;

typedef struct {
    Walrus_Texture  *albedo;
    vec4             albedo_factor;
    Walrus_Texture  *normal;
    f32              normal_scale;
    Walrus_Texture  *metallic_roughness;
    Walrus_Texture  *emissive;
    vec3             emissive_factor;
    Walrus_AlphaMode alpha_mode;
    bool             double_sided;
} Walrus_MeshMaterial;

typedef struct {
    Walrus_PrimitiveStream streams[16];
    u32                    num_streams;

    Walrus_MeshIndices indices;

    Walrus_MeshMaterial *material;
} Walrus_MeshPrimitive;

typedef struct {
    Walrus_MeshPrimitive *primitives;
    u32                   num_primitives;
} Walrus_Mesh;

typedef struct Walrus_ModelNode Walrus_ModelNode;

struct Walrus_ModelNode {
    Walrus_Mesh       *mesh;
    Walrus_ModelNode  *parent;
    Walrus_ModelNode **children;
    u32                num_children;
    Walrus_Transform   world_transform;
    Walrus_Transform   local_transform;
};

typedef struct {
    Walrus_BufferHandle *buffers;
    u32                  num_buffers;

    Walrus_Texture *textures;
    u32             num_textures;

    Walrus_Mesh *meshes;
    u32          num_meshes;

    Walrus_MeshMaterial *materials;
    u32                  num_materials;

    Walrus_ModelNode *nodes;
    u32               num_nodes;

    Walrus_ModelNode **roots;
    u32                num_roots;
} Walrus_Model;

typedef enum {
    WR_MODEL_SUCCESS = 0,
    WR_MODEL_GLTF_ERROR,
    WR_MODEL_BUFFER_ERROR,
    WR_MODEL_IMAGE_ERROR,

    WR_MODEL_UNKNOWN_ERROR = -1
} Walrus_ModelResult;

typedef void (*ModelSubmitCallback)(Walrus_ModelNode *node, Walrus_MeshPrimitive *primitive, void *userdata);

Walrus_ModelResult walrus_model_load_from_file(Walrus_Model *model, char const *filename);

void walrus_model_shutdown(Walrus_Model *model);

void walrus_model_submit(u16 view_id, Walrus_Model *model, Walrus_ProgramHandle shader, u32 depth,
                         ModelSubmitCallback cb, void *userdata);

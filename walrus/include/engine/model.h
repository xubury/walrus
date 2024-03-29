#pragma once

#include <rhi/type.h>
#include <core/transform.h>
#include <engine/material.h>

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

typedef enum {
    WR_MESH_ALBEDO,
    WR_MESH_ALBEDO_FACTOR,
    WR_MESH_NORMAL,
    WR_MESH_NORMAL_SCALE,
    WR_MESH_METALLIC_ROUGHNESS,
    WR_MESH_METALLIC_FACTOR,
    WR_MESH_ROUGHNESS_FACTOR,
    WR_MESH_SPECULAR_GLOSSINESS,
    WR_MESH_SPECULAR_FACTOR,
    WR_MESH_GLOSSINESS_FACTOR,
    WR_MESH_EMISSIVE,
    WR_MESH_EMISSIVE_FACTOR,
    WR_MESH_OCCLUSION,

    WR_MESH_ALPHA_CUTOFF,

    WR_MESH_PROPERTY_COUNT
} Walrus_MeshMaterialProperty;

typedef struct {
    Walrus_PrimitiveStream streams[WR_RHI_MAX_VERTEX_STREAM];
    u32                    num_streams;

    Walrus_MeshIndices indices;

    Walrus_Material     *material;
    Walrus_TextureHandle morph_target;

    vec3 min;
    vec3 max;
} Walrus_MeshPrimitive;

typedef struct {
    char                  name[256];
    Walrus_MeshPrimitive *primitives;
    u32                   num_primitives;

    f32 *weights;
    u32  num_weights;
} Walrus_Mesh;

typedef struct Walrus_ModelNode Walrus_ModelNode;

typedef struct {
    Walrus_ModelNode *node;
    mat4              inverse_bind_matrix;

    vec3 min;
    vec3 max;
} Walrus_SkinJoint;

typedef struct {
    Walrus_SkinJoint *joints;
    Walrus_ModelNode *skeleton;
    u32               num_joints;
} Walrus_ModelSkin;

struct Walrus_ModelNode {
    Walrus_Mesh       *mesh;
    Walrus_ModelNode  *parent;
    Walrus_ModelNode **children;
    Walrus_ModelSkin  *skin;
    u32                num_children;
    Walrus_Transform   world_transform;
    Walrus_Transform   local_transform;
};

typedef enum {
    WR_ANIMATION_PATH_TRANSLATION,
    WR_ANIMATION_PATH_ROTATION,
    WR_ANIMATION_PATH_SCALE,
    WR_ANIMATION_PATH_WEIGHTS,

    WR_ANIMATION_PATH_COUNT
} Walrus_AnimationPath;

typedef enum {
    WR_ANIMATION_INTERPOLATION_LINEAR,
    WR_ANIMATION_INTERPOLATION_STEP,
    WR_ANIMATION_INTERPOLATION_CUBIC_SPLINE,
} Walrus_AnimationInterpolation;

typedef struct {
    Walrus_AnimationInterpolation interpolation;

    f32 *timestamps;
    f32 *data;
    u32  num_components;
    u32  num_frames;
} Walrus_AnimationSampler;

typedef struct {
    Walrus_AnimationSampler *sampler;
    Walrus_ModelNode        *node;
    Walrus_AnimationPath     path;
} Walrus_AnimationChannel;

typedef struct {
    Walrus_AnimationChannel *channels;
    u32                      num_channels;

    Walrus_AnimationSampler *samplers;
    u32                      num_samplers;

    f32 duration;
} Walrus_Animation;

typedef struct {
    Walrus_BufferHandle *buffers;
    u32                  num_buffers;

    Walrus_BufferHandle tangent_buffer;

    Walrus_Mesh *meshes;
    u32          num_meshes;

    Walrus_Material *materials;
    u32              num_materials;

    Walrus_TextureHandle *textures;
    u32                   num_textures;

    Walrus_ModelNode *nodes;
    u32               num_nodes;

    Walrus_ModelNode **roots;
    u32                num_roots;

    Walrus_Animation *animations;
    u32               num_animations;

    Walrus_ModelSkin *skins;
    u32               num_skins;
} Walrus_Model;

typedef enum {
    WR_MODEL_SUCCESS = 0,
    WR_MODEL_GLTF_ERROR,
    WR_MODEL_BUFFER_ERROR,
    WR_MODEL_IMAGE_ERROR,

    WR_MODEL_UNKNOWN_ERROR = -1
} Walrus_ModelResult;

typedef void (*PrimitiveSubmitCallback)(Walrus_MeshPrimitive const *primitive, void *userdata);
typedef void (*NodeSubmitCallback)(Walrus_Model const *model, Walrus_ModelNode const *node, void *userdata);

Walrus_ModelResult walrus_model_load_from_file(Walrus_Model *model, char const *filename);

void walrus_model_shutdown(Walrus_Model *model);

void walrus_model_material_init_default(Walrus_Material *material);

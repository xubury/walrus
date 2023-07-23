#pragma once

#include <core/hash.h>
#include <cglm/cglm.h>
#include <rhi/type.h>

typedef enum {
    WR_MATERIAL_PROPERTY_FLOAT,
    WR_MATERIAL_PROPERTY_VEC4,
    WR_MATERIAL_PROPERTY_TEXTURE2D,
} Walrus_MaterialPropertyType;

typedef struct {
    Walrus_TextureHandle handle;
    bool                 srgb;
} Walrus_Texture;

typedef struct {
    char                       *name;
    Walrus_MaterialPropertyType type;

    union {
        vec4           vector;
        Walrus_Texture texture;
    };
} Walrus_MaterialProperty;

typedef struct {
    bool double_sided;

    Walrus_HashTable *properties;
} Walrus_Material;

void walrus_material_init(Walrus_Material *material);
void walrus_material_shutdown(Walrus_Material *material);

void walrus_material_set_texture(Walrus_Material *material, char const *name, Walrus_TextureHandle texture, bool srgb);
void walrus_material_set_float(Walrus_Material *material, char const *name, f32 value);
void walrus_material_set_vec4(Walrus_Material *material, char const *name, vec4 value);

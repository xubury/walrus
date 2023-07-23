#include <engine/material.h>
#include <core/macro.h>
#include <core/assert.h>
#include <core/log.h>
#include <core/string.h>
#include <core/memory.h>
#include <rhi/rhi.h>

#include <string.h>

Walrus_MaterialProperty *property_create(char const *name, Walrus_MaterialPropertyType type)
{
    Walrus_MaterialProperty *p = walrus_new(Walrus_MaterialProperty, 1);
    p->name                    = walrus_str_dup(name);
    p->type                    = type;

    Walrus_UniformType utype;
    switch (p->type) {
        case WR_MATERIAL_PROPERTY_BOOL:
            utype = WR_RHI_UNIFORM_BOOL;
            break;
        case WR_MATERIAL_PROPERTY_FLOAT:
            utype = WR_RHI_UNIFORM_FLOAT;
            break;
        case WR_MATERIAL_PROPERTY_VEC4:
            utype = WR_RHI_UNIFORM_VEC4;
            break;
        case WR_MATERIAL_PROPERTY_TEXTURE2D:
            utype = WR_RHI_UNIFORM_SAMPLER;
            break;
    }
    p->uni = walrus_rhi_create_uniform(name, utype, 1);

    return p;
}

void property_destroy(void *data)
{
    Walrus_MaterialProperty *p = data;
    walrus_str_free(p->name);
    if (p->type == WR_MATERIAL_PROPERTY_TEXTURE2D) {
        walrus_rhi_destroy_texture(p->texture.handle);
    }
    walrus_free(p);
}

void walrus_material_init(Walrus_Material *material)
{
    material->properties = walrus_hash_table_create_full(walrus_str_hash, walrus_str_equal, NULL, property_destroy);
}

void walrus_material_shutdown(Walrus_Material *material)
{
    walrus_hash_table_destroy(material->properties);
}

void walrus_material_set_texture(Walrus_Material *material, char const *name, Walrus_TextureHandle texture, bool srgb)
{
    if (texture.id == WR_INVALID_HANDLE) {
        walrus_error("Invalid texture for material is set!");
        return;
    }
    Walrus_MaterialProperty *p = NULL;
    if (walrus_hash_table_contains(material->properties, name)) {
        p = walrus_hash_table_lookup(material->properties, name);
    }
    else {
        p = property_create(name, WR_MATERIAL_PROPERTY_TEXTURE2D);
        walrus_hash_table_insert(material->properties, p->name, p);
    }
    p->texture = (Walrus_Texture){.handle = texture, .srgb = srgb};
}

static Walrus_TextureHandle black = {WR_INVALID_HANDLE};
static Walrus_TextureHandle white = {WR_INVALID_HANDLE};

// generate default texture if needed
static void gen_default_textures(void)
{
    u32 value = 0;
    if (black.id == WR_INVALID_HANDLE) {
        black = walrus_rhi_create_texture2d(1, 1, WR_RHI_FORMAT_RGB8, 0, 0, &value);
    }
    value = 0xffffffff;
    if (white.id == WR_INVALID_HANDLE) {
        white = walrus_rhi_create_texture2d(1, 1, WR_RHI_FORMAT_RGB8, 0, 0, &value);
    }
}

void walrus_material_set_texture_color(Walrus_Material *material, char const *name, Walrus_ColorTextureType type)
{
    gen_default_textures();

    switch (type) {
        case WR_COLOR_TEXTURE_BLACK:
            walrus_material_set_texture(material, name, black, false);
            break;
        case WR_COLOR_TEXTURE_WHITE:
            walrus_material_set_texture(material, name, white, false);
            break;
    }
}

void walrus_material_set_bool(Walrus_Material *material, char const *name, bool value)
{
    Walrus_MaterialProperty *p = NULL;
    if (walrus_hash_table_contains(material->properties, name)) {
        p = walrus_hash_table_lookup(material->properties, name);
    }
    else {
        p = property_create(name, WR_MATERIAL_PROPERTY_BOOL);
        walrus_hash_table_insert(material->properties, p->name, p);
    }
    p->boolean = value;
}

void walrus_material_set_float(Walrus_Material *material, char const *name, f32 value)
{
    Walrus_MaterialProperty *p = NULL;
    if (walrus_hash_table_contains(material->properties, name)) {
        p = walrus_hash_table_lookup(material->properties, name);
    }
    else {
        p = property_create(name, WR_MATERIAL_PROPERTY_FLOAT);
        walrus_hash_table_insert(material->properties, p->name, p);
    }
    p->vector[0] = value;
}

void walrus_material_set_vec3(Walrus_Material *material, char const *name, vec3 value)
{
    Walrus_MaterialProperty *p = NULL;
    if (walrus_hash_table_contains(material->properties, name)) {
        p = walrus_hash_table_lookup(material->properties, name);
    }
    else {
        p = property_create(name, WR_MATERIAL_PROPERTY_VEC4);
        walrus_hash_table_insert(material->properties, p->name, p);
    }
    glm_vec3_copy(value, p->vector);
}

void walrus_material_set_vec4(Walrus_Material *material, char const *name, vec4 value)
{
    Walrus_MaterialProperty *p = NULL;
    if (walrus_hash_table_contains(material->properties, name)) {
        p = walrus_hash_table_lookup(material->properties, name);
    }
    else {
        p = property_create(name, WR_MATERIAL_PROPERTY_VEC4);
        walrus_hash_table_insert(material->properties, p->name, p);
    }
    glm_vec4_copy(value, p->vector);
}

static void submit_uniform(void const *key, void *value, void *userdata)
{
    walrus_unused(key);
    u32                     *unit = userdata;
    Walrus_MaterialProperty *m    = value;
    switch (m->type) {
        case WR_MATERIAL_PROPERTY_BOOL:
            walrus_rhi_set_uniform(m->uni, 0, sizeof(bool), &m->boolean);
            break;
        case WR_MATERIAL_PROPERTY_FLOAT:
            walrus_rhi_set_uniform(m->uni, 0, sizeof(f32), m->vector);
            break;
        case WR_MATERIAL_PROPERTY_VEC4:
            walrus_rhi_set_uniform(m->uni, 0, sizeof(vec4), m->vector);
            break;
        case WR_MATERIAL_PROPERTY_TEXTURE2D:
            walrus_rhi_set_uniform(m->uni, 0, sizeof(u32), unit);
            walrus_rhi_set_texture(*unit, m->texture.handle);
            ++(*unit);
            break;
    }
}

void walrus_material_submit(Walrus_Material *material)
{
    u32 unit = 0;

    u64 flags = WR_RHI_STATE_DEFAULT;

    if (!material->double_sided) {
        flags |= WR_RHI_STATE_CULL_CW;
    }
    if (material->alpha_mode == WR_ALPHA_MODE_BLEND) {
        flags |= WR_RHI_STATE_BLEND_ALPHA;
    }
    walrus_rhi_set_state(flags, 0);
    walrus_hash_table_foreach(material->properties, submit_uniform, &unit);
}

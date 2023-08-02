#include <engine/material.h>
#include <core/macro.h>
#include <core/assert.h>
#include <core/log.h>
#include <core/string.h>
#include <core/memory.h>
#include <rhi/rhi.h>

#include <string.h>

static u32 find_property_id(Walrus_Material *material)
{
    for (u32 i = 0; i < walrus_count_of(material->properties); ++i) {
        if (!material->properties[i].valid) {
            return i;
        }
    }
    return -1;
}

static Walrus_MaterialProperty *property_get_or_create(Walrus_Material *material, char const *name,
                                                       Walrus_MaterialPropertyType type)
{
    Walrus_MaterialProperty *p = NULL;
    if (walrus_hash_table_contains(material->table, name)) {
        u32 id = walrus_ptr_to_val(walrus_hash_table_lookup(material->table, name));
        p      = &material->properties[id];
        walrus_assert(p->valid);
    }
    else {
        u32 id   = find_property_id(material);
        p        = &material->properties[id];
        p->valid = true;
        p->name  = walrus_str_dup(name);
        p->type  = type;

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

        walrus_hash_table_insert(material->table, p->name, walrus_val_to_ptr(id));
    }

    return p;
}

static void property_destroy(Walrus_MaterialProperty *p)
{
    if (p->valid) {
        p->valid = false;
        walrus_str_free(p->name);
        walrus_rhi_destroy_uniform(p->uni);
    }
}

void walrus_material_init(Walrus_Material *material)
{
    material->table = walrus_hash_table_create(walrus_str_hash, walrus_str_equal);
    for (u32 i = 0; i < walrus_count_of(material->properties); ++i) {
        material->properties[i].valid = false;
    }
}

void walrus_material_shutdown(Walrus_Material *material)
{
    for (u32 i = 0; i < walrus_count_of(material->properties); ++i) {
        property_destroy(&material->properties[i]);
    }
    walrus_hash_table_destroy(material->table);
}

void walrus_material_set_texture(Walrus_Material *material, char const *name, Walrus_TextureHandle texture, bool srgb)
{
    if (texture.id == WR_INVALID_HANDLE) {
        walrus_error("Invalid texture for material is set!");
        return;
    }
    Walrus_MaterialProperty *p = property_get_or_create(material, name, WR_MATERIAL_PROPERTY_TEXTURE2D);
    p->texture                 = (Walrus_Texture){.handle = texture, .srgb = srgb};
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
    Walrus_MaterialProperty *p = property_get_or_create(material, name, WR_MATERIAL_PROPERTY_BOOL);
    p->boolean                 = value;
}

void walrus_material_set_float(Walrus_Material *material, char const *name, f32 value)
{
    Walrus_MaterialProperty *p = property_get_or_create(material, name, WR_MATERIAL_PROPERTY_FLOAT);
    p->vector[0]               = value;
}

void walrus_material_set_vec3(Walrus_Material *material, char const *name, vec3 value)
{
    Walrus_MaterialProperty *p = property_get_or_create(material, name, WR_MATERIAL_PROPERTY_VEC4);
    glm_vec3_copy(value, p->vector);
}

void walrus_material_set_vec4(Walrus_Material *material, char const *name, vec4 value)
{
    Walrus_MaterialProperty *p = property_get_or_create(material, name, WR_MATERIAL_PROPERTY_VEC4);
    glm_vec4_copy(value, p->vector);
}

typedef struct {
    u32              unit;
    Walrus_Material *material;
} SubmitData;

static void submit_uniform(void const *key, void *value, void *userdata)
{
    walrus_unused(key);
    SubmitData              *data = userdata;
    u32                      id   = walrus_ptr_to_val(value);
    Walrus_MaterialProperty *p    = &data->material->properties[id];
    switch (p->type) {
        case WR_MATERIAL_PROPERTY_BOOL:
            walrus_rhi_set_uniform(p->uni, 0, sizeof(bool), &p->boolean);
            break;
        case WR_MATERIAL_PROPERTY_FLOAT:
            walrus_rhi_set_uniform(p->uni, 0, sizeof(f32), p->vector);
            break;
        case WR_MATERIAL_PROPERTY_VEC4:
            walrus_rhi_set_uniform(p->uni, 0, sizeof(vec4), p->vector);
            break;
        case WR_MATERIAL_PROPERTY_TEXTURE2D:
            walrus_rhi_set_uniform(p->uni, 0, sizeof(u32), &data->unit);
            walrus_rhi_set_texture(data->unit, p->texture.handle);
            ++data->unit;
            break;
    }
}

void walrus_material_submit(Walrus_Material *material)
{
    u64 flags = WR_RHI_STATE_DEFAULT;

    if (!material->double_sided) {
        flags |= WR_RHI_STATE_CULL_CW;
    }
    if (material->alpha_mode == WR_ALPHA_MODE_BLEND) {
        flags |= WR_RHI_STATE_BLEND_ALPHA;
    }
    walrus_rhi_set_state(flags, 0);
    SubmitData data = {.unit = 0, .material = material};
    walrus_hash_table_foreach(material->table, submit_uniform, &data);
}

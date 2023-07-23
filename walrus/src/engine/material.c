#include <engine/material.h>
#include <core/string.h>
#include <core/memory.h>
#include <rhi/rhi.h>

Walrus_MaterialProperty *property_create(char const *name, Walrus_MaterialPropertyType type)
{
    Walrus_MaterialProperty *p = walrus_new(Walrus_MaterialProperty, 1);
    p->name                    = walrus_str_dup(name);
    p->type                    = type;
    return p;
}

void property_destroy(void *data)
{
    Walrus_MaterialProperty *p = data;
    walrus_str_free(p->name);
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
    Walrus_MaterialProperty *p = property_create(name, WR_MATERIAL_PROPERTY_TEXTURE2D);
    p->texture                 = (Walrus_Texture){.handle = texture, .srgb = srgb};
    walrus_hash_table_insert(material->properties, p->name, p);
}

void walrus_material_set_float(Walrus_Material *material, char const *name, f32 value)
{
    Walrus_MaterialProperty *p = property_create(name, WR_MATERIAL_PROPERTY_FLOAT);
    p->vector[0]               = value;
    walrus_hash_table_insert(material->properties, p->name, p);
}

void walrus_material_set_vec4(Walrus_Material *material, char const *name, vec4 value)
{
    Walrus_MaterialProperty *p = property_create(name, WR_MATERIAL_PROPERTY_VEC4);
    glm_vec4_copy(value, p->vector);
    walrus_hash_table_insert(material->properties, p->name, p);
}

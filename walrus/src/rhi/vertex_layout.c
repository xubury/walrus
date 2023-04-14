#include <rhi/vertex_layout.h>
#include <core/math.h>
#include <core/macro.h>
#include <core/hash.h>

#include <string.h>

static u8 s_attr_stride[WR_RHI_ATTR_COUNT][4] = {
    {1, 2, 4, 4},    // UInt8
    {1, 2, 4, 4},    // Int8
    {2, 4, 6, 8},    // Int16
    {2, 4, 6, 8},    // UInt16
    {4, 8, 12, 16},  // Int32
    {4, 8, 12, 16},  // UInt32
    {4, 8, 12, 16}   // Float
};

struct _Walrus_VertexLayout {
    u8  instance_strde;
    u16 stride;
    u16 attributes[16];
    u16 offsets[16];
    u8  num_attributes;
    u32 hash;
};

void walrus_vertex_layout_begin(Walrus_VertexLayout* layout)
{
    layout->instance_strde = 0;
    layout->hash           = 0;
    layout->stride         = 0;
    layout->num_attributes = 0;
    memset(layout->attributes, 0, sizeof(layout->attributes));
    memset(layout->offsets, 0, sizeof(layout->offsets));
}

void walrus_vertex_layout_begin_instance(Walrus_VertexLayout* layout, u8 instance)
{
    layout->instance_strde = instance;
    layout->hash           = 0;
    layout->stride         = 0;
    layout->num_attributes = 0;
    memset(layout->attributes, 0, sizeof(layout->attributes));
    memset(layout->offsets, 0, sizeof(layout->offsets));
}

static u16 encode(u8 attr_id, u8 num, Walrus_Attribute type, bool normalized, bool as_int)
{
    walrus_assert_msg(num <= 4 && num > 0, "Invalid number of components!");
    u16 const encoded_attr  = ((u16)attr_id & 7);
    u16 const encoded_num   = ((num - 1) & 3) << 7;
    u16 const encoded_type  = ((u16)type & 7) << 10;
    u16 const encoded_norm  = (normalized & 1) << 13;
    u16 const encoded_asint = (as_int & 1) << 14;
    return encoded_attr | encoded_norm | encoded_type | encoded_num | encoded_asint;
}

void walrus_vertex_layout_add(Walrus_VertexLayout* layout, u8 attr_id, u8 num, Walrus_Attribute type, bool normalized)
{
    u16 const en                               = encode(attr_id, num, type, normalized, false);
    layout->attributes[layout->num_attributes] = en;
    layout->offsets[layout->num_attributes]    = layout->stride;
    layout->stride += s_attr_stride[type][num - 1];
    ++layout->num_attributes;
    walrus_assert_msg(layout->num_attributes <= walrus_array_len(layout->attributes),
                      "Number of attributes exceeds limit!");
}

void walrus_vertex_layout_add_int(Walrus_VertexLayout* layout, u8 attr_id, u8 num, Walrus_Attribute type,
                                  bool normalized)
{
    layout->attributes[layout->num_attributes] = encode(attr_id, num, type, normalized, true);
    layout->offsets[layout->num_attributes]    = layout->stride;
    layout->stride += s_attr_stride[type][num - 1];
    ++layout->num_attributes;
    walrus_assert_msg(layout->num_attributes <= walrus_array_len(layout->attributes),
                      "Number of attributes exceeds limit!");
}

void walrus_vertex_layout_end(Walrus_VertexLayout* layout)
{
    u32 hash     = walrus_array_hash(layout, offsetof(Walrus_VertexLayout, hash));
    layout->hash = hash;
}

void walrus_vertex_layout_decode(Walrus_VertexLayout const* layout, u32 index, u8* attr_id, u8* num,
                                 Walrus_Attribute* type, bool* normalized, bool* as_int)
{
    uint16_t val = layout->attributes[index];
    *attr_id     = (val & 7);
    *num         = ((val >> 7) & 3) + 1;
    *type        = (Walrus_Attribute)((val >> 10) & 7);
    *normalized  = !!(val & (1 << 13));
    *as_int      = !!(val & (1 << 14));
}

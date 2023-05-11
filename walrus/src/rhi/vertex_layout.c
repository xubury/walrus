#include <rhi/vertex_layout.h>
#include <core/math.h>
#include <core/assert.h>
#include <core/hash.h>

#include <string.h>

static u8 s_attr_stride[WR_RHI_ATTR_COUNT][4] = {
    {1, 2, 4, 4},      // UInt8
    {1, 2, 4, 4},      // Int8
    {2, 4, 6, 8},      // Int16
    {2, 4, 6, 8},      // UInt16
    {4, 8, 12, 16},    // Int32
    {4, 8, 12, 16},    // UInt32
    {4, 8, 12, 16},    // Float
    {16, 16, 16, 16},  // Vec4
    {16, 16, 16, 16},  // Mat4
};

static u8 s_attr_align[WR_RHI_ATTR_COUNT] = {
    1,   // UInt8
    1,   // Int8
    2,   // Int16
    2,   // UInt16
    4,   // Int32
    4,   // UInt32
    4,   // Float
    16,  // Vec4
    16,  // Mat4
};

struct _Walrus_VertexLayout {
    u8  instance_strde;
    u16 stride;
    u16 attributes[16];
    u16 offsets[16];
    u8  num_attributes;
    u32 hash;
};

void walrus_vertex_layout_begin(Walrus_VertexLayout* layout, u8 start_attr)
{
    walrus_vertex_layout_begin_instance(layout, 0, start_attr);
}

void walrus_vertex_layout_begin_instance(Walrus_VertexLayout* layout, u8 instance, u8 start_attr)
{
    layout->start_attr     = start_attr;
    layout->instance_strde = instance;
    layout->hash           = 0;
    layout->stride         = 0;
    layout->num_attributes = 0;
    layout->align          = 4;
    memset(layout->attributes, 0, sizeof(layout->attributes));
    memset(layout->offsets, 0, sizeof(layout->offsets));
}

static u16 encode(u8 attr_id, u8 num, Walrus_Attribute type, bool normalized, bool as_int)
{
    walrus_assert_msg(num <= 4 && num > 0, "Invalid number of components!");
    u16 const encoded_attr  = ((u16)attr_id & 15);
    u16 const encoded_num   = ((num - 1) & 3) << 4;
    u16 const encoded_type  = ((u16)type & 15) << 6;
    u16 const encoded_norm  = (normalized & 1) << 10;
    u16 const encoded_asint = (as_int & 1) << 11;
    return encoded_attr | encoded_norm | encoded_type | encoded_num | encoded_asint;
}

static void vertex_layout_add(Walrus_VertexLayout* layout, u8 num, Walrus_Attribute type, bool normalized, bool as_int)
{
    layout->attributes[layout->num_attributes] =
        encode(layout->start_attr + layout->num_attributes, num, type, normalized, as_int);
    layout->align                           = walrus_max(layout->align, s_attr_align[type]);
    layout->offsets[layout->num_attributes] = walrus_align_up(layout->stride, s_attr_align[type]);
    layout->stride                          = layout->stride + s_attr_stride[type][num - 1];
    ++layout->num_attributes;
    walrus_assert_msg(layout->num_attributes <= walrus_array_len(layout->attributes),
                      "Number of attributes exceeds limit!");
}

void walrus_vertex_layout_add(Walrus_VertexLayout* layout, u8 num, Walrus_Attribute type, bool normalized)
{
    if (type == WR_RHI_ATTR_MAT4) {
        for (u8 i = 0; i < 4; ++i) {
            vertex_layout_add(layout, 4, WR_RHI_ATTR_VEC4, normalized, false);
        }
    }
    else {
        vertex_layout_add(layout, num, type, normalized, false);
    }
}

void walrus_vertex_layout_add_int(Walrus_VertexLayout* layout, u8 num, Walrus_Attribute type)
{
    vertex_layout_add(layout, num, type, false, true);
}

void walrus_vertex_layout_end(Walrus_VertexLayout* layout)
{
    layout->stride = walrus_align_up(layout->stride, layout->align);
    u32 hash       = walrus_array_hash(layout, offsetof(Walrus_VertexLayout, hash));
    layout->hash   = hash;
}

void walrus_vertex_layout_decode(Walrus_VertexLayout const* layout, u32 index, u8* attr_id, u8* num,
                                 Walrus_Attribute* type, bool* normalized, bool* as_int)
{
    uint16_t val = layout->attributes[index];
    *attr_id     = (val & 15);
    *num         = ((val >> 4) & 3) + 1;
    *type        = (Walrus_Attribute)((val >> 6) & 15);
    *normalized  = !!(val & (1 << 10));
    *as_int      = !!(val & (1 << 11));
}

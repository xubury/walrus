#include <rhi/vertex_layout.h>
#include <core/math.h>
#include <core/assert.h>
#include <core/hash.h>

#include <string.h>

static u8 s_component_stride[WR_RHI_COMPONENT_COUNT][4] = {
    {1, 2, 4, 4},      // UInt8
    {1, 2, 4, 4},      // Int8
    {2, 4, 6, 8},      // Int16
    {2, 4, 6, 8},      // UInt16
    {4, 8, 12, 16},    // Int32
    {4, 8, 12, 16},    // UInt32
    {4, 8, 12, 16},    // Float
    {12, 12, 12, 12},  // Vec3
    {16, 16, 16, 16},  // Vec4
    {12, 12, 12, 12},  // Mat3
    {16, 16, 16, 16},  // Mat4
};

static u8 s_component_align[WR_RHI_COMPONENT_COUNT] = {
    1,   // UInt8
    1,   // Int8
    2,   // Int16
    2,   // UInt16
    4,   // Int32
    4,   // UInt32
    4,   // Float
    4,   // Vec3
    16,  // Vec4
    4,   // Mat3
    16,  // Mat4
};

struct Walrus_VertexLayout {
    u8  instance_strde;
    u16 stride;
    u16 attributes[16];
    u16 offsets[16];
    u8  num_attributes;
    u32 hash;
};

void walrus_vertex_layout_begin(Walrus_VertexLayout* layout)
{
    walrus_vertex_layout_begin_instance(layout, 0);
}

void walrus_vertex_layout_begin_instance(Walrus_VertexLayout* layout, u8 instance)
{
    layout->instance_strde = instance;
    layout->hash           = 0;
    layout->stride         = 0;
    layout->num_attributes = 0;
    layout->align          = 4;
    memset(layout->attributes, 0, sizeof(layout->attributes));
    memset(layout->offsets, 0, sizeof(layout->offsets));
}

static u16 encode(u8 attr_id, u8 num, Walrus_LayoutComponent type, bool normalized, bool as_int)
{
    walrus_assert_msg(num <= 4 && num > 0, "Invalid number of components!");
    u16 const encoded_attr  = ((u16)attr_id & 15);
    u16 const encoded_num   = ((num - 1) & 3) << 4;
    u16 const encoded_type  = ((u16)type & 15) << 6;
    u16 const encoded_norm  = (normalized & 1) << 10;
    u16 const encoded_asint = (as_int & 1) << 11;
    return encoded_attr | encoded_norm | encoded_type | encoded_num | encoded_asint;
}

static void vertex_layout_add_internal(Walrus_VertexLayout* layout, u8 attr, u8 num, Walrus_LayoutComponent type,
                                       u32 offset, u32 stride, bool normalized, bool as_int)
{
    layout->attributes[layout->num_attributes] = encode(attr, num, type, normalized, as_int);
    layout->offsets[layout->num_attributes]    = offset;
    layout->stride                             = stride;
    ++layout->num_attributes;
    walrus_assert_msg(layout->num_attributes <= walrus_array_len(layout->attributes),
                      "Number of attributes exceeds limit!");
}

static void vertex_layout_add(Walrus_VertexLayout* layout, u8 attr, u8 num, Walrus_LayoutComponent type,
                              bool normalized, bool as_int)
{
    layout->align = walrus_max(layout->align, s_component_align[type]);
    u32 offset    = walrus_align_up(layout->stride, s_component_align[type]);
    u32 stride    = layout->stride + s_component_stride[type][num - 1];
    if (type == WR_RHI_COMPONENT_MAT4) {
        for (u8 i = 0; i < 4; ++i) {
            vertex_layout_add_internal(layout, attr + i, 4, type, offset, stride, normalized, as_int);
            offset = walrus_align_up(layout->stride, s_component_align[type]);
            stride += s_component_stride[type][num - 1];
        }
    }
    else if (type == WR_RHI_COMPONENT_MAT3) {
        for (u8 i = 0; i < 3; ++i) {
            vertex_layout_add_internal(layout, attr + i, 3, type, offset, stride, normalized, as_int);
            offset = walrus_align_up(layout->stride, s_component_align[type]);
            stride += s_component_stride[type][num - 1];
        }
    }
    else {
        vertex_layout_add_internal(layout, attr, num, type, offset, stride, normalized, as_int);
    }
}

void walrus_vertex_layout_add(Walrus_VertexLayout* layout, u8 attr, u8 num, Walrus_LayoutComponent type,
                              bool normalized)
{
    vertex_layout_add(layout, attr, num, type, normalized, false);
}

void walrus_vertex_layout_add_override(Walrus_VertexLayout* layout, u8 attr, u8 num, Walrus_LayoutComponent type,
                                       bool normalized, u32 offset, u32 stride)
{
    vertex_layout_add_internal(layout, attr, num, type, offset, stride, normalized, false);
}

void walrus_vertex_layout_add_int(Walrus_VertexLayout* layout, u8 attr, u8 num, Walrus_LayoutComponent type)
{
    vertex_layout_add(layout, attr, num, type, false, true);
}

void walrus_vertex_layout_end(Walrus_VertexLayout* layout)
{
    layout->stride = walrus_align_up(layout->stride, layout->align);
    u32 hash       = walrus_array_hash(layout, offsetof(Walrus_VertexLayout, hash));
    layout->hash   = hash;
}

void walrus_vertex_layout_decode(Walrus_VertexLayout const* layout, u32 index, u8* attr_id, u8* num,
                                 Walrus_LayoutComponent* type, bool* normalized, bool* as_int)
{
    uint16_t val = layout->attributes[index];
    *attr_id     = (val & 15);
    *num         = ((val >> 4) & 3) + 1;
    *type        = (Walrus_LayoutComponent)((val >> 6) & 15);
    *normalized  = !!(val & (1 << 10));
    *as_int      = !!(val & (1 << 11));
}

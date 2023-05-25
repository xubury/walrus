#include <rhi/vertex_layout.h>
#include <core/math.h>
#include <core/assert.h>
#include <core/hash.h>

#include <string.h>

static u8 s_component_stride[WR_RHI_COMPONENT_COUNT][4] = {
    {1, 2, 4, 4},    // UInt8
    {1, 2, 4, 4},    // Int8
    {2, 4, 6, 8},    // Int16
    {2, 4, 6, 8},    // UInt16
    {4, 8, 12, 16},  // Int32
    {4, 8, 12, 16},  // UInt32
    {4, 8, 12, 16},  // Float
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
    layout->offsets[attr]                      = offset;
    layout->stride                             = stride;
    ++layout->num_attributes;
    walrus_assert_msg(layout->num_attributes <= walrus_array_len(layout->attributes),
                      "Number of attributes exceeds limit!");
}

static void vertex_layout_add(Walrus_VertexLayout* layout, u8 attr, u8 num, Walrus_LayoutComponent type,
                              bool normalized, bool as_int, u8 align)
{
    layout->align = walrus_max(layout->align, align);
    u32 offset    = walrus_align_up(layout->stride, align);
    u32 stride    = layout->stride + s_component_stride[type][num - 1];
    vertex_layout_add_internal(layout, attr, num, type, offset, stride, normalized, as_int);
}

void walrus_vertex_layout_add_mat4(Walrus_VertexLayout* layout, u8 attr)
{
    layout->align = walrus_max(layout->align, 16);
    u32 offset    = walrus_align_up(layout->stride, 16);
    u32 stride    = layout->stride + s_component_stride[WR_RHI_COMPONENT_FLOAT][3];
    walrus_vertex_layout_add_mat4_override(layout, attr, offset, stride);
}

void walrus_vertex_layout_add_mat4_override(Walrus_VertexLayout* layout, u8 attr, u32 offset, u32 stride)
{
    for (u8 i = 0; i < 4; ++i) {
        vertex_layout_add_internal(layout, attr + i, 4, WR_RHI_COMPONENT_FLOAT, offset, stride, false, false);
        offset = walrus_align_up(layout->stride, 16);
        stride += s_component_stride[WR_RHI_COMPONENT_FLOAT][3];
    }
}

void walrus_vertex_layout_add_mat3(Walrus_VertexLayout* layout, u8 attr)
{
    u32 offset = walrus_align_up(layout->stride, 4);
    u32 stride = layout->stride + s_component_stride[WR_RHI_COMPONENT_FLOAT][2];
    walrus_vertex_layout_add_mat4_override(layout, attr, offset, stride);
}

void walrus_vertex_layout_add_mat3_override(Walrus_VertexLayout* layout, u8 attr, u32 offset, u32 stride)
{
    for (u8 i = 0; i < 3; ++i) {
        vertex_layout_add_internal(layout, attr + i, 3, WR_RHI_COMPONENT_FLOAT, offset, stride, false, false);
        offset = walrus_align_up(layout->stride, 4);
        stride += s_component_stride[WR_RHI_COMPONENT_FLOAT][2];
    }
}

void walrus_vertex_layout_add(Walrus_VertexLayout* layout, u8 attr, u8 num, Walrus_LayoutComponent type,
                              bool normalized)
{
    vertex_layout_add(layout, attr, num, type, normalized, false, 4);
}

void walrus_vertex_layout_add_align(Walrus_VertexLayout* layout, u8 attr, u8 num, Walrus_LayoutComponent type,
                                    bool normalized, u8 align)
{
    vertex_layout_add(layout, attr, num, type, normalized, false, align);
}

void walrus_vertex_layout_add_override(Walrus_VertexLayout* layout, u8 attr, u8 num, Walrus_LayoutComponent type,
                                       bool normalized, u32 offset, u32 stride)
{
    vertex_layout_add_internal(layout, attr, num, type, offset, stride, normalized, false);
}

void walrus_vertex_layout_add_int(Walrus_VertexLayout* layout, u8 attr, u8 num, Walrus_LayoutComponent type)
{
    vertex_layout_add(layout, attr, num, type, false, true, 4);
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
    u16 val     = layout->attributes[index];
    *attr_id    = (val & 15);
    *num        = ((val >> 4) & 3) + 1;
    *type       = (Walrus_LayoutComponent)((val >> 6) & 15);
    *normalized = !!(val & (1 << 10));
    *as_int     = !!(val & (1 << 11));
}

#pragma once

#include "rhi_defines.h"

#include <core/type.h>

typedef enum {
    WR_RHI_ATTR_INT8 = 0,
    WR_RHI_ATTR_UINT8,
    WR_RHI_ATTR_INT16,
    WR_RHI_ATTR_UINT16,
    WR_RHI_ATTR_INT32,
    WR_RHI_ATTR_UINT32,
    WR_RHI_ATTR_FLOAT,
    WR_RHI_ATTR_VEC4,
    WR_RHI_ATTR_MAT4,

    WR_RHI_ATTR_COUNT,
} Walrus_Attribute;

typedef struct {
    u8  start_attr;
    u8  instance_strde;
    u64 stride;
    u16 attributes[WR_RHI_MAX_VERTEX_ATTRIBUTES];
    u64 offsets[WR_RHI_MAX_VERTEX_ATTRIBUTES];
    u8  num_attributes;
    u8  align;
    u32 hash;
} Walrus_VertexLayout;

void walrus_vertex_layout_begin(Walrus_VertexLayout* layout, u8 start_attr);
void walrus_vertex_layout_begin_instance(Walrus_VertexLayout* layout, u8 instance, u8 start_attr);
void walrus_vertex_layout_add(Walrus_VertexLayout* layout, u8 num, Walrus_Attribute type, bool normalized);
void walrus_vertex_layout_add_int(Walrus_VertexLayout* layout, u8 num, Walrus_Attribute type);
void walrus_vertex_layout_end(Walrus_VertexLayout* layout);
void walrus_vertex_layout_decode(Walrus_VertexLayout const* layout, u32 index, u8* attr_id, u8* num,
                                 Walrus_Attribute* type, bool* normalized, bool* as_int);

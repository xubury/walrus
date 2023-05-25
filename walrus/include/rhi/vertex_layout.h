#pragma once

#include "rhi_defines.h"

#include <core/type.h>

typedef enum {
    WR_RHI_COMPONENT_INT8 = 0,
    WR_RHI_COMPONENT_UINT8,
    WR_RHI_COMPONENT_INT16,
    WR_RHI_COMPONENT_UINT16,
    WR_RHI_COMPONENT_INT32,
    WR_RHI_COMPONENT_UINT32,
    WR_RHI_COMPONENT_FLOAT,

    WR_RHI_COMPONENT_COUNT,
} Walrus_LayoutComponent;

typedef struct {
    u8  instance_strde;
    u64 stride;
    u16 attributes[WR_RHI_MAX_VERTEX_ATTRIBUTES];
    u64 offsets[WR_RHI_MAX_VERTEX_ATTRIBUTES];
    u8  num_attributes;
    u8  align;
    u32 hash;
} Walrus_VertexLayout;

void walrus_vertex_layout_begin(Walrus_VertexLayout* layout);
void walrus_vertex_layout_begin_instance(Walrus_VertexLayout* layout, u8 instance);
void walrus_vertex_layout_add(Walrus_VertexLayout* layout, u8 attr, u8 num, Walrus_LayoutComponent type,
                              bool normalized);
void walrus_vertex_layout_add_align(Walrus_VertexLayout* layout, u8 attr, u8 num, Walrus_LayoutComponent type,
                                    bool normalized, u8 align);
void walrus_vertex_layout_add_mat4(Walrus_VertexLayout* layout, u8 attr);
void walrus_vertex_layout_add_mat4_override(Walrus_VertexLayout* layout, u8 attr, u32 offset, u32 stride);
void walrus_vertex_layout_add_mat3(Walrus_VertexLayout* layout, u8 attr);
void walrus_vertex_layout_add_mat3_override(Walrus_VertexLayout* layout, u8 attr, u32 offset, u32 stride);
void walrus_vertex_layout_add_override(Walrus_VertexLayout* layout, u8 attr, u8 num, Walrus_LayoutComponent type,
                                       bool normalized, u32 offset, u32 stride);
void walrus_vertex_layout_add_int(Walrus_VertexLayout* layout, u8 attr, u8 num, Walrus_LayoutComponent type);
void walrus_vertex_layout_end(Walrus_VertexLayout* layout);
void walrus_vertex_layout_decode(Walrus_VertexLayout const* layout, u32 index, u8* attr_id, u8* num,
                                 Walrus_LayoutComponent* type, bool* normalized, bool* as_int);

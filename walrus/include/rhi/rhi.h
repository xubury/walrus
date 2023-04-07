#pragma once

#include <rhi/rhi_defines.h>

#include "gl.h"

typedef enum {
    WR_RHI_SUCCESS = 0,

    WR_RHI_INIT_GLEW_ERROR,
    WR_RHI_ALLOC_ERROR
} Walrus_RhiError;

typedef enum {
    WR_RHI_FLAG_NONE = 0,

    WR_RHI_FLAG_OPENGL = 1 << 0,
} Walrus_RhiFlag;

Walrus_RhiError walrus_rhi_init(Walrus_RhiFlag flags);
void            walrus_rhi_shutdown(void);

char const* walrus_rhi_error_msg(void);

void walrus_rhi_set_resolution(i32 width, i32 height);

void walrus_rhi_frame(void);

void walrus_rhi_submit(i16 view_id);

u32  walrus_rhi_compose_rgba(u8 r, u8 g, u8 b, u8 a);
void walrus_rhi_decompose_rgba(u32 rgba, u8* r, u8* g, u8* b, u8* a);

void walrus_rhi_set_view_rect(i16 view_id, i32 x, i32 y, i32 width, i32 height);
void walrus_rhi_set_view_clear(i16 view_id, u16 flags, u32 rgba, f32 depth, u8 stencil);

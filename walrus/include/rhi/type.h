#pragma once

#include <core/handle_alloc.h>
#include <rhi/vertex_layout.h>

#define WR_RHI_HANDLE(name) \
    typedef struct {        \
        Walrus_Handle id;   \
    } name

typedef enum {
    WR_RHI_SUCCESS = 0,

    WR_RHI_INIT_GLEW_ERROR,
    WR_RHI_ALLOC_ERROR,

    WR_RHI_ALLOC_HADNLE_ERROR,
    WR_RHI_HANDLE_INVALID_ERROR,
    WR_RHI_TEXTURE_UNIT_ERROR,
} Walrus_RhiError;

typedef enum {
    WR_RHI_FLAG_NONE = 0,

    WR_RHI_FLAG_OPENGL = 1 << 0,
} Walrus_RhiFlag;

typedef enum {
    WR_RHI_SHADER_COMPUTE,
    WR_RHI_SHADER_VERTEX,
    WR_RHI_SHADER_GEOMETRY,
    WR_RHI_SHADER_FRAGMENT,

    WR_RHI_SHADER_COUNT
} Walrus_ShaderType;

typedef enum {
    WR_RHI_UNIFORM_SAMPLER = 0,

    WR_RHI_UNIFORM_BOOL,

    WR_RHI_UNIFORM_INT,
    WR_RHI_UNIFORM_UINT,
    WR_RHI_UNIFORM_FLOAT,

    WR_RHI_UNIFORM_VEC2,
    WR_RHI_UNIFORM_VEC3,
    WR_RHI_UNIFORM_VEC4,
    WR_RHI_UNIFORM_MAT3,
    WR_RHI_UNIFORM_MAT4,

    WR_RHI_UNIFORM_COUNT,
} Walrus_UniformType;

typedef enum {
    WR_RHI_FORMAT_ALPHA8,

    WR_RHI_FORMAT_R8,
    WR_RHI_FORMAT_R8S,
    WR_RHI_FORMAT_R32I,
    WR_RHI_FORMAT_R32UI,
    WR_RHI_FORMAT_R16F,
    WR_RHI_FORMAT_R32F,

    WR_RHI_FORMAT_RG8,
    WR_RHI_FORMAT_RG8S,
    WR_RHI_FORMAT_RG32I,
    WR_RHI_FORMAT_RG32UI,
    WR_RHI_FORMAT_RG16F,
    WR_RHI_FORMAT_RG32F,

    WR_RHI_FORMAT_RGB8,
    WR_RHI_FORMAT_RGB8S,
    WR_RHI_FORMAT_RGB32I,
    WR_RHI_FORMAT_RGB32UI,
    WR_RHI_FORMAT_RGB16F,
    WR_RHI_FORMAT_RGB32F,

    WR_RHI_FORMAT_RGBA8,
    WR_RHI_FORMAT_RGBA8S,
    WR_RHI_FORMAT_RGBA32I,
    WR_RHI_FORMAT_RGBA32UI,
    WR_RHI_FORMAT_RGBA16F,
    WR_RHI_FORMAT_RGBA32F,

    WR_RHI_FORMAT_DEPTH24,
    WR_RHI_FORMAT_STENCIL8,
    WR_RHI_FORMAT_DEPTH24STENCIL8,

    WR_RHI_FORMAT_COUNT
} Walrus_PixelFormat;

typedef enum {
    WR_RHI_ACCESS_READ,
    WR_RHI_ACCESS_WRITE,
    WR_RHI_ACCESS_READWRITE,

    WR_RHI_ACCESS_COUNT
} Walrus_DataAccess;

typedef enum {
    WR_RHI_RATIO_DOUBLE,
    WR_RHI_RATIO_EQUAL,
    WR_RHI_RATIO_HALF,
    WR_RHI_RATIO_QUARTER,
    WR_RHI_RATIO_EIGHTH,
    WR_RHI_RATIO_SIXTEENTH,

    WR_RHI_RATIO_COUNT
} Walrus_BackBufferRatio;

typedef struct {
    u32                    width;
    u32                    height;
    u32                    depth;
    u8                     num_mipmaps;
    u8                     num_layers;
    Walrus_PixelFormat     format;
    Walrus_BackBufferRatio ratio;
    void const*            data;
    u64                    size;

    // WR_RHI_SAMPLER_NONE set sampler params to default
    // Heres the default values:
    // Wrap: Repeated
    // MinFilter: Nearest
    // MagFilter: Linear
    // MipmapFilter: None.
    u64  flags;
    bool cube_map;
} Walrus_TextureCreateInfo;

WR_RHI_HANDLE(Walrus_ShaderHandle);
WR_RHI_HANDLE(Walrus_ProgramHandle);
WR_RHI_HANDLE(Walrus_UniformHandle);
WR_RHI_HANDLE(Walrus_BufferHandle);
WR_RHI_HANDLE(Walrus_LayoutHandle);
WR_RHI_HANDLE(Walrus_TextureHandle);

typedef struct {
    u8*                 data;
    u32                 size;
    u32                 offset;
    u16                 stride;
    Walrus_BufferHandle handle;
} Walrus_TransientBuffer;

typedef enum {
    WR_RHI_VIEWMODE_DEFAULT,
    WR_RHI_VIEWMODE_SEQUENTIAL,
    WR_RHI_VIEWMODE_DEPTH_ASCENDING,
    WR_RHI_VIEWMODE_DEPTH_DESCENDING,

    WR_RHI_VIEWMODE_COUNT
} Walrus_ViewMode;

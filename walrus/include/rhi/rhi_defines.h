#pragma once

#ifndef WR_RHI_MAX_VIEWS
#define WR_RHI_MAX_VIEWS (25)
#endif

#ifndef WR_RHI_MAX_DRAW_CALLS
#define WR_RHI_MAX_DRAW_CALLS (65536)
#endif

#ifndef WR_RHI_MAX_VERTEX_STREAM
#define WR_RHI_MAX_VERTEX_STREAM (8)
#endif

#ifndef WR_RHI_MAX_SHADERS
#define WR_RHI_MAX_SHADERS (256)
#endif

#ifndef WR_RHI_MAX_PROGRAMS
#define WR_RHI_MAX_PROGRAMS (256)
#endif

#ifndef WR_RHI_MAX_UNIFORMS
#define WR_RHI_MAX_UNIFORMS (256)
#endif

#ifndef WR_RHI_MAX_MATRIX_CACHE
#define WR_RHI_MAX_MATRIX_CACHE (WR_RHI_MAX_DRAW_CALLS + 1)
#endif

#ifndef WR_RHI_MAX_VERTEX_LAYOUTS
#define WR_RHI_MAX_VERTEX_LAYOUTS (128)
#endif

#ifndef WR_RHI_MAX_BUFFERS
#define WR_RHI_MAX_BUFFERS (4096)
#endif

#ifndef WR_RHI_MAX_VERTEX_ATTRIBUTES
#define WR_RHI_MAX_VERTEX_ATTRIBUTES (16)
#endif

#ifndef WR_RHI_MAX_TEXTURES
#define WR_RHI_MAX_TEXTURES (4096)
#endif

#ifndef WR_RHI_MAX_TEXTURE_SAMPLERS
#define WR_RHI_MAX_TEXTURE_SAMPLERS 32
#endif

#define WR_RHI_CLEAR_NONE            UINT16_C(0x0000)  //!< No clear flags.
#define WR_RHI_CLEAR_COLOR           UINT16_C(0x0001)  //!< Clear color.
#define WR_RHI_CLEAR_DEPTH           UINT16_C(0x0002)  //!< Clear depth.
#define WR_RHI_CLEAR_STENCIL         UINT16_C(0x0004)  //!< Clear stencil.
#define WR_RHI_CLEAR_DISCARD_COLOR_0 UINT16_C(0x0008)  //!< Discard frame buffer attachment 0.
#define WR_RHI_CLEAR_DISCARD_COLOR_1 UINT16_C(0x0010)  //!< Discard frame buffer attachment 1.
#define WR_RHI_CLEAR_DISCARD_COLOR_2 UINT16_C(0x0020)  //!< Discard frame buffer attachment 2.
#define WR_RHI_CLEAR_DISCARD_COLOR_3 UINT16_C(0x0040)  //!< Discard frame buffer attachment 3.
#define WR_RHI_CLEAR_DISCARD_COLOR_4 UINT16_C(0x0080)  //!< Discard frame buffer attachment 4.
#define WR_RHI_CLEAR_DISCARD_COLOR_5 UINT16_C(0x0100)  //!< Discard frame buffer attachment 5.
#define WR_RHI_CLEAR_DISCARD_COLOR_6 UINT16_C(0x0200)  //!< Discard frame buffer attachment 6.
#define WR_RHI_CLEAR_DISCARD_COLOR_7 UINT16_C(0x0400)  //!< Discard frame buffer attachment 7.
#define WR_RHI_CLEAR_DISCARD_DEPTH   UINT16_C(0x0800)  //!< Discard frame buffer depth attachment.
#define WR_RHI_CLEAR_DISCARD_STENCIL UINT16_C(0x1000)  //!< Discard frame buffer stencil attachment.
#define WR_RHI_CLEAR_DISCARD_COLOR_MASK                                                               \
    (0 | WR_RHI_CLEAR_DISCARD_COLOR_0 | WR_RHI_CLEAR_DISCARD_COLOR_1 | WR_RHI_CLEAR_DISCARD_COLOR_2 | \
     WR_RHI_CLEAR_DISCARD_COLOR_3 | WR_RHI_CLEAR_DISCARD_COLOR_4 | WR_RHI_CLEAR_DISCARD_COLOR_5 |     \
     WR_RHI_CLEAR_DISCARD_COLOR_6 | WR_RHI_CLEAR_DISCARD_COLOR_7)

#define WR_RHI_DISCARD_NONE           UINT8_C(0x00)  //!< Preserve everything.
#define WR_RHI_DISCARD_BINDINGS       UINT8_C(0x01)  //!< Discard texture sampler and buffer bindings.
#define WR_RHI_DISCARD_INDEX_BUFFER   UINT8_C(0x02)  //!< Discard index buffer.
#define WR_RHI_DISCARD_INSTANCE_DATA  UINT8_C(0x04)  //!< Discard instance data.
#define WR_RHI_DISCARD_STATE          UINT8_C(0x08)  //!< Discard state and uniform bindings.
#define WR_RHI_DISCARD_TRANSFORM      UINT8_C(0x10)  //!< Discard transform.
#define WR_RHI_DISCARD_VERTEX_STREAMS UINT8_C(0x20)  //!< Discard vertex streams.
#define WR_RHI_DISCARD_ALL            UINT8_C(0xff)  //!< Discard all states.

/* Texture flags */
#define WR_RHI_TEXTURE_NONE          UINT64_C(0x0000000000000000)
#define WR_RHI_TEXTURE_MSAA_SAMPLE   UINT64_C(0x0000000800000000)  //!< Texture will be used for MSAA sampling.
#define WR_RHI_TEXTURE_RT            UINT64_C(0x0000001000000000)  //!< Render target no MSAA.
#define WR_RHI_TEXTURE_COMPUTE_WRITE UINT64_C(0x0000100000000000)  //!< Texture will be used for compute write.
#define WR_RHI_TEXTURE_SRGB          UINT64_C(0x0000200000000000)  //!< Sample texture as sRGB.
#define WR_RHI_TEXTURE_BLIT_DST      UINT64_C(0x0000400000000000)  //!< Texture will be used as blit destination.

#define WR_RHI_TEXTURE_RT_MSAA_X2    UINT64_C(0x0000002000000000)  //!< Render target MSAAx2 mode.
#define WR_RHI_TEXTURE_RT_MSAA_X4    UINT64_C(0x0000003000000000)  //!< Render target MSAAx4 mode.
#define WR_RHI_TEXTURE_RT_MSAA_X8    UINT64_C(0x0000004000000000)  //!< Render target MSAAx8 mode.
#define WR_RHI_TEXTURE_RT_MSAA_X16   UINT64_C(0x0000005000000000)  //!< Render target MSAAx16 mode.
#define WR_RHI_TEXTURE_RT_MSAA_SHIFT 36

#define WR_RHI_TEXTURE_RT_MSAA_MASK UINT64_C(0x0000007000000000)

#define WR_RHI_TEXTURE_RT_WRITE_ONLY UINT64_C(0x0000008000000000)  //!< Render target will be used for writing
#define WR_RHI_TEXTURE_RT_SHIFT      36

#define WR_RHI_TEXTURE_RT_MASK UINT64_C(0x000000f000000000)

/**
 * Sampler flags.
 *
 */
#define WR_RHI_SAMPLER_U_MIRROR UINT32_C(0x00000001)  //!< Wrap U mode: Mirror
#define WR_RHI_SAMPLER_U_CLAMP  UINT32_C(0x00000002)  //!< Wrap U mode: Clamp
#define WR_RHI_SAMPLER_U_BORDER UINT32_C(0x00000003)  //!< Wrap U mode: Border
#define WR_RHI_SAMPLER_U_SHIFT  0

#define WR_RHI_SAMPLER_U_MASK UINT32_C(0x00000003)

#define WR_RHI_SAMPLER_V_MIRROR UINT32_C(0x00000004)  //!< Wrap V mode: Mirror
#define WR_RHI_SAMPLER_V_CLAMP  UINT32_C(0x00000008)  //!< Wrap V mode: Clamp
#define WR_RHI_SAMPLER_V_BORDER UINT32_C(0x0000000c)  //!< Wrap V mode: Border
#define WR_RHI_SAMPLER_V_SHIFT  2

#define WR_RHI_SAMPLER_V_MASK UINT32_C(0x0000000c)

#define WR_RHI_SAMPLER_W_MIRROR UINT32_C(0x00000010)  //!< Wrap W mode: Mirror
#define WR_RHI_SAMPLER_W_CLAMP  UINT32_C(0x00000020)  //!< Wrap W mode: Clamp
#define WR_RHI_SAMPLER_W_BORDER UINT32_C(0x00000030)  //!< Wrap W mode: Border
#define WR_RHI_SAMPLER_W_SHIFT  4

#define WR_RHI_SAMPLER_W_MASK UINT32_C(0x00000030)

#define WR_RHI_SAMPLER_MIN_NEAREST UINT32_C(0x00000040)  //!< Min sampling mode: Nearest
#define WR_RHI_SAMPLER_MIN_LINEAR  UINT32_C(0x00000080)  //!< Min sampling mode: Linear
#define WR_RHI_SAMPLER_MIN_SHIFT   6

#define WR_RHI_SAMPLER_MIN_MASK UINT32_C(0x000000c0)

#define WR_RHI_SAMPLER_MAG_NEAREST UINT32_C(0x00000100)  //!< Mag sampling mode: Nearest
#define WR_RHI_SAMPLER_MAG_LINEAR  UINT32_C(0x00000200)  //!< Mag sampling mode: Linear
#define WR_RHI_SAMPLER_MAG_SHIFT   8

#define WR_RHI_SAMPLER_MAG_MASK UINT32_C(0x00000300)

#define WR_RHI_SAMPLER_MIP_NEAREST UINT32_C(0x00000400)  //!< Mip sampling mode: Nearest
#define WR_RHI_SAMPLER_MIP_LINEAR  UINT32_C(0x00000800)  //!< Mip sampling mode: Linear
#define WR_RHI_SAMPLER_MIP_SHIFT   10

#define WR_RHI_SAMPLER_MIP_MASK UINT32_C(0x00000c00)

#define WR_RHI_SAMPLER_BORDER_COLOR_SHIFT 24

#define WR_RHI_SAMPLER_BORDER_COLOR_MASK UINT32_C(0x0f000000)
#define WR_RHI_SAMPLER_BORDER_COLOR(v) \
    (((uint32_t)(v) << WR_RHI_SAMPLER_BORDER_COLOR_SHIFT) & WR_RHI_SAMPLER_BORDER_COLOR_MASK)

#define WR_RHI_SAMPLER_RESERVED_SHIFT 28

#define WR_RHI_SAMPLER_RESERVED_MASK UINT32_C(0xf0000000)

#define WR_RHI_SAMPLER_NONE           UINT32_C(0x00000000)
#define WR_RHI_SAMPLER_SAMPLE_STENCIL UINT32_C(0x00100000)  //!< Sample stencil instead of depth.
#define WR_RHI_SAMPLER_NEAREST        (WR_RHI_SAMPLER_MIN_NEAREST | WR_RHI_SAMPLER_MAG_NEAREST | WR_RHI_SAMPLER_MIP_NEAREST)
#define WR_RHI_SAMPLER_LINEAR         (WR_RHI_SAMPLER_MIN_LINEAR | WR_RHI_SAMPLER_MAG_LINEAR | WR_RHI_SAMPLER_MIP_LINEAR)

#define WR_RHI_SAMPLER_UVW_MIRROR (WR_RHI_SAMPLER_U_MIRROR | WR_RHI_SAMPLER_V_MIRROR | WR_RHI_SAMPLER_W_MIRROR)

#define WR_RHI_SAMPLER_UVW_CLAMP (WR_RHI_SAMPLER_U_CLAMP | WR_RHI_SAMPLER_V_CLAMP | WR_RHI_SAMPLER_W_CLAMP)

#define WR_RHI_SAMPLER_UVW_BORDER (WR_RHI_SAMPLER_U_BORDER | WR_RHI_SAMPLER_V_BORDER | WR_RHI_SAMPLER_W_BORDER)

#define WR_RHI_SAMPLER_BITS_MASK                                                                       \
    (WR_RHI_SAMPLER_U_MASK | WR_RHI_SAMPLER_V_MASK | WR_RHI_SAMPLER_W_MASK | WR_RHI_SAMPLER_MIN_MASK | \
     WR_RHI_SAMPLER_MAG_MASK | WR_RHI_SAMPLER_MIP_MASK | WR_RHI_SAMPLER_COMPARE_MASK)

#define WR_RHI_BUFFER_NONE               UINT16_C(0x0000)
#define WR_RHI_BUFFER_COMPUTE_READ       UINT16_C(0x0100)  //!< Buffer will be read by shader.
#define WR_RHI_BUFFER_COMPUTE_WRITE      UINT16_C(0x0200)  //!< Buffer will be used for writing.
#define WR_RHI_BUFFER_DRAW_INDIRECT      UINT16_C(0x0400)  //!< Buffer will be used for storing draw indirect commands.
#define WR_RHI_BUFFER_ALLOW_RESIZE       UINT16_C(0x0800)  //!< Allow dynamic index/vertex buffer resize during update.
#define WR_RHI_BUFFER_INDEX              UINT16_C(0x1000)  //!< Index buffer.
#define WR_RHI_BUFFER_UNIFORM_BLOCK      UINT16_C(0x2000)  //!< Uniform block buffer.
#define WR_RHI_BUFFER_CLIENT_READ        UINT16_C(0x4000)  //!< Buffer will be read by the application.
#define WR_RHI_BUFFER_COMPUTE_READ_WRITE (BD_BUFFER_COMPUTE_READ | BD_BUFFER_COMPUTE_WRITE)

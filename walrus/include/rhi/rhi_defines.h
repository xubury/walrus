#pragma once

#ifndef WR_RHI_MIN_RESOURCE_COMMAND_BUFFER_SIZE
#define WR_RHI_MIN_RESOURCE_COMMAND_BUFFER_SIZE (64 << 10)
#endif

#ifndef WR_RHI_MIN_TRANSIENT_BUFFER_SIZE
#define WR_RHI_MIN_TRANSIENT_BUFFER_SIZE (6 << 25)
#endif

#ifndef WR_RHI_MIN_TRANSIENT_INDEX_BUFFER_SIZE
#define WR_RHI_MIN_TRANSIENT_INDEX_BUFFER_SIZE (6 << 12)
#endif

#ifndef WR_RHI_MAX_VIEWS
#define WR_RHI_MAX_VIEWS (25)
#endif

#ifndef WR_RHI_MAX_DRAW_CALLS
#define WR_RHI_MAX_DRAW_CALLS (65536)
#endif

#ifndef WR_RHI_MAX_VERTEX_STREAM
#define WR_RHI_MAX_VERTEX_STREAM (15)
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

/**
 * Set stencil ref value.
 *
 */
#define WR_RHI_STENCIL_FUNC_REF_SHIFT 0

#define WR_RHI_STENCIL_FUNC_REF_MASK UINT32_C(0x000000ff)
#define WR_RHI_STENCIL_FUNC_REF(v)   (((uint32_t)(v) << WR_RHI_STENCIL_FUNC_REF_SHIFT) & WR_RHI_STENCIL_FUNC_REF_MASK)

/**
 * Set stencil rmask value.
 *
 */
#define WR_RHI_STENCIL_FUNC_RMASK_SHIFT 8

#define WR_RHI_STENCIL_FUNC_RMASK_MASK UINT32_C(0x0000ff00)
#define WR_RHI_STENCIL_FUNC_RMASK(v) \
    (((uint32_t)(v) << WR_RHI_STENCIL_FUNC_RMASK_SHIFT) & WR_RHI_STENCIL_FUNC_RMASK_MASK)

#define WR_RHI_STENCIL_NONE    UINT32_C(0x00000000)
#define WR_RHI_STENCIL_MASK    UINT32_C(0xffffffff)
#define WR_RHI_STENCIL_DEFAULT UINT32_C(0x00000000)

#define WR_RHI_STENCIL_TEST_LESS     UINT32_C(0x00010000)  //!< Enable stencil test, less.
#define WR_RHI_STENCIL_TEST_LEQUAL   UINT32_C(0x00020000)  //!< Enable stencil test, less or equal.
#define WR_RHI_STENCIL_TEST_EQUAL    UINT32_C(0x00030000)  //!< Enable stencil test, equal.
#define WR_RHI_STENCIL_TEST_GEQUAL   UINT32_C(0x00040000)  //!< Enable stencil test, greater or equal.
#define WR_RHI_STENCIL_TEST_GREATER  UINT32_C(0x00050000)  //!< Enable stencil test, greater.
#define WR_RHI_STENCIL_TEST_NOTEQUAL UINT32_C(0x00060000)  //!< Enable stencil test, not equal.
#define WR_RHI_STENCIL_TEST_NEVER    UINT32_C(0x00070000)  //!< Enable stencil test, never.
#define WR_RHI_STENCIL_TEST_ALWAYS   UINT32_C(0x00080000)  //!< Enable stencil test, always.
#define WR_RHI_STENCIL_TEST_SHIFT    16                    //!< Stencil test bit shift
#define WR_RHI_STENCIL_TEST_MASK     UINT32_C(0x000f0000)  //!< Stencil test bit mask

#define WR_RHI_STENCIL_OP_FAIL_S_ZERO    UINT32_C(0x00000000)  //!< Zero.
#define WR_RHI_STENCIL_OP_FAIL_S_KEEP    UINT32_C(0x00100000)  //!< Keep.
#define WR_RHI_STENCIL_OP_FAIL_S_REPLACE UINT32_C(0x00200000)  //!< Replace.
#define WR_RHI_STENCIL_OP_FAIL_S_INCR    UINT32_C(0x00300000)  //!< Increment and wrap.
#define WR_RHI_STENCIL_OP_FAIL_S_INCRSAT UINT32_C(0x00400000)  //!< Increment and clamp.
#define WR_RHI_STENCIL_OP_FAIL_S_DECR    UINT32_C(0x00500000)  //!< Decrement and wrap.
#define WR_RHI_STENCIL_OP_FAIL_S_DECRSAT UINT32_C(0x00600000)  //!< Decrement and clamp.
#define WR_RHI_STENCIL_OP_FAIL_S_INVERT  UINT32_C(0x00700000)  //!< Invert.
#define WR_RHI_STENCIL_OP_FAIL_S_SHIFT   20                    //!< Stencil operation fail bit shift
#define WR_RHI_STENCIL_OP_FAIL_S_MASK    UINT32_C(0x00f00000)  //!< Stencil operation fail bit mask

#define WR_RHI_STENCIL_OP_FAIL_Z_ZERO    UINT32_C(0x00000000)  //!< Zero.
#define WR_RHI_STENCIL_OP_FAIL_Z_KEEP    UINT32_C(0x01000000)  //!< Keep.
#define WR_RHI_STENCIL_OP_FAIL_Z_REPLACE UINT32_C(0x02000000)  //!< Replace.
#define WR_RHI_STENCIL_OP_FAIL_Z_INCR    UINT32_C(0x03000000)  //!< Increment and wrap.
#define WR_RHI_STENCIL_OP_FAIL_Z_INCRSAT UINT32_C(0x04000000)  //!< Increment and clamp.
#define WR_RHI_STENCIL_OP_FAIL_Z_DECR    UINT32_C(0x05000000)  //!< Decrement and wrap.
#define WR_RHI_STENCIL_OP_FAIL_Z_DECRSAT UINT32_C(0x06000000)  //!< Decrement and clamp.
#define WR_RHI_STENCIL_OP_FAIL_Z_INVERT  UINT32_C(0x07000000)  //!< Invert.
#define WR_RHI_STENCIL_OP_FAIL_Z_SHIFT   24                    //!< Stencil operation depth fail bit shift
#define WR_RHI_STENCIL_OP_FAIL_Z_MASK    UINT32_C(0x0f000000)  //!< Stencil operation depth fail bit mask

#define WR_RHI_STENCIL_OP_PASS_Z_ZERO    UINT32_C(0x00000000)  //!< Zero.
#define WR_RHI_STENCIL_OP_PASS_Z_KEEP    UINT32_C(0x10000000)  //!< Keep.
#define WR_RHI_STENCIL_OP_PASS_Z_REPLACE UINT32_C(0x20000000)  //!< Replace.
#define WR_RHI_STENCIL_OP_PASS_Z_INCR    UINT32_C(0x30000000)  //!< Increment and wrap.
#define WR_RHI_STENCIL_OP_PASS_Z_INCRSAT UINT32_C(0x40000000)  //!< Increment and clamp.
#define WR_RHI_STENCIL_OP_PASS_Z_DECR    UINT32_C(0x50000000)  //!< Decrement and wrap.
#define WR_RHI_STENCIL_OP_PASS_Z_DECRSAT UINT32_C(0x60000000)  //!< Decrement and clamp.
#define WR_RHI_STENCIL_OP_PASS_Z_INVERT  UINT32_C(0x70000000)  //!< Invert.
#define WR_RHI_STENCIL_OP_PASS_Z_SHIFT   28                    //!< Stencil operation depth pass bit shift
#define WR_RHI_STENCIL_OP_PASS_Z_MASK    UINT32_C(0xf0000000)  //!< Stencil operation depth pass bit mask

#define WR_RHI_STATE_MASK UINT64_C(0xffffffffffffffff)  //!< State bit mask
/**
 * Color RGB/alpha/depth write. When it's not specified write will be disabled.
 */
#define WR_RHI_STATE_WRITE_R UINT64_C(0x0000000000000001)  //!< Enable R write.
#define WR_RHI_STATE_WRITE_G UINT64_C(0x0000000000000002)  //!< Enable G write.
#define WR_RHI_STATE_WRITE_B UINT64_C(0x0000000000000004)  //!< Enable B write.
#define WR_RHI_STATE_WRITE_A UINT64_C(0x0000000000000008)  //!< Enable alpha write.
#define WR_RHI_STATE_WRITE_Z UINT64_C(0x0000004000000000)  //!< Enable depth write.
/// Enable RGB write.
#define WR_RHI_STATE_WRITE_RGB (0 | WR_RHI_STATE_WRITE_R | WR_RHI_STATE_WRITE_G | WR_RHI_STATE_WRITE_B)

/// Write all channels mask.
#define WR_RHI_STATE_WRITE_MASK (0 | WR_RHI_STATE_WRITE_RGB | WR_RHI_STATE_WRITE_A | WR_RHI_STATE_WRITE_Z)

/**
 * Depth test state. When `WR_RHI_STATE_DEPTH_` is not specified depth test will be disabled.
 */
#define WR_RHI_STATE_DEPTH_TEST_LESS     UINT64_C(0x0000000000000010)  //!< Enable depth test, less.
#define WR_RHI_STATE_DEPTH_TEST_LEQUAL   UINT64_C(0x0000000000000020)  //!< Enable depth test, less or equal.
#define WR_RHI_STATE_DEPTH_TEST_EQUAL    UINT64_C(0x0000000000000030)  //!< Enable depth test, equal.
#define WR_RHI_STATE_DEPTH_TEST_GEQUAL   UINT64_C(0x0000000000000040)  //!< Enable depth test, greater or equal.
#define WR_RHI_STATE_DEPTH_TEST_GREATER  UINT64_C(0x0000000000000050)  //!< Enable depth test, greater.
#define WR_RHI_STATE_DEPTH_TEST_NOTEQUAL UINT64_C(0x0000000000000060)  //!< Enable depth test, not equal.
#define WR_RHI_STATE_DEPTH_TEST_NEVER    UINT64_C(0x0000000000000070)  //!< Enable depth test, never.
#define WR_RHI_STATE_DEPTH_TEST_ALWAYS   UINT64_C(0x0000000000000080)  //!< Enable depth test, always.
#define WR_RHI_STATE_DEPTH_TEST_SHIFT    4                             //!< Depth test state bit shift
#define WR_RHI_STATE_DEPTH_TEST_MASK     UINT64_C(0x00000000000000f0)  //!< Depth test state bit mask

/**
 * Use WR_RHI_STATE_BLEND_FUNC(_src, _dst) or WR_RHI_STATE_BLEND_FUNC_SEPARATE(_srcRGB, _dstRGB, _srcA, _dstA)
 * helper macros.
 */
#define WR_RHI_STATE_BLEND_ZERO          UINT64_C(0x0000000000001000)  //!< 0, 0, 0, 0
#define WR_RHI_STATE_BLEND_ONE           UINT64_C(0x0000000000002000)  //!< 1, 1, 1, 1
#define WR_RHI_STATE_BLEND_SRC_COLOR     UINT64_C(0x0000000000003000)  //!< Rs, Gs, Bs, As
#define WR_RHI_STATE_BLEND_INV_SRC_COLOR UINT64_C(0x0000000000004000)  //!< 1-Rs, 1-Gs, 1-Bs, 1-As
#define WR_RHI_STATE_BLEND_SRC_ALPHA     UINT64_C(0x0000000000005000)  //!< As, As, As, As
#define WR_RHI_STATE_BLEND_INV_SRC_ALPHA UINT64_C(0x0000000000006000)  //!< 1-As, 1-As, 1-As, 1-As
#define WR_RHI_STATE_BLEND_DST_ALPHA     UINT64_C(0x0000000000007000)  //!< Ad, Ad, Ad, Ad
#define WR_RHI_STATE_BLEND_INV_DST_ALPHA UINT64_C(0x0000000000008000)  //!< 1-Ad, 1-Ad, 1-Ad ,1-Ad
#define WR_RHI_STATE_BLEND_DST_COLOR     UINT64_C(0x0000000000009000)  //!< Rd, Gd, Bd, Ad
#define WR_RHI_STATE_BLEND_INV_DST_COLOR UINT64_C(0x000000000000a000)  //!< 1-Rd, 1-Gd, 1-Bd, 1-Ad
#define WR_RHI_STATE_BLEND_SRC_ALPHA_SAT UINT64_C(0x000000000000b000)  //!< f, f, f, 1; f = min(As, 1-Ad)
#define WR_RHI_STATE_BLEND_FACTOR        UINT64_C(0x000000000000c000)  //!< Blend factor
#define WR_RHI_STATE_BLEND_INV_FACTOR    UINT64_C(0x000000000000d000)  //!< 1-Blend factor
#define WR_RHI_STATE_BLEND_SHIFT         12                            //!< Blend state bit shift
#define WR_RHI_STATE_BLEND_MASK          UINT64_C(0x000000000ffff000)  //!< Blend state bit mask

/// Blend function separate.
#define WR_RHI_STATE_BLEND_FUNC_SEPARATE(_srcRGB, _dstRGB, _srcA, _dstA)  \
    (UINT64_C(0) | (((uint64_t)(_srcRGB) | ((uint64_t)(_dstRGB) << 4))) | \
     (((uint64_t)(_srcA) | ((uint64_t)(_dstA) << 4)) << 8))

/// Blend function.
#define WR_RHI_STATE_BLEND_FUNC(_src, _dst) WR_RHI_STATE_BLEND_FUNC_SEPARATE(_src, _dst, _src, _dst)

#define WR_RHI_STATE_BLEND_ALPHA \
    (0 | WR_RHI_STATE_BLEND_FUNC(WR_RHI_STATE_BLEND_SRC_ALPHA, WR_RHI_STATE_BLEND_INV_SRC_ALPHA))

#define WR_RHI_STATE_NONE UINT64_C(0)
#define WR_RHI_STATE_DEFAULT \
    (0 | WR_RHI_STATE_WRITE_RGB | WR_RHI_STATE_WRITE_A | WR_RHI_STATE_WRITE_Z | WR_RHI_STATE_DEPTH_TEST_LESS)

/**
 * Cull state. When `WR_RHI_STATE_CULL_*` is not specified culling will be disabled.
 *
 */
#define WR_RHI_STATE_CULL_CW    UINT64_C(0x0000001000000000)  //!< Cull clockwise triangles.
#define WR_RHI_STATE_CULL_CCW   UINT64_C(0x0000002000000000)  //!< Cull counter-clockwise triangles.
#define WR_RHI_STATE_CULL_SHIFT 36                            //!< Culling mode bit shift
#define WR_RHI_STATE_CULL_MASK  UINT64_C(0x0000003000000000)  //!< Culling mode bit mask

/* Primitive draw mode */
#define WR_RHI_STATE_DRAW_TRIANGLE_STRIP UINT64_C(0x0000010000000000)  //!< Triangle Strip
#define WR_RHI_STATE_DRAW_LINE           UINT64_C(0x0000020000000000)  //!< Line
#define WR_RHI_STATE_DRAW_SHIFT          40                            //!< Primitive mode bit shift
#define WR_RHI_STATE_DRAW_MASK           UINT64_C(0x00000f0000000000)  //!< Primitive mode bit mask

#define WR_RHI_STATE_WIREFRAME UINT64_C(0x0000100000000000)  //!< Wireframe

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
#define WR_RHI_BUFFER_COMPUTE_READ_WRITE (WR_RHI_BUFFER_COMPUTE_READ | WR_RHI_BUFFER_COMPUTE_WRITE)

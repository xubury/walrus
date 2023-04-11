#pragma once

#ifndef WR_RHI_MAX_VIEWS
#define WR_RHI_MAX_VIEWS 25
#endif

#ifndef WR_RHI_MAX_DRAW_CALLS
#define WR_RHI_MAX_DRAW_CALLS 65536
#endif

#ifndef WR_RHI_MAX_VERTEX_STREAM
#define WR_RHI_MAX_VERTEX_STREAM 8
#endif

#ifndef WR_RHI_MAX_SHADERS
#define WR_RHI_MAX_SHADERS 256
#endif

#ifndef WR_RHI_MAX_PROGRAMS
#define WR_RHI_MAX_PROGRAMS 256
#endif

#ifndef WR_RHI_MAX_UNIFORMS
#define WR_RHI_MAX_UNIFORMS 256
#endif

#ifndef WR_RHI_MAX_MATRIX_CACHE
#define WR_RHI_MAX_MATRIX_CACHE (WR_RHI_MAX_DRAW_CALLS + 1)
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

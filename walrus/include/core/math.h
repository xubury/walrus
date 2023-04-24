#pragma once

#include "macro.h"

#define walrus_max(a, b) ((a) > (b) ? a : b)

#define walrus_min(a, b) ((a) < (b) ? a : b)

#define walrus_clamp(v, low, high) walrus_min(walrus_max(v, low), high)

#define walrus_abs(a) ((a) > 0 ? (a) : -(a))

WR_INLINE u32 walrus_u32add(u32 a, u32 b)
{
    return a + b;
}

WR_INLINE u32 walrus_u32sub(u32 a, u32 b)
{
    return a - b;
}

WR_INLINE u32 walrus_u32cmplt(u32 a, u32 b)
{
    return -(a < b);
}

WR_INLINE u32 walrus_u32cmple(u32 a, u32 b)
{
    return -(a <= b);
}

WR_INLINE u32 walrus_u32or(u32 a, u32 b)
{
    return a | b;
}

WR_INLINE u32 walrus_u32and(u32 a, u32 b)
{
    return a & b;
}

WR_INLINE u32 walrus_u32andc(u32 a, u32 b)
{
    return a & ~b;
}

WR_INLINE u32 walrus_u32not(u32 a)
{
    return ~a;
}

WR_INLINE u32 walrus_u32dec(u32 a)
{
    return a - 1;
}

WR_INLINE u32 walrus_u32srl(u32 a, u32 sa)
{
    return a >> sa;
}

WR_INLINE u32 walrus_u32mod(u32 a, u32 b)
{
    return a % b;
}

WR_INLINE u32 walrus_u32cmpeq(u32 a, u32 b)
{
    return -(a == b);
}

WR_INLINE u32 walrus_u32selb(u32 mask, u32 a, u32 b)
{
    u32 const sel_a  = walrus_u32and(a, mask);
    u32 const sel_b  = walrus_u32andc(b, mask);
    u32 const result = walrus_u32or(sel_a, sel_b);

    return (result);
}

WR_INLINE u32 walrus_u32satadd(u32 a, u32 b)
{
    u32 const add    = walrus_u32add(a, b);
    u32 const lt     = walrus_u32cmplt(add, a);
    u32 const result = walrus_u32or(add, lt);
    return result;
}

WR_INLINE u32 walrus_u32satsub(u32 a, u32 b)
{
    u32 const sub    = walrus_u32sub(a, b);
    u32 const le     = walrus_u32cmple(sub, a);
    u32 const result = walrus_u32and(sub, le);

    return result;
}

WR_INLINE u32 walrus_u32cntbits(u32 val)
{
#if WR_COMPILER == WR_COMPILER_GCC || WR_COMPILER == WR_COMPILER_CLANG
    return __builtin_popcount(val);
#else
    u32 const tmp0   = walrus_u32srl(val, 1);
    u32 const tmp1   = walrus_u32and(tmp0, 0x55555555);
    u32 const tmp2   = walrus_u32sub(val, tmp1);
    u32 const tmp3   = walrus_u32and(tmp2, 0xc30c30c3);
    u32 const tmp4   = walrus_u32srl(tmp2, 2);
    u32 const tmp5   = walrus_u32and(tmp4, 0xc30c30c3);
    u32 const tmp6   = walrus_u32srl(tmp2, 4);
    u32 const tmp7   = walrus_u32and(tmp6, 0xc30c30c3);
    u32 const tmp8   = walrus_u32add(tmp3, tmp5);
    u32 const tmp9   = walrus_u32add(tmp7, tmp8);
    u32 const tmpA   = walrus_u32srl(tmp9, 6);
    u32 const tmpB   = walrus_u32add(tmp9, tmpA);
    u32 const tmpC   = walrus_u32srl(tmpB, 12);
    u32 const tmpD   = walrus_u32srl(tmpB, 24);
    u32 const tmpE   = walrus_u32add(tmpB, tmpC);
    u32 const tmpF   = walrus_u32add(tmpD, tmpE);
    u32 const result = walrus_u32and(tmpF, 0x3f);

    return result;
#endif
}

WR_INLINE u32 walrus_u32cnttz(u32 val)
{
#if WR_COMPILER == WR_COMPILER_GCC || WR_COMPILER == WR_COMPILER_CLANG
    return 0 == val ? 64 : __builtin_ctzll(val);
#else
    u32 const tmp0   = walrus_u32not(val);
    u32 const tmp1   = walrus_u32dec(val);
    u32 const tmp2   = walrus_u32and(tmp0, tmp1);
    u32 const result = walrus_u32cntbits(tmp2);

    return result;
#endif
}

WR_INLINE u8 walrus_u8ror(u8 x, u8 n)
{
    return (x << (sizeof(u8) * 8 - n) | (x >> n));
}

WR_INLINE u8 walrus_u8rol(u8 x, u8 n)
{
    return (x >> (sizeof(u8) * 8 - n) | (x << n));
}

WR_INLINE uint32_t walrus_stride_align(uint32_t offset, uint32_t stride)
{
    const uint32_t mod    = walrus_u32mod(offset, stride);
    const uint32_t add    = walrus_u32sub(stride, mod);
    const uint32_t mask   = walrus_u32cmpeq(mod, 0);
    const uint32_t tmp    = walrus_u32selb(mask, 0, add);
    const uint32_t result = walrus_u32add(offset, tmp);

    return result;
}

WR_INLINE u32 walrus_align_up(u32 a, u32 align)
{
    u32 const mask = align - 1;
    return (a + mask) & ~mask;
}

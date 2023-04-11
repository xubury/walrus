#pragma once

#include "macro.h"

#define walrus_max(a, b) (a > b ? a : b)

#define walrus_min(a, b) (a < b ? a : b)

WR_INLINE u32 walrus_u32add(u32 a, u32 b)
{
    return a + b;
}

WR_INLINE u32 walrus_u32cmplt(u32 a, u32 b)
{
    return -(a < b);
}

WR_INLINE u32 walrus_u32or(u32 a, u32 b)
{
    return a | b;
}

WR_INLINE u32 walrus_u32satadd(u32 a, u32 b)
{
    u32 const add    = walrus_u32add(a, b);
    u32 const lt     = walrus_u32cmplt(add, a);
    u32 const result = walrus_u32or(add, lt);
    return result;
}

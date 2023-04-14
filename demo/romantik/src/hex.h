#pragma once

#include <core/type.h>

// Pointy top hex conversion, reference: https://www.redblobgames.com/grids/hexagons

void hex_pixel_to_qr(f32 size, f32 x, f32 y, i32 *q, i32 *r);

void hex_qr_to_pixel(f32 size, i32 q, i32 r, f32 *x, f32 *y);

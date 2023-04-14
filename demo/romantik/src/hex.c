#include "hex.h"

#include <core/type.h>
#include <math.h>

void hex_pixel_to_qr(f32 size, f32 x, f32 y, i32 *q, i32 *r)
{
    f32 const sqrt3 = sqrt(3);

    *q = round((sqrt3 / 3.0 * x - 1.0 / 3.0 * y) / size);
    *r = round((2.0 / 3.0 * y) / size);
}

void hex_qr_to_pixel(f32 size, i32 q, i32 r, f32 *x, f32 *y)
{
    f32 const sqrt3 = sqrt(3);

    *x = (sqrt3 * q + sqrt3 / 2.0 * r) * size;
    *y = (3.0 / 2.0 * r) * size;
}

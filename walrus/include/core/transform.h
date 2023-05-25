#pragma once

#include <cglm/types.h>

typedef struct {
    vec3   trans;
    versor rot;
    vec3   scale;
} Walrus_Transform;

void walrus_transform_compose(Walrus_Transform *transform, mat4 matrix);
void walrus_transform_decompose(Walrus_Transform *transform, mat4 const matrix);

void walrus_trasform_right(Walrus_Transform *transform, vec3 right);
void walrus_trasform_up(Walrus_Transform *transform, vec3 up);
void walrus_trasform_front(Walrus_Transform *transform, vec3 front);

void walrus_transfrom_translate(Walrus_Transform *transform, vec3 trans);
void walrus_transfrom_rotate(Walrus_Transform *transform, versor rot);
void walrus_transfrom_scale(Walrus_Transform *transform, vec3 scale);

void walrus_transform_mul(Walrus_Transform *t1, Walrus_Transform *t2, Walrus_Transform *dst);
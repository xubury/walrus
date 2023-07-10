#pragma once

#include <cglm/types.h>

typedef struct {
    vec3   trans;
    versor rot;
    vec3   scale;
} Walrus_Transform;

void walrus_transform_compose(Walrus_Transform const *transform, mat4 matrix);
void walrus_transform_decompose(Walrus_Transform *transform, mat4 const matrix);

void walrus_transform_right(Walrus_Transform const *transform, vec3 right);
void walrus_transform_up(Walrus_Transform const *transform, vec3 up);
void walrus_transform_front(Walrus_Transform const *transform, vec3 front);

void walrus_transform_translate(Walrus_Transform *transform, vec3 trans);
void walrus_transform_rotate(Walrus_Transform *transform, versor rot);
void walrus_transform_scale(Walrus_Transform *transform, vec3 scale);

void walrus_transform_mul(Walrus_Transform const *t1, Walrus_Transform const *t2, Walrus_Transform *dst);

void walrus_transform_indenity(Walrus_Transform *transform);

void walrus_transform_indenity(Walrus_Transform *transform);

#include <core/transform.h>
#include <cglm/cglm.h>

void walrus_transform_compose(Walrus_Transform *transform, mat4 matrix)
{
    mat4 t, r, s;
    glm_quat_mat4(transform->rot, r);
    glm_translate_to(GLM_MAT4_IDENTITY, transform->trans, t);
    glm_scale_to(GLM_MAT4_IDENTITY, transform->scale, s);

    glm_mat4_mulN((mat4 *[]){&t, &r, &s}, 3, matrix);
}

void walrus_transform_decompose(Walrus_Transform *transform, mat4 const matrix)
{
    mat4 r;
    glm_decompose((vec4 *)matrix, transform->trans, r, transform->scale);
    glm_mat4_quat(r, transform->rot);
}

void walrus_trasform_right(Walrus_Transform *transform, vec3 right)
{
    glm_quat_rotatev(transform->rot, (vec3){1, 0, 0}, right);
}

void walrus_trasform_up(Walrus_Transform *transform, vec3 up)
{
    glm_quat_rotatev(transform->rot, (vec3){0, 1, 0}, up);
}

void walrus_trasform_front(Walrus_Transform *transform, vec3 front)
{
    glm_quat_rotatev(transform->rot, (vec3){0, 0, 1}, front);
}

void walrus_transfrom_translate(Walrus_Transform *transform, vec3 trans)
{
    glm_vec3_add(transform->trans, trans, transform->trans);
}

void walrus_transfrom_rotate(Walrus_Transform *transform, versor rot)
{
    glm_quat_mul(transform->rot, rot, transform->rot);
}

void walrus_transfrom_scale(Walrus_Transform *transform, vec3 scale)
{
    glm_vec3_mul(transform->scale, scale, transform->scale);
}

void walrus_transform_mul(Walrus_Transform *t1, Walrus_Transform *t2, Walrus_Transform *dst)
{
    mat4 m1, m2;
    walrus_transform_compose(t1, m1);
    walrus_transform_compose(t2, m2);
    glm_mat4_mul(m1, m2, m1);
    walrus_transform_decompose(dst, m1);
}

void walrus_transform_indenity(Walrus_Transform *transform)
{
    glm_vec3_zero(transform->trans);
    glm_quat_identity(transform->rot);
    glm_vec3_one(transform->scale);
}

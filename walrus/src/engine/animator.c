#include <engine/animator.h>
#include <core/macro.h>
#include <core/assert.h>

#include <cglm/cglm.h>
#include <string.h>

static void node_traverse(Walrus_Animator *animator, Walrus_ModelNode *root, Walrus_Transform *worlds,
                          Walrus_Transform *locals)
{
    for (u32 i = 0; i < root->num_children; ++i) {
        Walrus_ModelNode *child       = root->children[i];
        u32               root_index  = root - &animator->model->nodes[0];
        u32               child_index = child - &animator->model->nodes[0];
        walrus_transform_mul(&worlds[root_index], &locals[child_index], &worlds[child_index]);
        node_traverse(animator, root->children[i], worlds, locals);
    }
}

static void interpolate_frame_rot(Walrus_AnimationInterpolation mode, versor prev, versor next, f32 factor, versor out)
{
    switch (mode) {
        case WR_ANIMATION_INTERPOLATION_STEP:
            if (factor < 1.0) {
                glm_quat_copy(prev, out);
            }
            else {
                glm_quat_copy(next, out);
            }
            break;
        case WR_ANIMATION_INTERPOLATION_LINEAR: {
#if 0
            if (factor < 1.0) {
                glm_quat_copy(prev, out);
            }
            else {
                glm_quat_copy(next, out);
            }
#else
            versor a, b;
            glm_quat_copy(prev, a);
            glm_quat_copy(next, b);
            f32 dot = glm_quat_dot(a, b);
            if (dot < 0.0f) {
                glm_vec4_scale(b, -1, b);
                dot = -dot;
            }
            if (dot > 0.9995f) {
                glm_quat_lerp(a, b, factor, out);
                glm_quat_normalize(out);
            }
            else {
                f32 theta0     = acos(dot);
                f32 theta      = factor * theta0;
                f32 sin_theta  = sin(theta);
                f32 sin_theat0 = sin(theta0);
                f32 scale_a    = cos(theta) - dot * sin_theta / sin_theat0;
                f32 scale_b    = sin_theta / sin_theat0;
                glm_vec4_scale(a, scale_a, a);
                glm_vec4_scale(b, scale_b, b);
                glm_quat_add(a, b, out);
            }
#endif
        } break;
        case WR_ANIMATION_INTERPOLATION_CUBIC_SPLINE:
            walrus_assert(false);
            break;
    }
}

static void interpolate_frame(Walrus_AnimationInterpolation mode, f32 *prev, f32 *next, f32 factor, f32 *out, u32 cnt)
{
    switch (mode) {
        case WR_ANIMATION_INTERPOLATION_STEP:
            if (factor < 1.0) {
                for (u32 i = 0; i < cnt; ++i) {
                    out[i] = prev[i];
                }
            }
            else {
                for (u32 i = 0; i < cnt; ++i) {
                    out[i] = next[i];
                }
            }
            break;
        case WR_ANIMATION_INTERPOLATION_LINEAR:
            for (u32 i = 0; i < cnt; ++i) {
                out[i] = prev[i] * (1.0 - factor) + next[i] * factor;
            }
            break;
        case WR_ANIMATION_INTERPOLATION_CUBIC_SPLINE:
            walrus_assert(false);
            break;
    }
}

static void animator_apply(Walrus_Animator *animator)
{
    Walrus_Transform *worlds = walrus_array_get(animator->worlds, 0);
    Walrus_Transform *locals = walrus_array_get(animator->locals, 0);

    for (u32 i = 0; i < animator->model->num_nodes; ++i) {
        Walrus_ModelNode *node = &animator->model->nodes[i];
        if (node->parent == NULL) {
            node_traverse(animator, node, worlds, locals);
        }
    }
}

void walrus_animator_init(Walrus_Animator *animator)
{
    animator->keys           = walrus_array_create(sizeof(u32), 0);
    animator->worlds         = walrus_array_create(sizeof(Walrus_Transform), 0);
    animator->locals         = walrus_array_create(sizeof(Walrus_Transform), 0);
    animator->weights        = walrus_array_create(sizeof(f32), 0);
    animator->weight_offsets = walrus_array_create(sizeof(u32), 0);
}

void walrus_animator_shutdown(Walrus_Animator *animator)
{
    walrus_array_destroy(animator->keys);
    walrus_array_destroy(animator->worlds);
    walrus_array_destroy(animator->locals);
    walrus_array_destroy(animator->weights);
    walrus_array_destroy(animator->weight_offsets);
}

void walrus_animator_bind(Walrus_Animator *animator, Walrus_Model const *model)
{
    if (model->num_animations > 0) {
        animator->model     = model;
        animator->animation = &model->animations[0];
        walrus_array_resize(animator->worlds, model->num_nodes);
        walrus_array_resize(animator->locals, model->num_nodes);
        walrus_array_resize(animator->weight_offsets, model->num_nodes);

        u32 *offsets     = walrus_array_get(animator->weight_offsets, 0);
        u32  num_weights = 0;
        for (u32 i = 0; i < model->num_nodes; ++i) {
            offsets[i] = num_weights;
            if (model->nodes[i].mesh) {
                num_weights += model->nodes[i].mesh->num_weights;
            }
        }

        walrus_array_resize(animator->weights, num_weights);

        f32 *weights = walrus_array_get(animator->weights, 0);
        for (u32 i = 0; i < model->num_nodes; ++i) {
            if (model->nodes[i].mesh) {
                memcpy(weights + offsets[i], model->nodes[i].mesh->weights,
                       model->nodes[i].mesh->num_weights * sizeof(f32));
            }
        }
    }
}

static void animator_reset(Walrus_Animator *animator)
{
    animator->timestamp = 0;

    for (u32 i = 0; i < animator->animation->num_channels; ++i) {
        *(u32 *)walrus_array_get(animator->keys, i) = 0;
    }

    Walrus_Transform *locals = walrus_array_get(animator->locals, 0);
    Walrus_Transform *worlds = walrus_array_get(animator->worlds, 0);
    for (u32 i = 0; i < animator->model->num_nodes; ++i) {
        Walrus_ModelNode *node = &animator->model->nodes[i];
        locals[i]              = node->local_transform;
        worlds[i]              = node->world_transform;
    }
    animator_apply(animator);
}

void walrus_animator_play(Walrus_Animator *animator, u32 index)
{
    if (index >= animator->model->num_animations) {
        return;
    }

    animator->animation = &animator->model->animations[index];
    animator->playing   = true;
    animator->repeat    = true;

    walrus_array_resize(animator->keys, animator->animation->num_channels);

    animator_reset(animator);
}

static bool animator_update_animation(Walrus_Animator *animator)
{
    Walrus_Animation const *animation = animator->animation;
    bool                    update    = false;
    u32                    *keys      = walrus_array_get(animator->keys, 0);
    for (u32 i = 0; i < animation->num_channels; ++i) {
        Walrus_AnimationChannel *channel = &animation->channels[i];
        Walrus_AnimationSampler *sampler = channel->sampler;

        u32 const node_index = channel->node - &animator->model->nodes[0];

        Walrus_Transform *t = walrus_array_get(animator->locals, node_index);

        u32  offset  = *(u32 *)walrus_array_get(animator->weight_offsets, node_index);
        f32 *weights = walrus_array_get(animator->weights, offset);

        u32 next_frame = keys[i] + 1;
        for (; next_frame < sampler->num_frames; ++next_frame) {
            f32 timestamp = sampler->timestamps[next_frame];
            if (timestamp > animator->timestamp) {
                break;
            }
        }
        if (next_frame > 0 && next_frame < sampler->num_frames) {
            u32 prev_frame = next_frame - 1;
            if (keys[i] != prev_frame) {
                keys[i] = prev_frame;
            }

            f32  prev_timestamp = sampler->timestamps[prev_frame];
            f32  next_timestamp = sampler->timestamps[next_frame];
            f32 *prev_data      = &sampler->data[prev_frame * sampler->num_components];
            f32 *next_data      = &sampler->data[next_frame * sampler->num_components];

            f32 factor = (animator->timestamp - prev_timestamp) / (next_timestamp - prev_timestamp);
            if (factor > 0) {
                switch (channel->path) {
                    case WR_ANIMATION_PATH_TRANSLATION:
                        interpolate_frame(sampler->interpolation, prev_data, next_data, factor, t->trans,
                                          sampler->num_components);
                        break;
                    case WR_ANIMATION_PATH_ROTATION:
                        interpolate_frame_rot(sampler->interpolation, prev_data, next_data, factor, t->rot);
                        break;
                    case WR_ANIMATION_PATH_SCALE:
                        interpolate_frame(sampler->interpolation, prev_data, next_data, factor, t->scale,
                                          sampler->num_components);
                        break;
                    case WR_ANIMATION_PATH_WEIGHTS:
                        interpolate_frame(sampler->interpolation, prev_data, next_data, factor, weights,
                                          sampler->num_components);
                    default:
                        break;
                }
                update = true;
            }
        }
    }
    return update;
}

void walrus_animator_tick(Walrus_Animator *animator, f32 dt)
{
    if (animator->animation == NULL) {
        return;
    }

    if (animator->playing) {
        Walrus_Animation const *animation = animator->animation;
        if (animator->timestamp > animation->duration) {
            if (animator->repeat) {
                animator_reset(animator);
            }
        }
        else {
            animator->timestamp += dt;
        }

        if (animator_update_animation(animator)) {
            animator_apply(animator);
        }
    }
}

void walrus_animator_transform(Walrus_Animator *animator, Walrus_ModelNode *node, mat4 transform)
{
    u32 index = node - &animator->model->nodes[0];
    walrus_assert(index < animator->model->num_nodes);

    walrus_transform_compose(walrus_array_get(animator->worlds, index), transform);
}

void walrus_animator_weights(Walrus_Animator *animator, Walrus_ModelNode *node, f32 *out_weights)
{
    u32 index = node - &animator->model->nodes[0];
    walrus_assert(index < animator->model->num_nodes);

    if (node->mesh) {
        u32  offset  = *(u32 *)walrus_array_get(animator->weight_offsets, index);
        f32 *weights = walrus_array_get(animator->weights, offset);
        memcpy(out_weights, weights, node->mesh->num_weights);
    }
}

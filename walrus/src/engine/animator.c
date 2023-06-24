#include <engine/animator.h>
#include <core/macro.h>
#include <core/assert.h>

#include <cglm/cglm.h>
#include <string.h>

static void node_traverse(Walrus_Animator *animator, Walrus_Model const *model, Walrus_ModelNode *root,
                          Walrus_Transform *worlds, Walrus_Transform *locals)
{
    for (u32 i = 0; i < root->num_children; ++i) {
        Walrus_ModelNode *child       = root->children[i];
        u32               root_index  = root - &model->nodes[0];
        u32               child_index = child - &model->nodes[0];
        walrus_transform_mul(&worlds[root_index], &locals[child_index], &worlds[child_index]);
        node_traverse(animator, model, root->children[i], worlds, locals);
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
        case WR_ANIMATION_INTERPOLATION_LINEAR:
            glm_quat_slerp(prev, next, factor, out);
            break;
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

static void animator_apply(Walrus_Animator *animator, Walrus_Model const *model)
{
    Walrus_Transform *worlds = walrus_array_get(animator->worlds, 0);
    Walrus_Transform *locals = walrus_array_get(animator->locals, 0);

    for (u32 i = 0; i < model->num_nodes; ++i) {
        Walrus_ModelNode *node = &model->nodes[i];
        if (node->parent == NULL) {
            node_traverse(animator, model, node, worlds, locals);
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

static void animator_reset(Walrus_Animator *animator, Walrus_Model const *model)
{
    animator->timestamp = 0;

    Walrus_Animation const *animation = &model->animations[animator->index];
    for (u32 i = 0; i < animation->num_channels; ++i) {
        *(u32 *)walrus_array_get(animator->keys, i) = 0;
    }

    Walrus_Transform *locals = walrus_array_get(animator->locals, 0);
    Walrus_Transform *worlds = walrus_array_get(animator->worlds, 0);

    f32 *weights = walrus_array_get(animator->weights, 0);
    u32 *offsets = walrus_array_get(animator->weight_offsets, 0);
    for (u32 i = 0; i < model->num_nodes; ++i) {
        Walrus_ModelNode *node = &model->nodes[i];
        locals[i]              = node->local_transform;
        worlds[i]              = node->world_transform;
        if (node->mesh) {
            memcpy(weights + offsets[i], node->mesh->weights, node->mesh->num_weights * sizeof(f32));
        }
    }
    animator_apply(animator, model);
}

void walrus_animator_bind(Walrus_Animator *animator, Walrus_Model const *model)
{
    if (animator->index < model->num_animations) {
        Walrus_Animation const *animation = &model->animations[animator->index];
        walrus_array_resize(animator->keys, animation->num_channels);

        walrus_array_resize(animator->worlds, model->num_nodes);
        walrus_array_resize(animator->locals, model->num_nodes);
        walrus_array_resize(animator->weight_offsets, model->num_nodes);

        u32  num_weights = 0;
        u32 *offsets     = walrus_array_get(animator->weight_offsets, 0);
        for (u32 i = 0; i < model->num_nodes; ++i) {
            offsets[i] = num_weights;
            if (model->nodes[i].mesh) {
                num_weights += model->nodes[i].mesh->num_weights;
            }
        }

        walrus_array_resize(animator->weights, num_weights);

        animator_reset(animator, model);
    }
}

void walrus_animator_play(Walrus_Animator *animator, u32 index)
{
    animator->index   = index;
    animator->playing = true;
    animator->repeat  = true;
}

static bool animator_update_animation(Walrus_Animator *animator, Walrus_Model const *model)
{
    Walrus_Animation const *animation = &model->animations[animator->index];
    bool                    update    = false;
    u32                    *keys      = walrus_array_get(animator->keys, 0);
    for (u32 i = 0; i < animation->num_channels; ++i) {
        Walrus_AnimationChannel *channel = &animation->channels[i];
        Walrus_AnimationSampler *sampler = channel->sampler;

        u32 const node_index = channel->node - &model->nodes[0];

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
                        update = true;
                        break;
                    case WR_ANIMATION_PATH_ROTATION:
                        interpolate_frame_rot(sampler->interpolation, prev_data, next_data, factor, t->rot);
                        update = true;
                        break;
                    case WR_ANIMATION_PATH_SCALE:
                        interpolate_frame(sampler->interpolation, prev_data, next_data, factor, t->scale,
                                          sampler->num_components);
                        update = true;
                        break;
                    case WR_ANIMATION_PATH_WEIGHTS:
                        interpolate_frame(sampler->interpolation, prev_data, next_data, factor, weights,
                                          sampler->num_components);
                        break;
                    default:
                        break;
                }
            }
        }
    }
    return update;
}

void walrus_animator_tick(Walrus_Animator *animator, Walrus_Model const *model, f32 dt)
{
    if (animator->index >= model->num_animations) {
        return;
    }
    Walrus_Animation const *animation = &model->animations[animator->index];

    if (animator->playing) {
        if (animator->timestamp > animation->duration) {
            if (animator->repeat) {
                animator_reset(animator, model);
            }
        }
        else {
            animator->timestamp += dt;
        }

        if (animator_update_animation(animator, model)) {
            animator_apply(animator, model);
        }
    }
}

void walrus_animator_transform(Walrus_Animator const *animator, Walrus_Model const *model, Walrus_ModelNode const *node,
                               mat4 transform)
{
    u32 index = node - &model->nodes[0];
    walrus_assert(index < model->num_nodes);

    walrus_transform_compose(walrus_array_get(animator->worlds, index), transform);
}

void walrus_animator_weights(Walrus_Animator const *animator, Walrus_Model const *model, Walrus_ModelNode const *node,
                             f32 *out_weights)
{
    u32 index = node - &model->nodes[0];
    walrus_assert(index < model->num_nodes);

    u32  offset  = *(u32 *)walrus_array_get(animator->weight_offsets, index);
    f32 *weights = walrus_array_get(animator->weights, offset);
    memcpy(out_weights, weights, node->mesh->num_weights * sizeof(f32));
}

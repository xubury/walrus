#include <engine/animator.h>
#include <core/macro.h>
#include <core/assert.h>

#include <cglm/cglm.h>

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

static void apply_to_transform(Walrus_AnimationPath path, vec4 data, Walrus_Transform *t)
{
    switch (path) {
        case WR_ANIMATION_PATH_TRANSLATION:
            glm_vec3_copy(data, t->trans);
            break;
        case WR_ANIMATION_PATH_ROTATION:
            glm_quat_init(t->rot, data[0], data[1], data[2], data[3]);
            break;
        case WR_ANIMATION_PATH_SCALE:
            glm_vec3_copy(data, t->scale);
            break;
        default:
            break;
    }
}

static void animator_apply(Walrus_Animator *animator)
{
    Walrus_Transform *worlds = walrus_array_get_ptr(animator->worlds, Walrus_Transform, 0);
    Walrus_Transform *locals = walrus_array_get_ptr(animator->locals, Walrus_Transform, 0);

    Walrus_Animation const *animation = animator->animation;
    Walrus_AnimatorKey     *keys      = walrus_array_get_ptr(animator->keys, Walrus_AnimatorKey, 0);

    for (u32 i = 0; i < animation->num_channels; ++i) {
        Walrus_AnimationChannel *channel = &animation->channels[i];

        u32 node_index = channel->node - &animator->model->nodes[0];
        apply_to_transform(channel->path, keys[i].data, &locals[node_index]);
    }

    for (u32 i = 0; i < animator->model->num_nodes; ++i) {
        Walrus_ModelNode *node = &animator->model->nodes[i];
        if (node->parent == NULL) {
            node_traverse(animator, node, worlds, locals);
        }
    }
}

void walrus_animator_init(Walrus_Animator *animator)
{
    animator->keys   = walrus_array_create(sizeof(Walrus_AnimatorKey), 0);
    animator->worlds = walrus_array_create(sizeof(Walrus_Transform), 0);
    animator->locals = walrus_array_create(sizeof(Walrus_Transform), 0);
}

void walrus_animator_shutdown(Walrus_Animator *animator)
{
    walrus_array_destroy(animator->keys);
    walrus_array_destroy(animator->worlds);
    walrus_array_destroy(animator->locals);
}

void walrus_animator_bind(Walrus_Animator *animator, Walrus_Model const *model)
{
    if (model->num_animations > 0) {
        animator->model     = model;
        animator->animation = &model->animations[0];
        walrus_array_resize(animator->worlds, animator->model->num_nodes);
        walrus_array_resize(animator->locals, animator->model->num_nodes);
    }
}

static void animator_reset(Walrus_Animator *animator)
{
    animator->timestamp = 0;

    for (u32 i = 0; i < animator->animation->num_channels; ++i) {
        Walrus_AnimatorKey      *key     = walrus_array_get_ptr(animator->keys, Walrus_AnimatorKey, i);
        Walrus_AnimationChannel *channel = &animator->animation->channels[i];
        Walrus_AnimationSampler *sampler = channel->sampler;

        key->prev = 0;
        glm_vec4_copy(sampler->frames[0].data, key->data);
    }

    Walrus_Transform *locals = walrus_array_get_ptr(animator->locals, Walrus_Transform, 0);
    Walrus_Transform *worlds = walrus_array_get_ptr(animator->worlds, Walrus_Transform, 0);
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
    Walrus_AnimatorKey     *keys      = walrus_array_get_ptr(animator->keys, Walrus_AnimatorKey, 0);
    for (u32 i = 0; i < animation->num_channels; ++i) {
        Walrus_AnimationChannel *channel = &animation->channels[i];
        Walrus_AnimationSampler *sampler = channel->sampler;

        u32 next_frame = keys[i].prev + 1;
        for (; next_frame < sampler->num_frames; ++next_frame) {
            Walrus_AnimationFrame *frame = &sampler->frames[next_frame];
            if (frame->timestamp > animator->timestamp) {
                break;
            }
        }
        if (next_frame > 0) {
            u32 prev_frame = next_frame - 1;
            if (keys[i].prev != prev_frame) {
                keys[i].prev = prev_frame;
                update       = true;

                Walrus_AnimationFrame *frame = &sampler->frames[prev_frame];
                glm_vec4_copy(frame->data, keys[i].data);
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

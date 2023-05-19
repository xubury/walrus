#include <engine/model.h>
#include <core/memory.h>
#include <core/hash.h>
#include <core/log.h>
#include <core/assert.h>
#include <core/macro.h>
#include <core/image.h>
#include <core/string.h>
#include <core/math.h>
#include <rhi/rhi.h>

#include <cglm/cglm.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <string.h>

#define resource_new(type, count) count > 0 ? walrus_new(type, count) : NULL;

static void model_reset(Walrus_Model *model)
{
    model->num_nodes = 0;
    model->nodes     = NULL;

    model->num_roots = 0;
    model->roots     = NULL;

    model->buffers     = NULL;
    model->num_buffers = 0;

    model->textures     = NULL;
    model->num_textures = 0;

    model->meshes     = NULL;
    model->num_meshes = 0;
}

static void model_allocate(Walrus_Model *model, cgltf_data *gltf)
{
    // allocate resource
    model->num_buffers = gltf->buffers_count;
    model->buffers     = resource_new(Walrus_BufferHandle, model->num_buffers);
    for (u32 i = 0; i < model->num_buffers; ++i) {
        model->buffers[i].id = WR_INVALID_HANDLE;
    }

    model->num_meshes = gltf->meshes_count;
    model->meshes     = resource_new(Walrus_Mesh, model->num_meshes);

    for (u32 i = 0; i < model->num_meshes; ++i) {
        cgltf_mesh *mesh                = &gltf->meshes[i];
        model->meshes[i].num_primitives = mesh->primitives_count;
        model->meshes[i].primitives     = resource_new(Walrus_MeshPrimitive, mesh->primitives_count);

        for (u32 j = 0; j < mesh->primitives_count; ++j) {
            model->meshes[i].primitives[j].num_streams = 0;
        }
    }

    model->num_nodes = gltf->nodes_count;
    model->nodes     = resource_new(Walrus_ModelNode, model->num_nodes);
    for (u32 i = 0; i < model->num_nodes; ++i) {
        cgltf_node *node             = &gltf->nodes[i];
        model->nodes[i].num_children = node->children_count;
        model->nodes[i].children     = resource_new(Walrus_ModelNode *, node->children_count);
        model->nodes[i].parent       = NULL;
        model->nodes[i].mesh         = NULL;
    }

    model->num_roots = gltf->scene->nodes_count;
    model->roots     = resource_new(Walrus_ModelNode *, gltf->scene->nodes_count);

    model->num_textures = gltf->textures_count;
    model->textures     = resource_new(Walrus_TextureHandle, model->num_textures);
    for (u32 i = 0; i < model->num_textures; ++i) {
        model->textures[i].id = WR_INVALID_HANDLE;
    }
}

static void model_deallocate(Walrus_Model *model)
{
    for (u32 i = 0; i < model->num_meshes; ++i) {
        Walrus_Mesh *mesh = &model->meshes[i];

        walrus_free(mesh->primitives);
    }

    for (u32 i = 0; i < model->num_nodes; ++i) {
        Walrus_ModelNode *node = &model->nodes[i];

        walrus_free(node->children);
    }

    walrus_free(model->nodes);

    walrus_free(model->roots);

    walrus_free(model->buffers);

    walrus_free(model->meshes);

    walrus_free(model->textures);

    model_reset(model);
}

typedef struct {
    Walrus_VertexLayout     layout;
    Walrus_PrimitiveStream *stream;
} LayoutValue;

static void layout_hash_init(void const *key, void const *value, void *userdata)
{
    walrus_unused(key);
    walrus_unused(userdata);

    LayoutValue *val = (LayoutValue *)value;
    walrus_vertex_layout_end(&val->layout);
    val->stream->layout_handle = walrus_rhi_create_vertex_layout(&val->layout);
}

static void mesh_init(Walrus_Model *model, cgltf_data *gltf)
{
    static Walrus_LayoutComponent components[cgltf_component_type_max_enum] = {
        WR_RHI_COMPONENT_COUNT,  WR_RHI_COMPONENT_INT8,  WR_RHI_COMPONENT_UINT8, WR_RHI_COMPONENT_INT16,
        WR_RHI_COMPONENT_UINT16, WR_RHI_COMPONENT_INT32, WR_RHI_COMPONENT_FLOAT};
    static u32 component_num[cgltf_type_max_enum] = {0, 1, 2, 3, 4, 4, 1, 1};

    for (u32 i = 0; i < gltf->meshes_count; ++i) {
        cgltf_mesh *mesh = &gltf->meshes[i];
        for (u32 j = 0; j < mesh->primitives_count; ++j) {
            cgltf_primitive *prim = &mesh->primitives[j];

            // init indices
            cgltf_accessor *indices = prim->indices;
            if (indices) {
                model->meshes[i].primitives[j].indices.buffer =
                    model->buffers[indices->buffer_view->buffer - &gltf->buffers[0]];
                model->meshes[i].primitives[j].indices.index32 = indices->component_type == cgltf_component_type_r_32u;
                model->meshes[i].primitives[j].indices.offset  = indices->offset + indices->buffer_view->offset;
                model->meshes[i].primitives[j].indices.num_indices = indices->count;
            }
            else {
                model->meshes[i].primitives[j].indices.buffer.id = WR_INVALID_HANDLE;
            }

            Walrus_HashTable *layout_map =
                walrus_hash_table_create_full(walrus_direct_hash, walrus_direct_equal, NULL, walrus_free);

            for (u32 k = 0; k < prim->attributes_count; ++k) {
                cgltf_attribute     *attribute   = &prim->attributes[k];
                cgltf_accessor      *accessor    = attribute->data;
                cgltf_buffer_view   *buffer_view = accessor->buffer_view;
                Walrus_VertexLayout *layout      = NULL;
                void                *layout_key  = buffer_view;
                if (walrus_hash_table_contains(layout_map, layout_key)) {
                    LayoutValue *val = (LayoutValue *)walrus_hash_table_lookup(layout_map, layout_key);
                    walrus_assert_msg(val->stream->num_vertices == accessor->count,
                                      "mismatch number of vertices %d, %d", val->stream->num_vertices, accessor->count);
                    layout = &val->layout;
                }
                else {
                    u32 const    stream_id    = model->meshes[i].primitives[j].num_streams;
                    LayoutValue *val          = walrus_new(LayoutValue, 1);
                    layout                    = &val->layout;
                    val->stream               = &model->meshes[i].primitives[j].streams[stream_id];
                    val->stream->offset       = buffer_view->offset;
                    val->stream->buffer       = model->buffers[buffer_view->buffer - &gltf->buffers[0]];
                    val->stream->num_vertices = accessor->count;
                    model->meshes[i].primitives[j].num_streams =
                        walrus_min(model->meshes[i].primitives[j].num_streams + 1,
                                   walrus_array_len(model->meshes[i].primitives[j].streams));
                    walrus_hash_table_insert(layout_map, layout_key, val);
                    walrus_vertex_layout_begin(layout);
                }
                if (accessor->type == cgltf_type_mat4) {
                    walrus_vertex_layout_add_override(layout, attribute->type - 1, 1, WR_RHI_COMPONENT_MAT4,
                                                      accessor->normalized, accessor->offset, accessor->stride);
                }
                else if (accessor->type == cgltf_type_mat3) {
                    walrus_vertex_layout_add_override(layout, attribute->type - 1, 1, WR_RHI_COMPONENT_MAT3,
                                                      accessor->normalized, accessor->offset, accessor->stride);
                }
                else {
                    walrus_vertex_layout_add_override(layout, attribute->type - 1, component_num[accessor->type],
                                                      components[accessor->component_type], accessor->normalized,
                                                      accessor->offset, accessor->stride);
                }
            }

            // call end for each layout
            walrus_hash_table_foreach(layout_map, layout_hash_init, NULL);
            walrus_hash_table_destroy(layout_map);
        }
    }
}

static void mesh_shutdown(Walrus_Model *model)
{
    for (u32 i = 0; i < model->num_meshes; ++i) {
        Walrus_Mesh *mesh = &model->meshes[i];
        for (u32 j = 0; j < mesh->num_primitives; ++j) {
            for (u32 k = 0; k < mesh->primitives[j].num_streams; ++k) {
                walrus_rhi_destroy_vertex_layout(mesh->primitives[j].streams[k].layout_handle);
            }
        }
    }
}

static void node_traverse(Walrus_ModelNode *root)
{
    for (u32 i = 0; i < root->num_children; ++i) {
        Walrus_ModelNode *child = root->children[i];
        walrus_transform_mul(&root->world_transform, &child->local_transform, &child->world_transform);
        node_traverse(root->children[i]);
    }
}

static void node_init(Walrus_Model *model, cgltf_data *gltf)
{
    for (u32 i = 0; i < model->num_nodes; ++i) {
        cgltf_node       *node  = &gltf->nodes[i];
        Walrus_Transform *local = &model->nodes[i].local_transform;
        Walrus_Transform *world = &model->nodes[i].world_transform;

        if (node->has_matrix) {
            mat4 m;
            for (u8 c = 0; c < 4; ++c) {
                for (u8 r = 0; r < 4; ++r) {
                    m[c][r] = node->matrix[c * 4 + r];
                }
            }
            walrus_transform_decompose(local, m);
        }
        else {
            glm_vec3_copy(node->translation, local->trans);
            glm_vec3_copy(node->scale, local->scale);
            glm_quat_init(local->rot, node->rotation[0], node->rotation[1], node->rotation[2], node->rotation[3]);
        }

        *world = *local;

        if (node->parent) {
            model->nodes[i].parent = &model->nodes[node->parent - &gltf->nodes[0]];
        }
        for (u32 j = 0; j < node->children_count; ++j) {
            model->nodes[i].children[j] = &model->nodes[node->children[j] - &gltf->nodes[0]];
        }

        if (node->mesh) {
            model->nodes[i].mesh = &model->meshes[node->mesh - &gltf->meshes[0]];
        }
    }

    for (u32 i = 0; i < model->num_nodes; ++i) {
        Walrus_ModelNode *node = &model->nodes[i];
        if (node->parent == NULL) {
            node_traverse(node);
        }
    }

    for (u32 i = 0; i < gltf->scene->nodes_count; ++i) {
        model->roots[i] = &model->nodes[gltf->scene->nodes[i] - &gltf->nodes[0]];
    }
}

static Walrus_ModelResult images_load_from_file(Walrus_Image *images, cgltf_data *gltf, char const *filename)
{
    Walrus_ModelResult res         = WR_MODEL_SUCCESS;
    u32 const          num_images  = gltf->images_count;
    char              *parent_path = walrus_str_substr(filename, 0, strrchr(filename, '/') - filename);

    for (u32 i = 0; i < num_images; ++i) {
        cgltf_image *image = &gltf->images[i];
        char         path[255];
        snprintf(path, 255, "%s/%s", parent_path, image->uri);
        walrus_trace("loading image: %s", path);

        if (walrus_image_load_from_file_full(&images[i], path, 4) != WR_IMAGE_SUCCESS) {
            res = WR_MODEL_IMAGE_ERROR;
        }
    }

    walrus_str_free(parent_path);

    return res;
}

static void images_shutdown(Walrus_Image *images, u32 num_images)
{
    for (u32 i = 0; i < num_images; ++i) {
        walrus_image_shutdown(&images[i]);
    }
}

static void textures_init(Walrus_Model *model, Walrus_Image *images, cgltf_data *gltf)
{
    for (u32 i = 0; i < model->num_textures; ++i) {
        cgltf_texture *texture   = &gltf->textures[i];
        u32            img_index = texture->image - gltf->textures[0].image;
        Walrus_Image  *img       = &images[img_index];
        model->textures[i]       = walrus_rhi_create_texture2d(img->width, img->height, WR_RHI_FORMAT_RGBA8, 0,
                                                               WR_RHI_SAMPLER_LINEAR, img->data);
    }
}

static void textures_shutdown(Walrus_Model *model)
{
    for (u32 i = 0; i < model->num_textures; ++i) {
        walrus_rhi_destroy_texture(model->textures[i]);
    }
}

static void buffers_init(Walrus_Model *model, cgltf_data *gltf)
{
    for (u32 i = 0; i < model->num_buffers; ++i) {
        model->buffers[i] = walrus_rhi_create_buffer(gltf->buffers[i].data, gltf->buffers[i].size, 0);
    }
}

static void buffers_shutdown(Walrus_Model *model)
{
    for (u32 i = 0; i < model->num_buffers; ++i) {
        walrus_rhi_destroy_buffer(model->buffers[i]);
    }
}

static void model_init(Walrus_Model *model, Walrus_Image *images, cgltf_data *gltf)
{
    model_allocate(model, gltf);

    buffers_init(model, gltf);

    textures_init(model, images, gltf);

    mesh_init(model, gltf);

    node_init(model, gltf);
}

void walrus_model_shutdown(Walrus_Model *model)
{
    buffers_shutdown(model);

    textures_shutdown(model);

    mesh_shutdown(model);

    model_deallocate(model);
}

Walrus_ModelResult walrus_model_load_from_file(Walrus_Model *model, char const *filename)
{
    model_reset(model);

    cgltf_options opt    = {0};
    cgltf_data   *gltf   = NULL;
    cgltf_result  result = cgltf_parse_file(&opt, filename, &gltf);
    if (result != cgltf_result_success) {
        return WR_MODEL_GLTF_ERROR;
    }

    result = cgltf_load_buffers(&opt, gltf, filename);
    if (result != cgltf_result_success) {
        return WR_MODEL_BUFFER_ERROR;
    }

    u32 const     num_images = gltf->images_count;
    Walrus_Image *images     = walrus_alloca(sizeof(Walrus_Image) * num_images);
    if (images_load_from_file(images, gltf, filename) != WR_MODEL_SUCCESS) {
        return WR_MODEL_IMAGE_ERROR;
    }

    model_init(model, images, gltf);

    images_shutdown(images, num_images);

    cgltf_free(gltf);

    return WR_MODEL_SUCCESS;
}

static void model_node_submit(u16 view_id, Walrus_ModelNode *node, Walrus_ProgramHandle shader, u32 depth,
                              ModelSubmitCallback cb, void *userdata)
{
    Walrus_Mesh *mesh = node->mesh;
    if (mesh) {
        for (u32 i = 0; i < mesh->num_primitives; ++i) {
            if (cb) {
                cb(node, userdata);
            }
            Walrus_MeshPrimitive *prim = &mesh->primitives[i];
            if (prim->indices.index32) {
                walrus_rhi_set_index32_buffer(prim->indices.buffer, prim->indices.offset, prim->indices.num_indices);
            }
            else {
                walrus_rhi_set_index_buffer(prim->indices.buffer, prim->indices.offset, prim->indices.num_indices);
            }
            for (u32 k = 0; k < prim->num_streams; ++k) {
                Walrus_PrimitiveStream *stream = &prim->streams[k];
                walrus_rhi_set_vertex_buffer(k, stream->buffer, stream->layout_handle, stream->offset,
                                             stream->num_vertices);
            }
            walrus_rhi_submit(view_id, shader, depth, WR_RHI_DISCARD_ALL);
        }
    }

    for (u32 i = 0; i < node->num_children; ++i) {
        model_node_submit(view_id, node->children[i], shader, depth, cb, userdata);
    }
}

void walrus_model_submit(u16 view_id, Walrus_Model *model, Walrus_ProgramHandle shader, u32 depth,
                         ModelSubmitCallback cb, void *userdata)
{
    for (u32 i = 0; i < model->num_roots; ++i) {
        model_node_submit(view_id, model->roots[i], shader, depth, cb, userdata);
    }
}

#include <engine/model.h>
#include <core/memory.h>
#include <core/hash.h>
#include <core/log.h>
#include <core/assert.h>
#include <core/macro.h>
#include <core/image.h>
#include <core/string.h>
#include <core/math.h>
#include <core/sort.h>
#include <rhi/rhi.h>

#include <cglm/cglm.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <string.h>

#define resource_new(type, count) count > 0 ? walrus_new(type, count) : NULL;

#define GLTF_WRAP_REPEAT            10497
#define GLTF_WRAP_MIRROR            33648
#define GLTF_WRAP_CLAMP             33071
#define GLTF_FILTER_NONE            0
#define GLTF_FILTER_NEAREST         9728
#define GLTF_FILTER_LINEAR          9729
#define GLTF_FILTER_NEAREST_NEAREST 9984
#define GLTF_FILTER_LINEAR_NEAREST  9985
#define GLTF_FILTER_NEAREST_LINEAR  9986
#define GLTF_FILTER_LINEAR_LINEAR   9987

static u64 convert_wrap_s(cgltf_int wrap)
{
    switch (wrap) {
        case GLTF_WRAP_REPEAT:
            return 0;
        case GLTF_WRAP_MIRROR:
            return WR_RHI_SAMPLER_U_MIRROR;
        case GLTF_WRAP_CLAMP:
            return WR_RHI_SAMPLER_U_CLAMP;
    }
    return 0;
}

static uint64_t convert_warp_t(cgltf_int wrap)
{
    switch (wrap) {
        case GLTF_WRAP_REPEAT:
            return 0;
        case GLTF_WRAP_MIRROR:
            return WR_RHI_SAMPLER_V_MIRROR;
        case GLTF_WRAP_CLAMP:
            return WR_RHI_SAMPLER_V_CLAMP;
    }
    return 0;
}

static uint64_t convert_min_filter(cgltf_int min)
{
    switch (min) {
        case GLTF_FILTER_NEAREST:
            return WR_RHI_SAMPLER_MIN_NEAREST;
        case GLTF_FILTER_LINEAR:
            return WR_RHI_SAMPLER_MIN_LINEAR;
        case GLTF_FILTER_NEAREST_NEAREST:
            return WR_RHI_SAMPLER_MIN_NEAREST | WR_RHI_SAMPLER_MIP_NEAREST;
        case GLTF_FILTER_LINEAR_NEAREST:
            return WR_RHI_SAMPLER_MIN_LINEAR | WR_RHI_SAMPLER_MIP_NEAREST;
        case GLTF_FILTER_NEAREST_LINEAR:
            return WR_RHI_SAMPLER_MIN_NEAREST | WR_RHI_SAMPLER_MIP_LINEAR;
        case GLTF_FILTER_LINEAR_LINEAR:
            return WR_RHI_SAMPLER_MIN_LINEAR | WR_RHI_SAMPLER_MIP_LINEAR;
        default:
            return WR_RHI_SAMPLER_MIN_LINEAR | WR_RHI_SAMPLER_MIP_LINEAR;
    }
}
static uint64_t convert_mag_filter(cgltf_int mag)
{
    switch (mag) {
        case GLTF_FILTER_NEAREST:
            return WR_RHI_SAMPLER_MAG_NEAREST;
        case GLTF_FILTER_LINEAR:
            return WR_RHI_SAMPLER_MAG_LINEAR;
        default:
            return WR_RHI_SAMPLER_MAG_LINEAR;
    }
}

static void model_reset(Walrus_Model *model)
{
    model->num_nodes = 0;
    model->nodes     = NULL;

    model->num_roots = 0;
    model->roots     = NULL;

    model->buffers     = NULL;
    model->num_buffers = 0;

    model->materials     = NULL;
    model->num_materials = 0;

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
            model->meshes[i].primitives[j].num_streams         = 0;
            model->meshes[i].primitives[j].indices.buffer.id   = WR_INVALID_HANDLE;
            model->meshes[i].primitives[j].indices.offset      = 0;
            model->meshes[i].primitives[j].indices.num_indices = 0;
            model->meshes[i].primitives[j].indices.index32     = false;

            model->meshes[i].primitives[j].material = NULL;
        }
    }

    model->num_materials = gltf->materials_count;
    model->materials     = resource_new(Walrus_MeshMaterial, model->num_materials);
    for (u32 i = 0; i < model->num_materials; ++i) {
        Walrus_MeshMaterial *material = &model->materials[i];
        material->albedo              = NULL;
        glm_vec4_one(material->albedo_factor);
        material->metallic_roughness = NULL;
        material->normal             = NULL;
        material->normal_scale       = 1.0;
        material->emissive           = NULL;
        glm_vec3_zero(material->emissive_factor);
        material->double_sided = true;
    }

    model->num_nodes = gltf->nodes_count;
    model->nodes     = resource_new(Walrus_ModelNode, model->num_nodes);
    for (u32 i = 0; i < model->num_nodes; ++i) {
        cgltf_node *node             = &gltf->nodes[i];
        model->nodes[i].num_children = node->children_count;
        model->nodes[i].children     = resource_new(Walrus_ModelNode *, node->children_count);
        for (u32 j = 0; j < node->children_count; ++j) {
            model->nodes[i].children[j] = NULL;
        }
        model->nodes[i].parent = NULL;
        model->nodes[i].mesh   = NULL;
    }

    model->num_roots = gltf->scene->nodes_count;
    model->roots     = resource_new(Walrus_ModelNode *, gltf->scene->nodes_count);

    model->num_textures = gltf->textures_count;
    model->textures     = resource_new(Walrus_Texture, model->num_textures);
    for (u32 i = 0; i < model->num_textures; ++i) {
        model->textures[i].handle.id = WR_INVALID_HANDLE;
        model->textures[i].srgb      = true;
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

    walrus_free(model->materials);

    walrus_free(model->textures);

    model_reset(model);
}

static i32 attr_comp(void const *lhs, void const *rhs)
{
    cgltf_attribute const *lattr = lhs;
    cgltf_attribute const *rattr = rhs;
    return lattr->type - rattr->type;
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

            cgltf_accessor *indices = prim->indices;
            if (indices) {
                model->meshes[i].primitives[j].indices.buffer =
                    model->buffers[indices->buffer_view->buffer - &gltf->buffers[0]];
                model->meshes[i].primitives[j].indices.index32 = indices->component_type == cgltf_component_type_r_32u;
                model->meshes[i].primitives[j].indices.offset  = indices->offset + indices->buffer_view->offset;
                model->meshes[i].primitives[j].indices.num_indices = indices->count;
            }

            if (prim->material) {
                model->meshes[i].primitives[j].material = &model->materials[prim->material - &gltf->materials[0]];
            }

            walrus_quick_sort(prim->attributes, prim->attributes_count, sizeof(cgltf_attribute), attr_comp);

            model->meshes[i].primitives[j].num_streams = 0;
            u32 loc                                    = 0;
            for (u32 k = 0; k < prim->attributes_count; ++k) {
                cgltf_attribute *attribute = &prim->attributes[k];
                if (attribute->index > 0) {
                    continue;
                }
                cgltf_accessor         *accessor    = attribute->data;
                cgltf_buffer_view      *buffer_view = accessor->buffer_view;
                Walrus_VertexLayout     layout;
                u32                     id     = model->meshes[i].primitives[j].num_streams;
                Walrus_PrimitiveStream *stream = &model->meshes[i].primitives[j].streams[id];
                stream->offset                 = buffer_view->offset + accessor->offset;
                stream->buffer                 = model->buffers[buffer_view->buffer - &gltf->buffers[0]];
                stream->num_vertices           = accessor->count;
                walrus_vertex_layout_begin(&layout);
                if (accessor->type == cgltf_type_mat4) {
                    walrus_vertex_layout_add_override(&layout, loc, 1, WR_RHI_COMPONENT_MAT4, accessor->normalized, 0,
                                                      buffer_view->stride);
                }
                else if (accessor->type == cgltf_type_mat3) {
                    walrus_vertex_layout_add_override(&layout, loc, 1, WR_RHI_COMPONENT_MAT3, accessor->normalized, 0,
                                                      buffer_view->stride);
                }
                else {
                    walrus_vertex_layout_add_override(&layout, loc, component_num[accessor->type],
                                                      components[accessor->component_type], accessor->normalized, 0,
                                                      buffer_view->stride);
                }
                walrus_vertex_layout_end(&layout);
                stream->layout_handle = walrus_rhi_create_vertex_layout(&layout);
                ++loc;
                ++model->meshes[i].primitives[j].num_streams;
            }
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

static void materials_init(Walrus_Model *model, cgltf_data *gltf)
{
    for (u32 i = 0; i < model->num_materials; ++i) {
        cgltf_material *material = &gltf->materials[i];
        if (material->normal_texture.texture) {
            model->materials[i].normal       = &model->textures[material->normal_texture.texture - &gltf->textures[0]];
            model->materials[i].normal_scale = material->normal_texture.scale;
            model->materials[i].normal->srgb = false;
        }

        if (material->emissive_texture.texture) {
            model->materials[i].emissive = &model->textures[material->emissive_texture.texture - &gltf->textures[0]];
            model->materials[i].emissive->srgb = true;
        }
        glm_vec3_copy(material->emissive_factor, model->materials[i].emissive_factor);

        if (material->has_pbr_metallic_roughness) {
            cgltf_pbr_metallic_roughness *metallic_roughness = &material->pbr_metallic_roughness;
            if (metallic_roughness->base_color_texture.texture) {
                model->materials[i].albedo =
                    &model->textures[metallic_roughness->base_color_texture.texture - &gltf->textures[0]];
                model->materials[i].albedo->srgb = true;
            }
            if (metallic_roughness->metallic_roughness_texture.texture) {
                model->materials[i].metallic_roughness =
                    &model->textures[metallic_roughness->metallic_roughness_texture.texture - &gltf->textures[0]];
                model->materials[i].metallic_roughness->srgb = false;
            }
        }
        if (material->has_pbr_specular_glossiness) {
            cgltf_pbr_specular_glossiness *specular_glossiness = &material->pbr_specular_glossiness;
            if (specular_glossiness->diffuse_texture.texture) {
                model->materials[i].albedo =
                    &model->textures[specular_glossiness->diffuse_texture.texture - &gltf->textures[0]];
                model->materials[i].albedo->srgb = true;
            }
        }
    }
}

static void textures_init(Walrus_Model *model, Walrus_Image *images, cgltf_data *gltf)
{
    for (u32 i = 0; i < model->num_textures; ++i) {
        cgltf_texture *texture = &gltf->textures[i];
        Walrus_Image  *img     = &images[texture->image - &gltf->images[0]];
        u64            flags   = WR_RHI_SAMPLER_LINEAR;
        if (texture->sampler) {
            flags = convert_min_filter(texture->sampler->min_filter) |
                    convert_mag_filter(texture->sampler->mag_filter) | convert_wrap_s(texture->sampler->wrap_s) |
                    convert_warp_t(texture->sampler->wrap_t);
        }
        if (model->textures[i].srgb) flags |= WR_RHI_TEXTURE_SRGB;
        model->textures[i].handle =
            walrus_rhi_create_texture2d(img->width, img->height, WR_RHI_FORMAT_RGBA8, 0, flags, img->data);
    }
}

static void textures_shutdown(Walrus_Model *model)
{
    for (u32 i = 0; i < model->num_textures; ++i) {
        walrus_rhi_destroy_texture(model->textures[i].handle);
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

    materials_init(model, gltf);

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
            Walrus_MeshPrimitive *prim = &mesh->primitives[i];

            if (cb) {
                cb(node, prim, userdata);
            }

            if (prim->indices.buffer.id != WR_INVALID_HANDLE) {
                if (prim->indices.index32) {
                    walrus_rhi_set_index32_buffer(prim->indices.buffer, prim->indices.offset,
                                                  prim->indices.num_indices);
                }
                else {
                    walrus_rhi_set_index_buffer(prim->indices.buffer, prim->indices.offset, prim->indices.num_indices);
                }
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

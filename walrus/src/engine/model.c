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

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <string.h>

#define resource_new(type, count) count > 0 ? walrus_new(type, count) : NULL;

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
        mesh->num_primitives = 0;
        mesh->primitives     = NULL;
    }

    walrus_free(model->buffers);
    walrus_free(model->meshes);

    walrus_free(model->textures);

    model->buffers     = NULL;
    model->num_buffers = 0;

    model->textures     = NULL;
    model->num_textures = 0;

    model->meshes     = NULL;
    model->num_meshes = 0;
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

static void mesh_init(Walrus_Model *model, cgltf_data *gltf, Walrus_HashTable *buffer_map)
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
                walrus_assert(walrus_hash_table_contains(buffer_map, indices->buffer_view->buffer));
                model->meshes[i].primitives[j].indices.buffer.id =
                    walrus_ptr_to_val(walrus_hash_table_lookup(buffer_map, indices->buffer_view->buffer));
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
                    u32 const    stream_id = model->meshes[i].primitives[j].num_streams;
                    LayoutValue *val       = walrus_new(LayoutValue, 1);
                    layout                 = &val->layout;
                    val->stream            = &model->meshes[i].primitives[j].streams[stream_id];
                    val->stream->offset    = buffer_view->offset;
                    walrus_assert(walrus_hash_table_contains(buffer_map, buffer_view->buffer));
                    val->stream->buffer.id =
                        walrus_ptr_to_val(walrus_hash_table_lookup(buffer_map, buffer_view->buffer));
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

static Walrus_ModelResult textures_init(Walrus_Model *model, cgltf_data *gltf, char const *filename)
{
    Walrus_ModelResult res = WR_MODEL_SUCCESS;

    Walrus_HashTable *image_map  = walrus_hash_table_create(walrus_direct_hash, walrus_direct_equal);
    u32 const         num_images = gltf->images_count;
    Walrus_Image     *images     = walrus_alloca(sizeof(Walrus_Image) * num_images);

    char *parent_path = walrus_str_substr(filename, 0, strrchr(filename, '/') - filename);

    for (u32 i = 0; i < num_images; ++i) {
        cgltf_image *image = &gltf->images[i];
        char         path[255];
        snprintf(path, 255, "%s/%s", parent_path, image->uri);
        walrus_trace("loading image: %s", path);

        if (walrus_image_load_from_file_full(&images[i], path, 4) == WR_IMAGE_SUCCESS) {
            walrus_hash_table_insert(image_map, image, &images[i]);
        }
        else {
            res = WR_MODEL_IMAGE_ERROR;
        }
    }

    walrus_str_free(parent_path);

    for (u32 i = 0; i < model->num_textures; ++i) {
        cgltf_texture *texture = &gltf->textures[i];
        if (walrus_hash_table_contains(image_map, texture->image)) {
            Walrus_Image *image = walrus_hash_table_lookup(image_map, texture->image);
            model->textures[i]  = walrus_rhi_create_texture2d(image->width, image->height, WR_RHI_FORMAT_RGBA8, 0,
                                                              WR_RHI_SAMPLER_LINEAR, image->data);
        }
    }

    for (u32 i = 0; i < num_images; ++i) {
        walrus_image_shutdown(&images[i]);
    }

    walrus_hash_table_destroy(image_map);

    return res;
}

static Walrus_ModelResult model_init(Walrus_Model *model, cgltf_data *gltf, char const *filename)
{
    Walrus_ModelResult res = WR_MODEL_SUCCESS;

    model_allocate(model, gltf);

    // init buffers
    Walrus_HashTable *buffer_map = walrus_hash_table_create(walrus_direct_hash, walrus_direct_equal);
    for (u32 i = 0; i < model->num_buffers; ++i) {
        model->buffers[i] = walrus_rhi_create_buffer(gltf->buffers[i].data, gltf->buffers[i].size, 0);
        walrus_hash_table_insert(buffer_map, &gltf->buffers[i], walrus_val_to_ptr(model->buffers[i].id));
    }

#if 0
    if (res == WR_MODEL_SUCCESS) {
        res = textures_init(model, gltf, filename);
    }
    if (res != WR_MODEL_SUCCESS) {
        walrus_model_shutdown(model);
    }
#else
    walrus_unused(filename);
#endif

    if (res == WR_MODEL_SUCCESS) {
        mesh_init(model, gltf, buffer_map);
    }

    walrus_hash_table_destroy(buffer_map);

    return res;
}

void walrus_model_shutdown(Walrus_Model *model)
{
    for (u32 i = 0; i < model->num_buffers; ++i) {
        walrus_rhi_destroy_buffer(model->buffers[i]);
    }

    for (u32 i = 0; i < model->num_textures; ++i) {
        walrus_rhi_destroy_texture(model->textures[i]);
    }

    mesh_shutdown(model);

    model_deallocate(model);
}

Walrus_ModelResult walrus_model_load_from_file(Walrus_Model *model, char const *filename)
{
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

    Walrus_ModelResult res = model_init(model, gltf, filename);

    cgltf_free(gltf);

    return res;
}

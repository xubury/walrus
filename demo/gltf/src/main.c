#include <engine/engine.h>
#include <core/memory.h>
#include <core/macro.h>
#include <core/log.h>
#include <core/hash.h>
#include <core/math.h>
#include <core/assert.h>
#include <core/string.h>
#include <rhi/rhi.h>

#include <cglm/cglm.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

typedef struct {
    Walrus_BufferHandle buffer;
    Walrus_LayoutHandle layout_handle;
    u32                 offset;
    u32                 num_vertices;
} Walrus_PrimitiveStream;

typedef struct {
    Walrus_BufferHandle buffer;
    u32                 offset;
    u32                 num_indices;
    bool                index32;
} Walrus_MeshIndices;

typedef struct {
    Walrus_PrimitiveStream streams[WR_RHI_MAX_VERTEX_STREAM];
    u32                    num_streams;

    Walrus_MeshIndices indices;
} Walrus_MeshPrimitive;

typedef struct {
    Walrus_MeshPrimitive *primitives;
    u32                   num_primitives;
} Walrus_Mesh;

typedef struct {
    u32   width;
    u32   height;
    void *data;
} Walrus_Image;

typedef struct {
    Walrus_BufferHandle *buffers;
    u32                  num_buffers;

    Walrus_TextureHandle *textures;
    u32                   num_textures;

    Walrus_Mesh *meshes;
    u32          num_meshes;
} Walrus_Model;

typedef struct {
    Walrus_Model         model;
    Walrus_ProgramHandle shader;
    mat4                 world;
} AppData;

char const *vs_src =
    "layout(location = 0) in vec3 a_pos;"
    "layout(location = 1) in vec3 a_normal;"
    "uniform mat4 u_viewproj;"
    "uniform mat4 u_model;"
    "out vec3 v_normal;"
    "void main() {"
    " gl_Position = u_viewproj * u_model * vec4(a_pos, 1);"
    " mat3 nmat = transpose(inverse(mat3(u_model))); "
    " v_normal = nmat * a_normal;"
    "}";

char const *fs_src =
    "out vec4 fragcolor;"
    "in vec3 v_normal;"
    "void main() {"
    " vec3 light_dir = normalize(vec3(0, 1, -1));"
    " float diff = max(dot(normalize(v_normal), light_dir), 0.0);"
    " fragcolor = vec4(diff * vec3(1), 1);"
    "}";
#define resource_new(type, count) count > 0 ? walrus_new(type, count) : NULL;

static void model_allocate(Walrus_Model *model, cgltf_data *gltf)
{
    // allocate resource
    model->num_buffers = gltf->buffers_count;
    model->buffers     = resource_new(Walrus_BufferHandle, model->num_buffers);

    model->num_meshes = gltf->meshes_count;
    model->meshes     = resource_new(Walrus_Mesh, model->num_meshes);

    for (u32 i = 0; i < model->num_meshes; ++i) {
        cgltf_mesh *mesh                = &gltf->meshes[i];
        model->meshes[i].num_primitives = mesh->primitives_count;
        model->meshes[i].primitives     = resource_new(Walrus_MeshPrimitive, mesh->primitives_count);
    }

#if 0 
    model->num_textures = gltf->textures_count;
    model->textures     = resource_new(Walrus_TextureHandle, model->num_textures);
#else
    model->num_textures = 0;
    model->textures     = NULL;
#endif
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
    Walrus_LayoutComponent components[cgltf_component_type_max_enum] = {
        WR_RHI_COMPONENT_COUNT,  WR_RHI_COMPONENT_INT8,  WR_RHI_COMPONENT_UINT8, WR_RHI_COMPONENT_INT16,
        WR_RHI_COMPONENT_UINT16, WR_RHI_COMPONENT_INT32, WR_RHI_COMPONENT_FLOAT};
    u32 component_num[cgltf_type_max_enum] = {0, 1, 2, 3, 4, 4, 1, 1};

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

            model->meshes[i].primitives[j].num_streams = 0;

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
                    u32 const               stream_id = model->meshes[i].primitives[j].num_streams;
                    Walrus_PrimitiveStream *stream    = &model->meshes[i].primitives[j].streams[stream_id];
                    LayoutValue            *val       = walrus_new(LayoutValue, 1);
                    layout                            = &val->layout;
                    val->stream                       = stream;
                    stream->offset                    = buffer_view->offset;
                    walrus_assert(walrus_hash_table_contains(buffer_map, buffer_view->buffer));
                    stream->buffer.id    = walrus_ptr_to_val(walrus_hash_table_lookup(buffer_map, buffer_view->buffer));
                    stream->num_vertices = accessor->count;
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

static void textures_init(Walrus_Model *model, cgltf_data *gltf, char const *filename)
{
    Walrus_HashTable *image_map  = walrus_hash_table_create(walrus_direct_hash, walrus_direct_equal);
    u32 const         num_images = gltf->images_count;
    Walrus_Image     *images     = walrus_alloca(sizeof(Walrus_Image) * num_images);

    char *parent_path = walrus_str_substr(filename, 0, strrchr(filename, '/') - filename);

    for (u32 i = 0; i < num_images; ++i) {
        cgltf_image *image = &gltf->images[i];
        char         path[255];
        snprintf(path, 255, "%s/%s", parent_path, image->uri);
        walrus_trace("loading image: %s", path);

        i32 width, height;
        images[i].data = stbi_load(path, &width, &height, NULL, 4);
        if (images[i].data) {
            images[i].width  = width;
            images[i].height = height;
        }
        else {
            images[i].width  = 0;
            images[i].height = 0;
            walrus_error("fail to load image: %s", stbi_failure_reason());
        }

        walrus_hash_table_insert(image_map, image, &images[i]);
    }

    walrus_str_free(parent_path);

    for (u32 i = 0; i < model->num_textures; ++i) {
        cgltf_texture *texture = &gltf->textures[i];
        walrus_assert(walrus_hash_table_contains(image_map, texture->image));
        Walrus_Image *image = walrus_hash_table_lookup(image_map, texture->image);
        model->textures[i]  = walrus_rhi_create_texture2d(image->width, image->height, WR_RHI_FORMAT_RGBA8, 0,
                                                          WR_RHI_SAMPLER_LINEAR, image->data);
    }

    for (u32 i = 0; i < num_images; ++i) {
        stbi_image_free(images[i].data);
    }

    walrus_hash_table_destroy(image_map);
}

static void model_init(Walrus_Model *model, cgltf_data *gltf, char const *filename)
{
    model_allocate(model, gltf);

    // init buffers
    Walrus_HashTable *buffer_map = walrus_hash_table_create(walrus_direct_hash, walrus_direct_equal);
    for (u32 i = 0; i < model->num_buffers; ++i) {
        model->buffers[i] = walrus_rhi_create_buffer(gltf->buffers[i].data, gltf->buffers[i].size, 0);
        walrus_hash_table_insert(buffer_map, &gltf->buffers[i], walrus_val_to_ptr(model->buffers[i].id));
    }

#if 0
    textures_init(model, gltf, filename);
#endif

    mesh_init(model, gltf, buffer_map);

    walrus_hash_table_destroy(buffer_map);
}

static void model_shutdown(Walrus_Model *model)
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

static void walrus_model_load_from_file(Walrus_Model *model, char const *filename)
{
    cgltf_options opt    = {0};
    cgltf_data   *gltf   = NULL;
    cgltf_result  result = cgltf_parse_file(&opt, filename, &gltf);
    if (result != cgltf_result_success) {
        walrus_error("fail to load gltf");
    }

    result = cgltf_load_buffers(&opt, gltf, filename);
    if (result != cgltf_result_success) {
        walrus_error("fail to load buffers");
    }

    model_init(model, gltf, filename);

    cgltf_free(gltf);
}

Walrus_AppError on_init(Walrus_App *app)
{
    AppData *data = walrus_app_userdata(app);
    glm_mat4_identity(data->world);
    Walrus_ShaderHandle vs = walrus_rhi_create_shader(WR_RHI_SHADER_VERTEX, vs_src);
    Walrus_ShaderHandle fs = walrus_rhi_create_shader(WR_RHI_SHADER_FRAGMENT, fs_src);
    data->shader           = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs, fs}, 2, true);
    walrus_rhi_destroy_shader(vs);
    walrus_rhi_destroy_shader(fs);

    walrus_rhi_set_view_rect(0, 0, 0, 1440, 900);
    walrus_rhi_set_view_clear(0, WR_RHI_CLEAR_COLOR | WR_RHI_CLEAR_DEPTH, 0, 1.0, 0);

    mat4 view;
    mat4 projection;
    glm_lookat((vec3){0, 100, 135}, (vec3){0, 100, 0}, (vec3){0, 1, 0}, view);
    glm_perspective(glm_rad(45), 1440.0 / 900.0, 0.1, 1000.0, projection);
    walrus_rhi_set_view_transform(0, view, projection);

    walrus_model_load_from_file(&data->model, "assets/gltf/shibahu/scene.gltf");

    walrus_rhi_frame();

    return WR_APP_SUCCESS;
}

static void model_submit(Walrus_Model *model, Walrus_ProgramHandle shader, mat4 world)
{
    walrus_rhi_set_transform(world);

    for (u32 i = 0; i < model->num_meshes; ++i) {
        Walrus_Mesh *mesh = &model->meshes[i];
        for (u32 j = 0; j < mesh->num_primitives; ++j) {
            Walrus_MeshPrimitive *prim = &mesh->primitives[j];
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
            if (i == model->num_meshes - 1 && j == mesh->num_primitives - 1) {
                walrus_rhi_submit(0, shader, 0, WR_RHI_DISCARD_ALL);
            }
            else {
                walrus_rhi_submit(0, shader, 0, ~WR_RHI_DISCARD_TRANSFORM);
            }
        }
    }
}

void on_render(Walrus_App *app)
{
    AppData *data = walrus_app_userdata(app);

    model_submit(&data->model, data->shader, data->world);
}

void on_tick(Walrus_App *app, f32 dt)
{
    AppData *data = walrus_app_userdata(app);
    glm_rotate(data->world, glm_rad(20.0) * dt, (vec3){0, 1, 0});
}

void on_shutdown(Walrus_App *app)
{
    AppData *data = walrus_app_userdata(app);
    model_shutdown(&data->model);
}

int main(void)
{
    Walrus_App *app = walrus_app_create(walrus_new(AppData, 1));
    walrus_app_set_init(app, on_init);
    walrus_app_set_render(app, on_render);
    walrus_app_set_tick(app, on_tick);
    walrus_app_set_shutdown(app, on_shutdown);

    walrus_engine_init_run("gltf", 1440, 900, app);

    return 0;
}

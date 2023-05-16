#include <engine/engine.h>
#include <core/memory.h>
#include <core/macro.h>
#include <core/log.h>
#include <core/hash.h>
#include <core/sort.h>
#include <core/assert.h>
#include <rhi/rhi.h>

#include <cglm/cglm.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

typedef struct {
    Walrus_BufferHandle buffer;
    Walrus_VertexLayout layout;
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
    Walrus_PrimitiveStream streams[8];
    u32                    num_streams;

    Walrus_MeshIndices indices;
} Walrus_MeshPrimitive;

typedef struct {
    Walrus_MeshPrimitive *primitives;
    u32                   num_primitives;
} Walrus_Mesh;

typedef struct {
    Walrus_BufferHandle *buffers;
    u32                  num_buffers;

    Walrus_Mesh *meshes;
    u32          num_meshes;
} Walrus_Model;

typedef struct {
    Walrus_Model         model;
    Walrus_ProgramHandle shader;
    cgltf_data          *gltf;
    mat4                 world;
} AppData;

char const *vs_src =
    "layout(location = 0) in vec3 a_pos;"
    "layout(location = 1) in vec3 a_normal;"
    "uniform mat4 u_viewproj;"
    "uniform mat4 u_model;"
    "out vec3 v_normal;"
    "void main() { gl_Position = u_viewproj * u_model * vec4(a_pos, 1); v_normal = a_normal;}";

char const *fs_src =
    "out vec4 fragcolor;"
    "in vec3 v_normal;"
    "void main() { fragcolor = vec4(v_normal, 1);}";

static void layout_hash_init(void const *key, void const *value, void *userdata)
{
    walrus_unused(key);
    walrus_unused(userdata);

    Walrus_PrimitiveStream *stream = (Walrus_PrimitiveStream *)value;
    walrus_vertex_layout_end(&stream->layout);
    stream->layout_handle = walrus_rhi_create_vertex_layout(&stream->layout);
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
    glm_lookat((vec3){0, 0, 35}, (vec3){0, 0, 0}, (vec3){0, 1, 0}, view);
    glm_perspective(glm_rad(45), 1440.0 / 900.0, 0.1, 100.0, projection);
    walrus_rhi_set_view_transform(0, view, projection);

    cgltf_options opt    = {0};
    cgltf_result  result = cgltf_parse_file(&opt, "assets/gltf/EmissiveStrengthTest.gltf", &data->gltf);
    if (result != cgltf_result_success) {
        walrus_error("fail to load gltf");
    }

    result = cgltf_load_buffers(&opt, data->gltf, "assets/gltf/EmissiveStrengthTest.gltf");
    if (result != cgltf_result_success) {
        walrus_error("fail to load buffers");
    }
    Walrus_HashTable *layout_map = walrus_hash_table_create(walrus_direct_hash, walrus_direct_equal);
    Walrus_HashTable *buffer_map = walrus_hash_table_create(walrus_direct_hash, walrus_direct_equal);

    Walrus_Model *model = &data->model;
    cgltf_data   *gltf  = data->gltf;
    model->num_buffers  = gltf->buffers_count;
    model->buffers      = walrus_new(Walrus_BufferHandle, model->num_buffers);
    for (u32 i = 0; i < model->num_buffers; ++i) {
        model->buffers[i] = walrus_rhi_create_buffer(gltf->buffers[i].data, gltf->buffers[i].size, 0);
        walrus_hash_table_insert(buffer_map, &gltf->buffers[i], walrus_val_to_ptr(model->buffers[i].id));
    }

    model->num_meshes = gltf->meshes_count;
    model->meshes     = walrus_new(Walrus_Mesh, model->num_meshes);

    Walrus_LayoutComponent components[cgltf_component_type_max_enum] = {
        WR_RHI_COMPONENT_COUNT,  WR_RHI_COMPONENT_INT8,  WR_RHI_COMPONENT_UINT8, WR_RHI_COMPONENT_INT16,
        WR_RHI_COMPONENT_UINT16, WR_RHI_COMPONENT_INT32, WR_RHI_COMPONENT_FLOAT};
    u32 component_num[cgltf_type_max_enum] = {0, 1, 2, 3, 4, 4, 1, 1};

    for (u32 i = 0; i < gltf->meshes_count; ++i) {
        cgltf_mesh *mesh                = &gltf->meshes[i];
        model->meshes[i].num_primitives = mesh->primitives_count;
        model->meshes[i].primitives     = walrus_new(Walrus_MeshPrimitive, mesh->primitives_count);
        for (u32 j = 0; j < mesh->primitives_count; ++j) {
            cgltf_primitive *prim = &mesh->primitives[j];

            model->meshes[i].primitives[j].num_streams = 0;

            // init indices
            cgltf_accessor *indices = prim->indices;
            if (indices) {
                walrus_assert(walrus_hash_table_contains(buffer_map, indices->buffer_view->buffer));
                model->meshes[i].primitives[j].indices.index32 = indices->component_type == cgltf_component_type_r_32u;
                model->meshes[i].primitives[j].indices.buffer.id =
                    walrus_ptr_to_val(walrus_hash_table_lookup(buffer_map, indices->buffer_view->buffer));
                model->meshes[i].primitives[j].indices.offset      = indices->offset + indices->buffer_view->offset;
                model->meshes[i].primitives[j].indices.num_indices = indices->count;
            }
            else {
                model->meshes[i].primitives[j].indices.buffer.id = WR_INVALID_HANDLE;
            }

            for (u32 k = 0; k < prim->attributes_count; ++k) {
                cgltf_attribute     *attribute   = &prim->attributes[k];
                cgltf_accessor      *accessor    = attribute->data;
                cgltf_buffer_view   *buffer_view = accessor->buffer_view;
                Walrus_VertexLayout *layout      = NULL;
                if (walrus_hash_table_contains(layout_map, buffer_view)) {
                    Walrus_PrimitiveStream *stream =
                        (Walrus_PrimitiveStream *)walrus_hash_table_lookup(layout_map, buffer_view);
                    walrus_assert(stream->num_vertices == accessor->count);
                    layout = &stream->layout;
                }
                else {
                    u32                     id     = model->meshes[i].primitives[j].num_streams;
                    Walrus_PrimitiveStream *stream = &model->meshes[i].primitives[j].streams[id];
                    layout                         = &stream->layout;
                    stream->offset                 = buffer_view->offset;
                    walrus_assert(walrus_hash_table_contains(buffer_map, buffer_view->buffer));
                    stream->buffer.id    = walrus_ptr_to_val(walrus_hash_table_lookup(buffer_map, buffer_view->buffer));
                    stream->num_vertices = accessor->count;
                    ++model->meshes[i].primitives[j].num_streams;
                    walrus_hash_table_insert(layout_map, buffer_view, stream);
                    walrus_vertex_layout_begin(layout);
                }
                if (accessor->type == cgltf_type_mat4) {
                    walrus_vertex_layout_add_override(layout, attribute->type - 1, 1, WR_RHI_COMPONENT_MAT4,
                                                      accessor->normalized, accessor->offset, accessor->stride);
                }
                else if (accessor->type == cgltf_type_mat3) {
                    walrus_assert_msg(false, "unimplemented");
                }
                else {
                    walrus_vertex_layout_add_override(layout, attribute->type - 1, component_num[accessor->type],
                                                      components[accessor->component_type], accessor->normalized,
                                                      accessor->offset, accessor->stride);
                }
            }
        }
    }

    // call end for each layout
    walrus_hash_table_foreach(layout_map, layout_hash_init, NULL);

    walrus_hash_table_destroy(buffer_map);
    walrus_hash_table_destroy(layout_map);

    walrus_rhi_frame();

    return WR_APP_SUCCESS;
}

void on_render(Walrus_App *app)
{
    AppData *data = walrus_app_userdata(app);

    walrus_rhi_set_transform(data->world);
    for (u32 i = 0; i < data->model.num_meshes; ++i) {
        Walrus_Mesh *mesh = &data->model.meshes[i];
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
            walrus_rhi_submit(0, data->shader, 0, ~WR_RHI_DISCARD_TRANSFORM);
        }
    }
}

void on_tick(Walrus_App *app, f32 dt)
{
    AppData *data = walrus_app_userdata(app);
    glm_rotate(data->world, glm_rad(20.0) * dt, (vec3){0, 1, 0});
}

int main(void)
{
    Walrus_App *app = walrus_app_create(walrus_new(AppData, 1));
    walrus_app_set_init(app, on_init);
    walrus_app_set_render(app, on_render);
    walrus_app_set_tick(app, on_tick);

    walrus_engine_init_run("gltf", 1440, 900, app);

    return 0;
}

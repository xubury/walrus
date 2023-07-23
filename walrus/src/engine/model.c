#include <engine/model.h>
#include <engine/systems/model_system.h>
#include <engine/thread_pool.h>
#include <core/memory.h>
#include <core/hash.h>
#include <core/log.h>
#include <core/assert.h>
#include <core/macro.h>
#include <core/image.h>
#include <core/string.h>
#include <core/math.h>
#include <core/sort.h>
#include <core/sys.h>
#include <core/list.h>
#include <core/array.h>
#include <rhi/rhi.h>

#include <cglm/cglm.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <mikktspace.h>

#include <string.h>

typedef struct {
    cgltf_primitive *primitive;
    cgltf_attribute *position;
    cgltf_attribute *normal;
    cgltf_attribute *texcoord;
    u8              *buffer;
} MikData;

static i32 get_num_vertices_of_face(SMikkTSpaceContext const *ctx, i32 const iface)
{
    walrus_unused(iface);
    walrus_unused(ctx);
    // only triangle,  other primitive?
    return 3;
}

static i32 get_num_of_faces(SMikkTSpaceContext const *ctx)
{
    MikData *userdata = (MikData *)ctx->m_pUserData;
    if (userdata->primitive->indices) {
        return userdata->primitive->indices->count / get_num_vertices_of_face(ctx, 0);
    }
    else {
        return userdata->primitive->attributes[0].data->count / get_num_vertices_of_face(ctx, 0);
    }
}

static i32 get_vertex_index(SMikkTSpaceContext const *ctx, i32 iface, i32 ivert)
{
    MikData *userdata = (MikData *)ctx->m_pUserData;
    if (userdata->primitive->indices) {
        i32 index = ivert + iface * get_num_vertices_of_face(ctx, iface);
        i32 ret   = cgltf_accessor_read_index(userdata->primitive->indices, index);
        return ret;
    }
    else {
        i32 index = ivert + iface * get_num_vertices_of_face(ctx, iface);
        return index;
    }
}

static void get_position(SMikkTSpaceContext const *ctx, float *out, const int iface, const int ivert)
{
    MikData *userdata = (MikData *)ctx->m_pUserData;
    i32      index    = get_vertex_index(ctx, iface, ivert);
    if (userdata->position) {
        cgltf_accessor_read_float(userdata->position->data, index, out, sizeof(vec3));
    }
}

static void get_normal(SMikkTSpaceContext const *ctx, float *out, const int iface, const int ivert)
{
    MikData *userdata = (MikData *)ctx->m_pUserData;
    i32      index    = get_vertex_index(ctx, iface, ivert);
    if (userdata->normal) {
        cgltf_accessor_read_float(userdata->normal->data, index, out, sizeof(vec3));
    }
}

static void get_texcoord(SMikkTSpaceContext const *ctx, float *out, const int iface, const int ivert)
{
    MikData *userdata = (MikData *)ctx->m_pUserData;
    i32      index    = get_vertex_index(ctx, iface, ivert);
    if (userdata->texcoord) {
        cgltf_accessor_read_float(userdata->texcoord->data, index, out, sizeof(vec2));
    }
}

static void set_tangent_basic(SMikkTSpaceContext const *ctx, float const *tangents, f32 const fsign, i32 const iface,
                              i32 const ivert)
{
    MikData *userdata = (MikData *)ctx->m_pUserData;
    i32      index    = get_vertex_index(ctx, iface, ivert);
    f32     *out      = (f32 *)&userdata->buffer[index * sizeof(vec4)];
    out[0]            = tangents[0];
    out[1]            = tangents[1];
    out[2]            = tangents[2];
    out[3]            = fsign;
}

static void create_tangents(cgltf_primitive *primitive, void *buffer)
{
    SMikkTSpaceContext   ctx;
    SMikkTSpaceInterface mik_interface;
    MikData              userdata;
    userdata.normal    = NULL;
    userdata.position  = NULL;
    userdata.texcoord  = NULL;
    userdata.primitive = primitive;
    for (u32 i = 0; i < primitive->attributes_count; ++i) {
        cgltf_attribute *attribute = &primitive->attributes[i];
        if (attribute->index > 0) {
            continue;
        }
        switch (attribute->type) {
            default:
                break;
            case cgltf_attribute_type_position:
                userdata.position = attribute;
                break;
            case cgltf_attribute_type_texcoord:
                userdata.texcoord = attribute;
                break;
            case cgltf_attribute_type_normal:
                userdata.normal = attribute;
                break;
        }
    }
    userdata.buffer  = buffer;
    ctx.m_pInterface = &mik_interface;
    ctx.m_pUserData  = &userdata;

    mik_interface.m_getNumFaces          = get_num_of_faces;
    mik_interface.m_getNumVerticesOfFace = get_num_vertices_of_face;
    mik_interface.m_getPosition          = get_position;
    mik_interface.m_getNormal            = get_normal;
    mik_interface.m_getTexCoord          = get_texcoord;
    mik_interface.m_setTSpaceBasic       = set_tangent_basic;
    mik_interface.m_setTSpace            = NULL;
    genTangSpaceDefault(&ctx);
}

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

static uint64_t convert_wrap_t(cgltf_int wrap)
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
static Walrus_LayoutComponent s_components[cgltf_component_type_max_enum] = {
    WR_RHI_COMPONENT_COUNT,  WR_RHI_COMPONENT_INT8,  WR_RHI_COMPONENT_UINT8, WR_RHI_COMPONENT_INT16,
    WR_RHI_COMPONENT_UINT16, WR_RHI_COMPONENT_INT32, WR_RHI_COMPONENT_FLOAT};

static u32 s_component_num[cgltf_type_max_enum] = {0, 1, 2, 3, 4, 4, 1, 1};

static char const *s_property_names[WR_MESH_PROPERTY_COUNT] = {"u_albedo",
                                                               "u_albedo_factor",
                                                               "u_normal",
                                                               "u_normal_scale",
                                                               "u_metallic_roughness",
                                                               "u_metallic_roughness_factor",
                                                               "u_specular_glossiness",
                                                               "u_specular_glossiness_factor",
                                                               "u_emissive",
                                                               "u_emissive_factor",
                                                               "u_occlusion",
                                                               "u_alpha_cutoff"};

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

    model->meshes     = NULL;
    model->num_meshes = 0;

    model->animations     = NULL;
    model->num_animations = 0;

    model->skins     = NULL;
    model->num_skins = 0;
}

void walrus_model_material_init_default(Walrus_Material *material)
{
    material->double_sided = true;
    material->alpha_mode   = WR_ALPHA_MODE_OPAQUE;

    walrus_material_init(material);

    walrus_material_set_float(material, s_property_names[WR_MESH_ALPHA_CUTOFF], 0);

    walrus_material_set_texture_color(material, s_property_names[WR_MESH_ALBEDO], "black");
    walrus_material_set_vec4(material, s_property_names[WR_MESH_ALBEDO_FACTOR], (vec4){0, 0, 0, 0});

    walrus_material_set_texture_color(material, s_property_names[WR_MESH_EMISSIVE], "white");
    walrus_material_set_vec3(material, s_property_names[WR_MESH_EMISSIVE_FACTOR], (vec3){0, 0, 0});

    walrus_material_set_texture_color(material, s_property_names[WR_MESH_NORMAL], "black");
    walrus_material_set_float(material, s_property_names[WR_MESH_NORMAL_SCALE], 1.0);
}

static void model_allocate(Walrus_Model *model, cgltf_data *gltf)
{
    // allocate resource
    model->num_buffers = gltf->buffers_count;
    model->buffers     = resource_new(Walrus_BufferHandle, model->num_buffers);
    for (u32 i = 0; i < model->num_buffers; ++i) {
        model->buffers[i].id = WR_INVALID_HANDLE;
    }
    model->tangent_buffer.id = WR_INVALID_HANDLE;

    model->num_meshes = gltf->meshes_count;
    model->meshes     = resource_new(Walrus_Mesh, model->num_meshes);

    for (u32 i = 0; i < model->num_meshes; ++i) {
        cgltf_mesh *mesh                = &gltf->meshes[i];
        model->meshes[i].num_primitives = mesh->primitives_count;
        model->meshes[i].primitives     = resource_new(Walrus_MeshPrimitive, mesh->primitives_count);

        model->meshes[i].num_weights = mesh->weights_count;
        model->meshes[i].weights     = resource_new(f32, mesh->weights_count);

        glm_vec3_copy((vec3){FLT_MAX, FLT_MAX, FLT_MAX}, model->meshes[i].min);
        glm_vec3_copy((vec3){-FLT_MAX, -FLT_MAX, -FLT_MAX}, model->meshes[i].max);

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
    model->materials     = resource_new(Walrus_Material, model->num_materials);
    for (u32 i = 0; i < model->num_materials; ++i) {
        Walrus_Material *material = &model->materials[i];
        walrus_model_material_init_default(material);
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
        model->nodes[i].skin   = NULL;
        walrus_transform_decompose(&model->nodes[i].local_transform, GLM_MAT4_IDENTITY);
        walrus_transform_decompose(&model->nodes[i].world_transform, GLM_MAT4_IDENTITY);
    }

    model->num_roots = gltf->scene->nodes_count;
    model->roots     = resource_new(Walrus_ModelNode *, gltf->scene->nodes_count);

    model->num_animations = gltf->animations_count;
    model->animations     = resource_new(Walrus_Animation, model->num_animations);
    for (u32 i = 0; i < model->num_animations; ++i) {
        cgltf_animation *animation        = &gltf->animations[i];
        model->animations[i].num_channels = animation->channels_count;
        model->animations[i].channels     = resource_new(Walrus_AnimationChannel, animation->channels_count);

        model->animations[i].num_samplers = animation->samplers_count;
        model->animations[i].samplers     = resource_new(Walrus_AnimationSampler, animation->samplers_count);

        for (u32 j = 0; j < animation->samplers_count; ++j) {
            cgltf_animation_sampler *sampler = &animation->samplers[j];

            model->animations[i].samplers[j].num_frames = sampler->input->count;
            model->animations[i].samplers[j].timestamps = resource_new(f32, sampler->input->count);

            u64 data_size = sampler->output->count * sampler->output->stride;

            model->animations[i].samplers[j].data = walrus_malloc(data_size);
        }
    }

    model->num_skins = gltf->skins_count;
    model->skins     = resource_new(Walrus_ModelSkin, gltf->skins_count);
    for (u32 i = 0; i < gltf->skins_count; ++i) {
        cgltf_skin *skin = &gltf->skins[i];

        model->skins[i].num_joints = skin->joints_count;
        model->skins[i].joints     = resource_new(Walrus_SkinJoint, skin->joints_count);

        for (u32 j = 0; j < skin->joints_count; ++j) {
            glm_vec3_copy((vec3){FLT_MAX, FLT_MAX, FLT_MAX}, model->skins[i].joints[j].min);
            glm_vec3_copy((vec3){-FLT_MAX, -FLT_MAX, -FLT_MAX}, model->skins[i].joints[j].max);
        }
    }
}

static void model_deallocate(Walrus_Model *model)
{
    for (u32 i = 0; i < model->num_animations; ++i) {
        Walrus_Animation *animation = &model->animations[i];
        walrus_free(animation->channels);
        for (u32 j = 0; j < animation->num_samplers; ++j) {
            walrus_free(animation->samplers[j].timestamps);
            walrus_free(animation->samplers[j].data);
        }
        walrus_free(animation->samplers);
    }
    walrus_free(model->animations);

    for (u32 i = 0; i < model->num_meshes; ++i) {
        Walrus_Mesh *mesh = &model->meshes[i];

        walrus_free(mesh->weights);
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

    model_reset(model);
}

typedef struct {
    cgltf_primitive *prim;
    void            *buffer;
    u64              offset;
} TangentTask;

static i32 tangent_create_task(void *userdata)
{
    TangentTask *data = userdata;
    create_tangents(data->prim, (u8 *)data->buffer + data->offset);
    return 0;
}

static Walrus_TextureHandle create_morph_texture(cgltf_primitive *primitive, u32 num_vertices)
{
    Walrus_TextureHandle handle = {WR_INVALID_HANDLE};
    if (primitive->targets_count > 0) {
        u32 const num_attributes = 4;

        f32 *buffer = walrus_malloc0(num_attributes * num_vertices * primitive->targets_count * sizeof(vec3));
        for (u32 i = 0; i < primitive->targets_count; ++i) {
            cgltf_morph_target *target = &primitive->targets[i];
            for (u32 j = 0; j < target->attributes_count; ++j) {
                cgltf_attribute *attribute = &target->attributes[j];
                for (u32 k = 0; k < num_vertices; ++k) {
                    u64 offset = (i + (attribute->type - 1) * primitive->targets_count) * num_vertices + k;
                    cgltf_accessor_read_float(attribute->data, k, buffer + offset * 3, sizeof(vec3));
                }
            }
        }
        Walrus_TextureCreateInfo info;
        info.width       = num_vertices;
        info.height      = primitive->targets_count;
        info.depth       = 1;
        info.ratio       = WR_RHI_RATIO_COUNT;
        info.num_layers  = num_attributes;
        info.num_mipmaps = 1;
        info.format      = WR_RHI_FORMAT_RGB32F;
        info.flags       = 0;
        info.cube_map    = false;
        handle           = walrus_rhi_create_texture(&info, buffer);

        walrus_free(buffer);
    }
    return handle;
}

static void meshes_init(Walrus_Model *model, cgltf_data *gltf)
{
    Walrus_Array *task_list = walrus_array_create(sizeof(TangentTask), 0);

    u64 tangent_buffer_size = 0;
    for (u32 i = 0; i < gltf->meshes_count; ++i) {
        cgltf_mesh *mesh = &gltf->meshes[i];
        memcpy(model->meshes[i].weights, mesh->weights, sizeof(f32) * mesh->weights_count);

        for (u32 j = 0; j < mesh->primitives_count; ++j) {
            bool has_tangent = false;

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

            u32 num_verticies                          = 0;
            model->meshes[i].primitives[j].num_streams = 0;
            for (u32 k = 0; k < prim->attributes_count; ++k) {
                cgltf_attribute *attribute = &prim->attributes[k];
                if (attribute->index > 0) {
                    continue;
                }
                if (attribute->type == cgltf_attribute_type_tangent) {
                    has_tangent = true;
                }
                if (attribute->type == cgltf_attribute_type_position) {
                    glm_vec3_minv(model->meshes[i].min, attribute->data->min, model->meshes[i].min);
                    glm_vec3_maxv(model->meshes[i].max, attribute->data->max, model->meshes[i].max);
                }
                Walrus_VertexLayout     layout;
                cgltf_accessor         *accessor    = attribute->data;
                cgltf_buffer_view      *buffer_view = accessor->buffer_view;
                u32                     loc         = attribute->type - 1;
                u32                     id          = model->meshes[i].primitives[j].num_streams;
                Walrus_PrimitiveStream *stream      = &model->meshes[i].primitives[j].streams[id];
                stream->offset                      = buffer_view->offset + accessor->offset;
                stream->buffer                      = model->buffers[buffer_view->buffer - &gltf->buffers[0]];
                stream->num_vertices                = accessor->count;
                num_verticies                       = accessor->count;
                walrus_vertex_layout_begin(&layout);
                if (accessor->type == cgltf_type_mat4) {
                    walrus_vertex_layout_add_mat4_override(&layout, loc, 0, buffer_view->stride);
                }
                else if (accessor->type == cgltf_type_mat3) {
                    walrus_vertex_layout_add_mat3_override(&layout, loc, 0, buffer_view->stride);
                }
                else {
                    walrus_vertex_layout_add_override(&layout, loc, s_component_num[accessor->type],
                                                      s_components[accessor->component_type], accessor->normalized, 0,
                                                      buffer_view->stride);
                }
                walrus_vertex_layout_end(&layout);
                stream->layout_handle = walrus_rhi_create_vertex_layout(&layout);
                ++model->meshes[i].primitives[j].num_streams;
            }

            if (!has_tangent && model->meshes[i].primitives[j].num_streams > 0) {
                u32 num_vertices = model->meshes[i].primitives[j].streams[0].num_vertices;
                u64 size         = num_vertices * sizeof(vec4);
                u32 stream_id    = model->meshes[i].primitives[j].num_streams;

                TangentTask data;
                data.prim   = prim;
                data.buffer = NULL;
                data.offset = tangent_buffer_size;
                walrus_array_append(task_list, &data);

                Walrus_PrimitiveStream *stream = &model->meshes[i].primitives[j].streams[stream_id];
                stream->offset                 = tangent_buffer_size;
                stream->num_vertices           = num_vertices;
                stream->buffer.id              = WR_INVALID_HANDLE;  // ready for following tangent buffer
                Walrus_VertexLayout layout;
                walrus_vertex_layout_begin(&layout);
                walrus_vertex_layout_add_override(&layout, cgltf_attribute_type_tangent - 1, 4, WR_RHI_COMPONENT_FLOAT,
                                                  false, 0, sizeof(vec4));
                walrus_vertex_layout_end(&layout);
                stream->layout_handle = walrus_rhi_create_vertex_layout(&layout);
                model->meshes[i].primitives[j].num_streams++;
                tangent_buffer_size += size;
            }

            model->meshes[i].primitives[j].morph_target = create_morph_texture(prim, num_verticies);
        }
    }
    // Allocate the buffer, push task to thread pool
    if (tangent_buffer_size > 0) {
        void *tangent_buffer = walrus_malloc(tangent_buffer_size);

        u32                  num_task = walrus_array_len(task_list);
        Walrus_ThreadResult *threads  = walrus_new(Walrus_ThreadResult, num_task);
        for (u32 i = 0; i < num_task; ++i) {
            TangentTask *task = walrus_array_get(task_list, i);
            task->buffer      = tangent_buffer;
            walrus_thread_pool_queue(tangent_create_task, task, &threads[i]);
        }
        // Wait for all the tasks to finish
        for (u32 i = 0; i < num_task; ++i) {
            walrus_thread_pool_result_get(&threads[i], -1);
        }
        walrus_free(threads);

        model->tangent_buffer = walrus_rhi_create_buffer(tangent_buffer, tangent_buffer_size, 0);
        walrus_free(tangent_buffer);
    }

    walrus_array_destroy(task_list);

    // Assgin  buffer handle
    for (u32 i = 0; i < gltf->meshes_count; ++i) {
        cgltf_mesh *mesh = &gltf->meshes[i];
        for (u32 j = 0; j < mesh->primitives_count; ++j) {
            if (model->meshes[i].primitives[j].num_streams == 0) {
                continue;
            }
            Walrus_PrimitiveStream *stream =
                &model->meshes[i].primitives[j].streams[model->meshes[i].primitives[j].num_streams - 1];
            if (stream->buffer.id == WR_INVALID_HANDLE) {
                stream->buffer = model->tangent_buffer;
            }
        }
    }
}

static void mesh_shutdown(Walrus_Model *model)
{
    for (u32 i = 0; i < model->num_meshes; ++i) {
        Walrus_Mesh *mesh = &model->meshes[i];
        for (u32 j = 0; j < mesh->num_primitives; ++j) {
            if (mesh->primitives[j].morph_target.id != WR_INVALID_HANDLE) {
                walrus_rhi_destroy_texture(mesh->primitives[j].morph_target);
            }
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

static void nodes_init(Walrus_Model *model, cgltf_data *gltf)
{
    for (u32 i = 0; i < model->num_nodes; ++i) {
        cgltf_node *node = &gltf->nodes[i];

        if (node->skin) {
            model->nodes[i].skin = &model->skins[node->skin - &gltf->skins[0]];
        }

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

static Walrus_AnimationPath translate_animation_path(cgltf_animation_path_type type)
{
    switch (type) {
        case cgltf_animation_path_type_translation:
            return WR_ANIMATION_PATH_TRANSLATION;
        case cgltf_animation_path_type_rotation:
            return WR_ANIMATION_PATH_ROTATION;
        case cgltf_animation_path_type_scale:
            return WR_ANIMATION_PATH_SCALE;
        case cgltf_animation_path_type_weights:
            return WR_ANIMATION_PATH_WEIGHTS;
        default:
            break;
    }
    return WR_ANIMATION_PATH_COUNT;
}

static Walrus_AnimationInterpolation translate_interpolation(cgltf_interpolation_type type)
{
    switch (type) {
        case cgltf_interpolation_type_linear:
            return WR_ANIMATION_INTERPOLATION_LINEAR;
        case cgltf_interpolation_type_step:
            return WR_ANIMATION_INTERPOLATION_STEP;
        case cgltf_interpolation_type_cubic_spline:
            return WR_ANIMATION_INTERPOLATION_CUBIC_SPLINE;
        default:
            break;
    }
    return WR_ANIMATION_INTERPOLATION_LINEAR;
}

static void animations_init(Walrus_Model *model, cgltf_data *gltf)
{
    for (u32 i = 0; i < gltf->animations_count; ++i) {
        cgltf_animation *animation = &gltf->animations[i];

        f32 duration = 0;
        for (u32 j = 0; j < animation->channels_count; ++j) {
            cgltf_animation_channel *channel = &animation->channels[j];

            model->animations[i].channels[j].path = translate_animation_path(channel->target_path);
            model->animations[i].channels[j].node = &model->nodes[channel->target_node - &gltf->nodes[0]];
            model->animations[i].channels[j].sampler =
                &model->animations[i].samplers[channel->sampler - &animation->samplers[0]];
        }
        for (u32 j = 0; j < animation->samplers_count; ++j) {
            cgltf_animation_sampler *sampler = &animation->samplers[j];

            model->animations[i].samplers[j].interpolation = translate_interpolation(sampler->interpolation);

            u8 const num_components =
                sampler->output->stride * sampler->output->count / sampler->input->count / sizeof(f32);
            model->animations[i].samplers[j].num_components = num_components;
            for (u32 k = 0; k < sampler->input->count; ++k) {
                cgltf_accessor_read_float(sampler->input, k, &model->animations[i].samplers[j].timestamps[k],
                                          sizeof(f32));
                duration = walrus_max(model->animations[i].samplers[j].timestamps[k], duration);
            }
            for (u32 k = 0; k < sampler->output->count; ++k) {
                cgltf_accessor_read_float(
                    sampler->output, k,
                    &model->animations[i].samplers[j].data[k * sampler->output->stride / sizeof(f32)],
                    sampler->output->stride);
            }
        }
        model->animations[i].duration = duration;
    }
}

static void animations_shutdown(Walrus_Model *model)
{
    walrus_unused(model);
}

static void skins_init(Walrus_Model *model, cgltf_data *gltf)
{
    for (u32 i = 0; i < model->num_skins; ++i) {
        cgltf_skin *skin         = &gltf->skins[i];
        model->skins[i].skeleton = &model->nodes[skin->skeleton - &gltf->nodes[0]];
        for (u32 j = 0; j < skin->joints_count; ++j) {
            float m[4][4];
            cgltf_accessor_read_float(skin->inverse_bind_matrices, j, &m[0][0], sizeof(m));
            model->skins[i].joints[j].node = &model->nodes[skin->joints[j] - &gltf->nodes[0]];
            for (u32 c = 0; c < 4; ++c) {
                for (u32 r = 0; r < 4; ++r) {
                    model->skins[i].joints[j].inverse_bind_matrix[c][r] = m[c][r];
                }
            }
        }
    }
}

typedef struct {
    Walrus_Image *image;
    char         *path;
} ImageTask;

static i32 image_load_task(void *userdata)
{
    ImageTask *data = userdata;
    walrus_trace("loading image: %s", data->path);
    if (walrus_image_load_from_file_full(data->image, data->path, 4) != WR_MODEL_SUCCESS) {
        return WR_MODEL_IMAGE_ERROR;
    }
    return WR_MODEL_SUCCESS;
}

static Walrus_ModelResult images_load_from_file(Walrus_Image *images, cgltf_data *gltf, char const *filename)
{
    Walrus_ModelResult res         = WR_MODEL_SUCCESS;
    u32 const          num_images  = gltf->images_count;
    char              *parent_path = walrus_str_substr(filename, 0, walrus_str_last_of(filename, '/'));

    ImageTask           *tasks    = walrus_new(ImageTask, num_images);
    Walrus_ThreadResult *task_res = walrus_new(Walrus_ThreadResult, num_images);
    for (u32 i = 0; i < num_images; ++i) {
        cgltf_image *image = &gltf->images[i];
        char         path[255];
        snprintf(path, 255, "%s/%s", parent_path, image->uri);
        tasks[i].image = &images[i];
        tasks[i].path  = walrus_str_dup(path);
        walrus_thread_pool_queue(image_load_task, &tasks[i], &task_res[i]);
    }

    for (u32 i = 0; i < num_images; ++i) {
        if (walrus_thread_pool_result_get(&task_res[i], -1) != WR_MODEL_SUCCESS) {
            walrus_error("fail to load image from %s", tasks[i].path);
        }
        walrus_str_free(tasks[i].path);
    }
    walrus_free(tasks);
    walrus_free(task_res);

    walrus_str_free(parent_path);

    return res;
}

static void images_shutdown(Walrus_Image *images, u32 num_images)
{
    for (u32 i = 0; i < num_images; ++i) {
        walrus_image_shutdown(&images[i]);
    }
}

static u64 set_texture(Walrus_Material *material, cgltf_texture *texture, Walrus_Image *image, char const *name,
                       bool srgb)
{
    u64 flags = WR_RHI_SAMPLER_LINEAR;
    if (texture->sampler) {
        flags = convert_min_filter(texture->sampler->min_filter) | convert_mag_filter(texture->sampler->mag_filter) |
                convert_wrap_s(texture->sampler->wrap_s) | convert_wrap_t(texture->sampler->wrap_t);
    }
    if (srgb) flags |= WR_RHI_TEXTURE_SRGB;

    Walrus_TextureHandle handle =
        walrus_rhi_create_texture2d(image->width, image->height, WR_RHI_FORMAT_RGBA8, 0, flags, image->data);
    walrus_rhi_frame();
    walrus_material_set_texture(material, name, handle, srgb);
    return flags;
}

static void materials_init(Walrus_Model *model, Walrus_Image *images, cgltf_data *gltf)
{
    static Walrus_AlphaMode mode[cgltf_alpha_mode_max_enum] = {WR_ALPHA_MODE_OPAQUE, WR_ALPHA_MODE_MASK,
                                                               WR_ALPHA_MODE_BLEND};
    for (u32 i = 0; i < model->num_materials; ++i) {
        cgltf_material *material         = &gltf->materials[i];
        model->materials[i].double_sided = material->double_sided;
        model->materials[i].alpha_mode   = mode[material->alpha_mode];

        if (material->alpha_mode == cgltf_alpha_mode_mask) {
            walrus_material_set_float(&model->materials[i], s_property_names[WR_MESH_ALPHA_CUTOFF],
                                      material->alpha_cutoff);
        }
        if (material->normal_texture.texture) {
            set_texture(&model->materials[i], material->normal_texture.texture,
                        &images[material->normal_texture.texture->image - &gltf->images[0]],
                        s_property_names[WR_MESH_NORMAL], false);
            walrus_material_set_float(&model->materials[i], s_property_names[WR_MESH_NORMAL_SCALE],
                                      material->normal_texture.scale);
        }

        if (material->emissive_texture.texture) {
            set_texture(&model->materials[i], material->emissive_texture.texture,
                        &images[material->emissive_texture.texture->image - &gltf->images[0]],
                        s_property_names[WR_MESH_EMISSIVE], true);
        }
        walrus_material_set_vec3(&model->materials[i], s_property_names[WR_MESH_EMISSIVE_FACTOR],
                                 material->emissive_factor);

        /* if (material->occlusion_texture.texture) { */
        /*     model->materials[i].occlusion = &model->textures[material->occlusion_texture.texture -
         * &gltf->textures[0]]; */
        /*     model->materials[i].occlusion->srgb = false; */
        /* } */

        if (material->has_pbr_metallic_roughness) {
            cgltf_pbr_metallic_roughness *metallic_roughness = &material->pbr_metallic_roughness;
            if (metallic_roughness->base_color_texture.texture) {
                set_texture(&model->materials[i], metallic_roughness->base_color_texture.texture,
                            &images[metallic_roughness->base_color_texture.texture->image - &gltf->images[0]],
                            s_property_names[WR_MESH_ALBEDO], true);
            }
            walrus_material_set_vec4(&model->materials[i], s_property_names[WR_MESH_ALBEDO_FACTOR],
                                     metallic_roughness->base_color_factor);
            /* model->materials[i].metallic_factor  = metallic_roughness->metallic_factor; */
            /* model->materials[i].roughness_factor = metallic_roughness->roughness_factor; */
            /* if (metallic_roughness->metallic_roughness_texture.texture) { */
            /*     model->materials[i].metallic_roughness = */
            /*         &model->textures[metallic_roughness->metallic_roughness_texture.texture - &gltf->textures[0]]; */
            /*     model->materials[i].metallic_roughness->srgb = false; */
            /* } */
        }
        if (material->has_pbr_specular_glossiness) {
            cgltf_pbr_specular_glossiness *specular_glossiness = &material->pbr_specular_glossiness;
            /* model->materials[i].specular_glossiness = */
            /*     &model->textures[specular_glossiness->specular_glossiness_texture.texture - &gltf->textures[0]]; */
            /* model->materials[i].specular_glossiness->srgb = true; */

            if (specular_glossiness->diffuse_texture.texture) {
                set_texture(&model->materials[i], specular_glossiness->diffuse_texture.texture,
                            &images[specular_glossiness->diffuse_texture.texture->image - &gltf->images[0]],
                            s_property_names[WR_MESH_ALBEDO], true);
            }
            walrus_material_set_vec4(&model->materials[i], s_property_names[WR_MESH_ALBEDO_FACTOR],
                                     specular_glossiness->diffuse_factor);

            /* glm_vec3_copy(specular_glossiness->specular_factor, model->materials[i].specular_factor); */
            /* model->materials[i].glossiness_factor = specular_glossiness->glossiness_factor; */
        }
    }
}

static void buffers_init(Walrus_Model *model, cgltf_data *gltf)
{
    for (u32 i = 0; i < model->num_buffers; ++i) {
        model->buffers[i] = walrus_rhi_create_buffer(gltf->buffers[i].data, gltf->buffers[i].size, 0);
        walrus_rhi_frame();
    }
}

static void buffers_shutdown(Walrus_Model *model)
{
    for (u32 i = 0; i < model->num_buffers; ++i) {
        walrus_rhi_destroy_buffer(model->buffers[i]);
    }
    walrus_rhi_destroy_buffer(model->tangent_buffer);
}

static void calculate_skin_min_max(Walrus_Model *model, cgltf_data *gltf)
{
    for (u32 i = 0; i < model->num_nodes; ++i) {
        cgltf_node *node = &gltf->nodes[i];
        if (node->skin) {
            cgltf_mesh       *mesh = node->mesh;
            Walrus_ModelSkin *skin = &model->skins[node->skin - &gltf->skins[0]];
            for (u32 j = 0; j < mesh->primitives_count; ++j) {
                cgltf_primitive *primitive = &mesh->primitives[j];
                cgltf_accessor  *apos      = NULL;
                cgltf_accessor  *aweights  = NULL;
                cgltf_accessor  *ajoints   = NULL;
                for (u32 k = 0; k < primitive->attributes_count; ++k) {
                    cgltf_attribute *attr = &primitive->attributes[k];
                    if (attr->index > 0) {
                        continue;
                    }
                    if (attr->type == cgltf_attribute_type_position) {
                        apos = attr->data;
                    }
                    else if (attr->type == cgltf_attribute_type_weights) {
                        aweights = attr->data;
                    }
                    else if (attr->type == cgltf_attribute_type_joints) {
                        ajoints = attr->data;
                    }
                }
                for (u32 k = 0; k < primitive->indices->count; ++k) {
                    u32  v_index = cgltf_accessor_read_index(primitive->indices, k);
                    vec4 pos;
                    pos[3] = 1;
                    vec4 weights;
                    vec4 joints;
                    cgltf_accessor_read_float(apos, v_index, pos, sizeof(vec3));
                    cgltf_accessor_read_float(aweights, v_index, weights, sizeof(vec4));
                    cgltf_accessor_read_float(ajoints, v_index, joints, sizeof(vec4));
                    mat4     skin_matrix       = GLM_MAT4_ZERO_INIT;
                    u8 const max_weight_factor = 4;

                    mat4 skin_matrices[max_weight_factor];
                    for (u32 l = 0; l < max_weight_factor; ++l) {
                        mat4 world;
                        walrus_transform_compose(&skin->joints[(u32)joints[l]].node->world_transform, world);
                        glm_mat4_copy(skin->joints[(u32)joints[l]].inverse_bind_matrix, skin_matrices[l]);
                        glm_mat4_mul(world, skin_matrices[l], world);

                        glm_mat4_scale(skin_matrices[l], weights[l]);
                        glm_vec4_add(skin_matrix[0], skin_matrices[l][0], skin_matrix[0]);
                        glm_vec4_add(skin_matrix[1], skin_matrices[l][1], skin_matrix[1]);
                        glm_vec4_add(skin_matrix[2], skin_matrices[l][2], skin_matrix[2]);
                        glm_vec4_add(skin_matrix[3], skin_matrices[l][3], skin_matrix[3]);
                    }
                    vec4 world_pos;
                    glm_mat4_mulv(skin_matrix, pos, world_pos);
                    for (u32 l = 0; l < max_weight_factor; ++l) {
                        if (weights[l] > 0) {
                            mat4 inv_world;
                            walrus_transform_compose(&skin->joints[(u32)joints[l]].node->world_transform, inv_world);
                            glm_inv_tr(inv_world);

                            vec4 local;
                            glm_mat4_mulv(inv_world, world_pos, local);
                            glm_vec4_scale(local, 1.0 / local[3], local);
                            glm_vec3_minv(local, skin->joints[(u32)joints[l]].min, skin->joints[(u32)joints[l]].min);
                            glm_vec3_maxv(local, skin->joints[(u32)joints[l]].max, skin->joints[(u32)joints[l]].max);
                        }
                    }
                }
            }
        }
    }
    for (u32 i = 0; i < model->num_skins; ++i) {
        for (u32 j = 0; j < model->skins[i].num_joints; ++j) {
            if (glm_vec3_eq(model->skins[i].joints[j].min, FLT_MAX)) {
                glm_vec3_zero(model->skins[i].joints[j].min);
            }
            if (glm_vec3_eq(model->skins[i].joints[j].max, -FLT_MAX)) {
                glm_vec3_zero(model->skins[i].joints[j].max);
            }
        }
    }
}

static void model_init(Walrus_Model *model, Walrus_Image *images, cgltf_data *gltf)
{
    model_allocate(model, gltf);

    buffers_init(model, gltf);

    materials_init(model, images, gltf);

    meshes_init(model, gltf);

    nodes_init(model, gltf);

    animations_init(model, gltf);

    skins_init(model, gltf);

    calculate_skin_min_max(model, gltf);
}

void walrus_model_shutdown(Walrus_Model *model)
{
    animations_shutdown(model);

    mesh_shutdown(model);

    buffers_shutdown(model);

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

    walrus_rhi_frame();

    images_shutdown(images, num_images);

    cgltf_free(gltf);

    return WR_MODEL_SUCCESS;
}

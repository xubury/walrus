#include "gl_shader.h"

#include <core/log.h>
#include <core/memory.h>
#include <core/macro.h>
#include <core/math.h>

#include <string.h>

static Walrus_UniformType conver_gl_type(GLenum type)
{
    switch (type) {
        case GL_BOOL:
            return WR_RHI_UNIFORM_BOOL;

        case GL_INT:
            return WR_RHI_UNIFORM_INT;
        case GL_UNSIGNED_INT:
            return WR_RHI_UNIFORM_UINT;

        case GL_FLOAT:
            return WR_RHI_UNIFORM_FLOAT;
        case GL_FLOAT_VEC2:
            return WR_RHI_UNIFORM_VEC2;
        case GL_FLOAT_VEC3:
            return WR_RHI_UNIFORM_VEC3;
        case GL_FLOAT_VEC4:
            return WR_RHI_UNIFORM_VEC4;

        case GL_FLOAT_MAT2:
            break;

        case GL_FLOAT_MAT3:
            return WR_RHI_UNIFORM_MAT3;

        case GL_FLOAT_MAT4:
            return WR_RHI_UNIFORM_MAT4;

        case GL_SAMPLER_2D:
        case GL_SAMPLER_2D_ARRAY:
        case GL_SAMPLER_2D_MULTISAMPLE:

        case GL_INT_SAMPLER_2D:
        case GL_INT_SAMPLER_2D_ARRAY:
        case GL_INT_SAMPLER_2D_MULTISAMPLE:

        case GL_UNSIGNED_INT_SAMPLER_2D:
        case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:

        case GL_SAMPLER_2D_SHADOW:
        case GL_SAMPLER_2D_ARRAY_SHADOW:

        case GL_SAMPLER_3D:
        case GL_INT_SAMPLER_3D:
        case GL_UNSIGNED_INT_SAMPLER_3D:

        case GL_SAMPLER_CUBE:
        case GL_INT_SAMPLER_CUBE:
        case GL_UNSIGNED_INT_SAMPLER_CUBE:

        case GL_IMAGE_1D:
        case GL_INT_IMAGE_1D:
        case GL_UNSIGNED_INT_IMAGE_1D:

        case GL_IMAGE_2D:
        case GL_IMAGE_2D_ARRAY:
        case GL_INT_IMAGE_2D:
        case GL_UNSIGNED_INT_IMAGE_2D:

        case GL_IMAGE_3D:
        case GL_INT_IMAGE_3D:
        case GL_UNSIGNED_INT_IMAGE_3D:

        case GL_IMAGE_CUBE:
        case GL_INT_IMAGE_CUBE:
        case GL_UNSIGNED_INT_IMAGE_CUBE:
            return WR_RHI_UNIFORM_SAMPLER;
    };

    walrus_assert_msg(false, "Unrecognized GL type %d.", type);
    return WR_RHI_UNIFORM_COUNT;
}

static GLenum const s_shader_type[] = {GL_COMPUTE_SHADER, GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER};

void gl_shader_create(Walrus_ShaderType type, Walrus_ShaderHandle handle, char const *source)
{
    char const *sources[] = {get_glsl_header(), source};
    GLenum      shader    = glCreateShader(s_shader_type[type]);
    glShaderSource(shader, 2, sources, NULL);
    glCompileShader(shader);
    GLint succ;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &succ);
    if (!succ) {
        GLint log_size;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_size);

        char *log     = (char *)walrus_malloc(log_size);
        log[log_size] = 0;
        glGetShaderInfoLog(shader, log_size, NULL, log);
        walrus_error("Shader compile error: %s", log);
        walrus_free(log);
    }
    g_ctx->shaders[handle.id] = shader;
}

void gl_shader_destroy(Walrus_ShaderHandle handle)
{
    glDeleteShader(g_ctx->shaders[handle.id]);
    g_ctx->shaders[handle.id] = 0;
}

void gl_program_create(Walrus_ProgramHandle handle, Walrus_ShaderHandle shader0, Walrus_ShaderHandle shader1,
                       Walrus_ShaderHandle shader2)
{
    GLuint     id         = glCreateProgram();
    GlProgram *prog       = &g_ctx->programs[handle.id];
    prog->id              = id;
    prog->buffer          = NULL;
    prog->num_predefineds = 0;

    if (shader0.id != WR_INVALID_HANDLE) {
        glAttachShader(id, g_ctx->shaders[shader0.id]);
    }
    if (shader1.id != WR_INVALID_HANDLE) {
        glAttachShader(id, g_ctx->shaders[shader1.id]);
    }
    if (shader2.id != WR_INVALID_HANDLE) {
        glAttachShader(id, g_ctx->shaders[shader2.id]);
    }
    glLinkProgram(id);

    GLint succ;
    glGetProgramiv(id, GL_LINK_STATUS, &succ);
    if (!succ) {
        GLint log_size = 0;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &log_size);

        char *log     = (char *)walrus_malloc(log_size);
        log[log_size] = 0;
        glGetProgramInfoLog(id, log_size, NULL, log);
        walrus_error("Failed to link shader program: %s", log);
        walrus_free(log);
    }

    i32 count;
    glGetProgramiv(id, GL_ACTIVE_UNIFORMS, &count);
    for (i32 i = 0; i < count; ++i) {
        i32    num;
        GLenum gl_type;
        char   name[128];
        memset(name, 0, sizeof(name));

        glGetActiveUniform(id, i, sizeof(name), NULL, &num, &gl_type, name);

        num = walrus_max(num, 1);

        u32   loc     = glGetUniformLocation(id, name);
        char *bracket = strstr(name, "[");
        if (bracket) {
            bracket = 0;
        }

        switch (gl_type) {
            case GL_SAMPLER_2D:
            case GL_SAMPLER_2D_ARRAY:
            case GL_SAMPLER_2D_MULTISAMPLE:

            case GL_INT_SAMPLER_2D:
            case GL_INT_SAMPLER_2D_ARRAY:
            case GL_INT_SAMPLER_2D_MULTISAMPLE:

            case GL_UNSIGNED_INT_SAMPLER_2D:
            case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
            case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:

            case GL_SAMPLER_2D_SHADOW:
            case GL_SAMPLER_2D_ARRAY_SHADOW:

            case GL_SAMPLER_3D:
            case GL_INT_SAMPLER_3D:
            case GL_UNSIGNED_INT_SAMPLER_3D:

            case GL_SAMPLER_CUBE:
            case GL_INT_SAMPLER_CUBE:
            case GL_UNSIGNED_INT_SAMPLER_CUBE:

            case GL_IMAGE_1D:
            case GL_INT_IMAGE_1D:
            case GL_UNSIGNED_INT_IMAGE_1D:

            case GL_IMAGE_2D:
            case GL_INT_IMAGE_2D:
            case GL_UNSIGNED_INT_IMAGE_2D:

            case GL_IMAGE_3D:
            case GL_INT_IMAGE_3D:
            case GL_UNSIGNED_INT_IMAGE_3D:

            case GL_IMAGE_CUBE:
            case GL_INT_IMAGE_CUBE:
            case GL_UNSIGNED_INT_IMAGE_CUBE:
                break;

            default:
                break;
        }
        Walrus_UniformType type = conver_gl_type(gl_type);
        if (type != WR_RHI_UNIFORM_COUNT && loc != 0xffffffff) {
            u8 predefined_type = get_predefined_type(name);
            if (predefined_type != PREDEFINED_COUNT && prog->num_predefineds < PREDEFINED_COUNT) {
                prog->predefineds[prog->num_predefineds].loc  = loc;
                prog->predefineds[prog->num_predefineds].type = predefined_type;
                ++prog->num_predefineds;
            }
            else if (walrus_hash_table_contains(g_ctx->uniform_registry, name)) {
                if (prog->buffer == NULL) {
                    prog->buffer = uniform_buffer_create(1024);
                }
                Walrus_UniformHandle uni_handle;
                uni_handle.id = walrus_ptr_to_u32(walrus_hash_table_lookup(g_ctx->uniform_registry, name));
                uniform_buffer_write_uniform_handle(prog->buffer, type, loc, uni_handle, num);
            }
            else {
                walrus_trace("uniform \"%s\" not found in registry.", name);
            }
        }
    }
    if (prog->buffer) {
        uniform_buffer_finish(prog->buffer);
    }
}

void gl_program_destroy(Walrus_ProgramHandle handle)
{
    GlProgram *prog = &g_ctx->programs[handle.id];
    glDeleteProgram(prog->id);
    prog->id = 0;

    if (prog->buffer) {
        uniform_buffer_destroy(prog->buffer);
        prog->buffer = NULL;
    }
}

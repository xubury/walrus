#include "gl_shader.h"

#include <core/log.h>
#include <core/memory.h>

static GLenum const s_shader_type[] = {GL_COMPUTE_SHADER, GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER};

void gl_create_shader(Walrus_ShaderType type, Walrus_ShaderHandle handle, char const *source)
{
    GLenum shader = glCreateShader(s_shader_type[type]);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint succ;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &succ);
    if (!succ) {
        GLint logSize;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);

        char *infoLog    = (char *)walrus_malloc(logSize);
        infoLog[logSize] = 0;
        glGetShaderInfoLog(shader, logSize, NULL, infoLog);
        walrus_error("Shader compile error: %s", infoLog);
        walrus_free(infoLog);
    }
    g_ctx->shaders[handle.id] = shader;
}

void gl_destroy_shader(Walrus_ShaderHandle handle)
{
    glDeleteShader(g_ctx->shaders[handle.id]);
    g_ctx->shaders[handle.id] = 0;
}

void gl_create_program(Walrus_ProgramHandle handle, Walrus_ShaderHandle shader0, Walrus_ShaderHandle shader1,
                       Walrus_ShaderHandle shader2)
{
    GLuint prog = glCreateProgram();
    if (shader0.id != WR_INVALID_HANDLE) {
        glAttachShader(prog, g_ctx->shaders[shader0.id]);
    }
    if (shader1.id != WR_INVALID_HANDLE) {
        glAttachShader(prog, g_ctx->shaders[shader1.id]);
    }
    if (shader2.id != WR_INVALID_HANDLE) {
        glAttachShader(prog, g_ctx->shaders[shader2.id]);
    }
    glLinkProgram(prog);

    GLint succ;
    glUseProgram(prog);  // TODO: Delete this?
    glGetProgramiv(prog, GL_LINK_STATUS, &succ);
    if (!succ) {
        GLint logSize = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logSize);

        char *infoLog    = (char *)walrus_malloc(logSize);
        infoLog[logSize] = 0;
        glGetProgramInfoLog(prog, logSize, NULL, infoLog);
        walrus_error("Failed to link shader program: %s", infoLog);
        walrus_free(infoLog);
    }

    g_ctx->programs[handle.id] = prog;
}

void gl_destroy_program(Walrus_ProgramHandle handle)
{
    glDeleteProgram(g_ctx->programs[handle.id]);
    g_ctx->programs[handle.id] = 0;
}

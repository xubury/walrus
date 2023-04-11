#pragma once

#include "gl_context.h"

void gl_shader_create(Walrus_ShaderType type, Walrus_ShaderHandle handle, char const *source);

void gl_shader_destroy(Walrus_ShaderHandle handle);

void gl_program_create(Walrus_ProgramHandle handle, Walrus_ShaderHandle shader0, Walrus_ShaderHandle shader1,
                       Walrus_ShaderHandle shader2);

void gl_program_destroy(Walrus_ProgramHandle handle);

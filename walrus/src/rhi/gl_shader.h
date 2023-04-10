#pragma once

#include "gl_context.h"

void gl_create_shader(Walrus_ShaderType type, Walrus_ShaderHandle handle, char const *source);

void gl_destroy_shader(Walrus_ShaderHandle handle);

void gl_create_program(Walrus_ProgramHandle handle, Walrus_ShaderHandle shader0, Walrus_ShaderHandle shader1,
                       Walrus_ShaderHandle shader2);

void gl_destroy_program(Walrus_ProgramHandle handle);

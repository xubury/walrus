#pragma once

#include "gl_renderer.h"

void gl_shader_create(Walrus_ShaderType type, Walrus_ShaderHandle handle, char const *source);

void gl_shader_destroy(Walrus_ShaderHandle handle);

void gl_program_create(Walrus_ProgramHandle handle, Walrus_ShaderHandle *shaders, u32 num);

void gl_program_destroy(Walrus_ProgramHandle handle);

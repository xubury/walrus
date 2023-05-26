#pragma once

#include <rhi/type.h>

void walrus_shader_library_init(char const *dir);

void walrus_shader_library_shutdown(void);

Walrus_ShaderHandle walrus_shader_library_load(Walrus_ShaderType type, char const *path);

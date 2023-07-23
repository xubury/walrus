#pragma once

#include <rhi/type.h>

void walrus_shader_library_init(char const *dir);

void walrus_shader_library_shutdown(void);

Walrus_ProgramHandle walrus_shader_library_load(char const *path);

void walrus_shader_library_recompile(char const *path);

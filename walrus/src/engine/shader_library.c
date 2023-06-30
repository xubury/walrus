#include <engine/shader_library.h>
#include <rhi/rhi.h>

#include <core/math.h>
#include <core/string.h>
#include <core/memory.h>
#include <core/log.h>
#include <core/hash.h>

#include <ctype.h>
#include <stdio.h>

#define STB_INCLUDE_LINE_GLSL
#define STB_INCLUDE_IMPLEMENTATION
#include <stb_include.h>

typedef struct {
    char             *dir;
    Walrus_HashTable *shader_map;
} ShaderLibary;

typedef struct {
    Walrus_ShaderType   type;
    Walrus_ShaderHandle handle;
} ShaderRef;

static ShaderLibary *s_library;

WR_INLINE void shader_destroy(void *ptr)
{
    ShaderRef *ref = ptr;
    walrus_rhi_destroy_shader(ref->handle);
    walrus_free(ref);
}

void walrus_shader_library_init(char const *dir)
{
    s_library             = walrus_new(ShaderLibary, 1);
    s_library->dir        = walrus_str_dup(dir);
    s_library->shader_map = walrus_hash_table_create_full(walrus_str_hash, walrus_str_equal,
                                                          (Walrus_KeyDestroyFunc)walrus_str_free, shader_destroy);
}

void walrus_shader_library_shutdown(void)
{
    walrus_hash_table_destroy(s_library->shader_map);
    walrus_str_free(s_library->dir);
    walrus_free(s_library);
    s_library = NULL;
}

Walrus_ShaderHandle walrus_shader_library_load(Walrus_ShaderType type, char const *path)
{
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s/%s", s_library->dir, path);
    ShaderRef *ref = NULL;

    for (u32 i = 0; full_path[i] != '\0'; ++i) {
        if (full_path[i] == '\\') {
            full_path[i] = '/';
        }
        else {
            full_path[i] = tolower(full_path[i]);
        }
    }

    if (walrus_hash_table_contains(s_library->shader_map, full_path)) {
        char const *name[WR_RHI_SHADER_COUNT] = {
            "Compute shader",
            "Vertex shader",
            "Geometry shader",
            "Fragment shader",
        };
        ref = walrus_hash_table_lookup(s_library->shader_map, full_path);
        if (ref->type != type) {
            walrus_error("Mimatched type shader is loaded, type \"%s\" should be \"%s\"", name[type], name[ref->type]);
        }
    }
    else {
        FILE *file = fopen(full_path, "rb");
        char  error[256];
        if (file) {
            u64 size = 0;
            if (fseek(file, 0, SEEK_END) == 0) {
                size = ftell(file);
            }
            fseek(file, 0, SEEK_SET);
            char *source = walrus_str_alloc(size);
            fread(source, 1, size, file);
            walrus_str_skip(source, size);
            char *final = stb_include_string(source, NULL, s_library->dir, full_path, error);
            if (final != NULL) {
                ref         = walrus_new(ShaderRef, 1);
                ref->type   = type;
                ref->handle = walrus_rhi_create_shader(type, final);
                walrus_hash_table_insert(s_library->shader_map, walrus_str_dup(full_path), ref);
                free(final);
            }
            else {
                walrus_error("fail to parse shader file \"%s\", error: %s", full_path, error);
            }
            walrus_str_free(source);

            fclose(file);
        }
        else {
            walrus_error("fail to fopen: %s", full_path);
        }
    }

    return ref ? ref->handle : (Walrus_ShaderHandle){WR_INVALID_HANDLE};
}

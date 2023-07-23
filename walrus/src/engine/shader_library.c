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
    char                *path;
    Walrus_ProgramHandle handle;
} Walrus_Shader;

static ShaderLibary *s_library;

WR_INLINE void shader_destroy(void *ptr)
{
    Walrus_Shader *ref = ptr;
    walrus_rhi_destroy_program(ref->handle);
    walrus_str_free(ref->path);
    walrus_free(ref);
}

void walrus_shader_library_init(char const *dir)
{
    s_library             = walrus_new(ShaderLibary, 1);
    s_library->dir        = walrus_str_dup(dir);
    s_library->shader_map = walrus_hash_table_create_full(walrus_str_hash, walrus_str_equal, NULL, shader_destroy);
}

void walrus_shader_library_shutdown(void)
{
    walrus_hash_table_destroy(s_library->shader_map);
    walrus_str_free(s_library->dir);
    walrus_free(s_library);
    s_library = NULL;
}

static u64 find_directive(char const *shader, char const *type)
{
    char f[64] = "#pragma ";
    if (type) {
        snprintf(f, 64, "#pragma %s", type);
    }
    char *pra = strstr(shader, f);
    if (pra == NULL) {
        return WR_STRING_INVALID_POS;
    }

    if (type == NULL || strncmp(pra + strlen("#pragma "), type, strlen(type)) == 0) {
        return pra - shader;
    }

    return WR_STRING_INVALID_POS;
}

static void find_shader(char const *shader, char const *type, i32 *start, u64 *len)
{
    u64 d   = find_directive(shader, type);
    u64 max = walrus_str_len(shader);

    if (d != WR_STRING_INVALID_POS) {
        shader += d;

        u64 begin  = walrus_str_first_of(shader, '\n');
        u64 next_d = find_directive(shader + begin, NULL);
        *start     = begin + d;
        *len       = walrus_min(next_d, max - *start);
    }
}

static void format_path(char *full_path, i32 n, char const *path)
{
    snprintf(full_path, n, "%s/%s", s_library->dir, path);

    for (u32 i = 0; full_path[i] != '\0'; ++i) {
        if (full_path[i] == '\\') {
            full_path[i] = '/';
        }
        else {
            full_path[i] = tolower(full_path[i]);
        }
    }
}

static void shader_compile(Walrus_Shader *ref)
{
    char full_path[256];
    char error[256];
    format_path(full_path, sizeof(full_path), ref->path);
    FILE *file = fopen(full_path, "rb");
    if (file) {
        u64 size = 0;
        if (fseek(file, 0, SEEK_END) == 0) {
            size = ftell(file);
        }
        fseek(file, 0, SEEK_SET);
        char *source = walrus_str_alloc(size);
        fread(source, 1, size, file);
        walrus_str_skip(source, size);
        if (source != NULL) {
            struct {
                i32 start;
                u64 len;
            } v, f;
            u64 header = find_directive(source, NULL);
            find_shader(source, "vertex", &v.start, &v.len);
            find_shader(source, "fragment", &f.start, &f.len);
            char *vertex = walrus_str_alloc(header + v.len);
            char *frag   = walrus_str_alloc(header + f.len);

            walrus_str_skip(vertex, header + v.len);
            walrus_str_skip(frag, header + f.len);

            memcpy(vertex, source, header);
            memcpy(vertex + header, source + v.start, v.len);
            memcpy(frag, source, header);
            memcpy(frag + header, source + f.start, f.len);

            char *vertex_final = stb_include_string(vertex, NULL, s_library->dir, ref->path, error);
            if (vertex_final == NULL) {
                walrus_error("shader compile error: %s", error);
                return;
            }
            char *frag_final = stb_include_string(frag, NULL, s_library->dir, ref->path, error);
            if (frag_final == NULL) {
                walrus_error("shader compile error: %s", error);
                return;
            }

            Walrus_ShaderHandle vs = walrus_rhi_create_shader(WR_RHI_SHADER_VERTEX, vertex_final);
            Walrus_ShaderHandle fs = walrus_rhi_create_shader(WR_RHI_SHADER_FRAGMENT, frag_final);
            ref->handle            = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs, fs}, 2, false);
            walrus_str_free(vertex);
            walrus_str_free(frag);
            free(vertex_final);
            free(frag_final);
            walrus_rhi_destroy_shader(vs);
            walrus_rhi_destroy_shader(fs);
        }
        else {
            walrus_error("fail to parse shader file \"%s\", error: %s", ref->path, error);
        }
        walrus_str_free(source);

        fclose(file);
    }
    else {
        walrus_error("fail to fopen: %s", ref->path);
    }
}

Walrus_ProgramHandle walrus_shader_library_load(char const *path)
{
    char full_path[256];
    format_path(full_path, sizeof(full_path), path);
    Walrus_Shader *shader = NULL;
    if (walrus_hash_table_contains(s_library->shader_map, path)) {
        shader = walrus_hash_table_lookup(s_library->shader_map, path);
    }
    else {
        shader            = walrus_new(Walrus_Shader, 1);
        shader->path      = walrus_str_dup(path);
        shader->handle.id = WR_INVALID_HANDLE;
        walrus_hash_table_insert(s_library->shader_map, shader->path, shader);
        shader_compile(shader);
    }

    return shader ? shader->handle : (Walrus_ProgramHandle){WR_INVALID_HANDLE};
}

void walrus_shader_library_recompile(char const *path)
{
    if (walrus_hash_table_contains(s_library->shader_map, path)) {
        Walrus_Shader *ref = walrus_hash_table_lookup(s_library->shader_map, path);
        walrus_rhi_destroy_program(ref->handle);
        ref->handle.id = WR_INVALID_HANDLE;
        shader_compile(ref);
    }
}

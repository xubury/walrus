#include <core/type.h>
#include <core/sys.h>
#include <core/macro.h>
#include <core/log.h>
#include <core/handle_alloc.h>
#include <core/memory.h>
#include <core/string.h>

#include <engine/engine.h>
#include <rhi/rhi.h>

#include <math.h>
#include <string.h>
#include "core/hash.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cglm/cglm.h>

char const *vs_src =
    "#version 300 es\n"
    "layout (location = 0) in vec3 a_pos;\n"
    "layout (location = 1) in vec2 a_uv;\n"
    "out vec2 v_pos;\n"
    "out vec2 v_uv;\n"
    "uniform mat4 u_viewproj;\n"
    "uniform mat4 u_model;\n"
    "void main() { \n"
    "    gl_Position = u_viewproj * u_model * vec4(a_pos, 1.0);\n"
    "    v_pos = a_pos.xy;\n"
    "    v_uv = a_uv;\n"
    "}\n";

char const *fs_src =
    "#version 300 es\n"
    "precision mediump float;"
    "out vec4 fragColor;"
    "in vec2 v_pos;"
    "in vec2 v_uv;"
    "uniform sampler2D u_texture;"
    "void main() { "
    "    fragColor = texture(u_texture, v_uv) * vec4(v_pos, 1.0, 1.0);"
    "}";

typedef struct {
    Walrus_ProgramHandle shader;
    Walrus_UniformHandle texture_handle;
    GLuint               textures[2];
    GLuint               vbo;

    mat4 viewproj;
    mat4 model;

    // Uniforms
    GLuint u_texture;
} AppData;

void on_render(Walrus_App *app)
{
    AppData *data = walrus_app_userdata(app);

    glBindTexture(GL_TEXTURE_2D, data->textures[0]);
    glActiveTexture(GL_TEXTURE0);

    walrus_rhi_set_transform(data->model);
    walrus_rhi_submit(0, data->shader, WR_RHI_DISCARD_ALL);
    glUniform1i(data->u_texture, 0);
}

void on_tick(Walrus_App *app, float dt)
{
    AppData *data = walrus_app_userdata(app);
    glm_rotate(data->model, 1.0 * dt, (vec3){0, 1, 0});
}

void on_event(Walrus_App *app, Walrus_Event *e)
{
    walrus_unused(app);

    if (e->type == WR_EVENT_TYPE_BUTTON) {
        if (e->button.device == WR_INPUT_KEYBOARD && e->button.button == WR_KEY_ESCAPE) {
            walrus_engine_exit();
        }
    }
}

bool i32_equal(void const *a, void const *b)
{
    return *(i32 *)a == *(i32 *)b;
}

u32 i32_hash(void const *a)
{
    return *(i32 *)a;
}

Walrus_AppError on_init(Walrus_App *app)
{
    // Handle test
    Walrus_HandleAlloc *alloc   = walrus_handle_create(1234);
    Walrus_Handle       handle0 = walrus_handle_alloc(alloc);
    Walrus_Handle       handle1 = walrus_handle_alloc(alloc);
    walrus_handle_free(alloc, handle0);
    walrus_assert(!walrus_handle_valid(alloc, handle0));
    Walrus_Handle handle2 = walrus_handle_alloc(alloc);
    /* walrus_handle_free(alloc, handle1); */
    Walrus_Handle handle3 = walrus_handle_alloc(alloc);
    walrus_trace("handle alloc: %d, %d, %d, %d", handle0, handle1, handle2, handle3);

    Walrus_HashTable *table = walrus_hash_table_create(i32_hash, i32_equal);

    // Hash table test
    i32 i = 123;
    walrus_hash_table_add(table, &i);
    walrus_assert(walrus_hash_table_contains(table, &i));
    walrus_hash_table_remove(table, &i);
    walrus_assert(!walrus_hash_table_contains(table, &i));
    walrus_hash_table_destroy(table);

    {
        // String test
        char *str = walrus_str_alloc(0);
        walrus_str_append(&str, "hello world");
        walrus_trace("str:%s, len:%d", str, walrus_str_len(str));
        walrus_str_append(&str, "yes yes");
        walrus_trace("str:%s, len:%d", str, walrus_str_len(str));
        char *sub = walrus_str_dup("hello world");
        walrus_trace("substr:%s, len:%d", sub, walrus_str_len(sub));
        walrus_str_resize(&sub, 6);
        walrus_str_append(&sub, "1");
        walrus_trace("substr:%s, len:%d", sub, walrus_str_len(sub));
        walrus_str_free(str);
        walrus_str_free(sub);
    }
    {
        // String hash table test
        Walrus_HashTable *table = NULL;
        table = walrus_hash_table_create_full(walrus_str_hash, walrus_str_equal, (Walrus_KeyDestroyFunc)walrus_str_free,
                                              (Walrus_KeyDestroyFunc)walrus_str_free);
        char *key    = walrus_str_dup("Hello");
        char *value  = walrus_str_dup("World");
        char *value2 = walrus_str_dup("World2");
        walrus_hash_table_insert(table, key, value);
        walrus_trace("hash key: %s value: %s", key, walrus_hash_table_lookup(table, key));
        walrus_assert(!walrus_hash_table_insert(table, key, value2));
        walrus_assert(walrus_hash_table_contains(table, "Hello"));
        walrus_trace("hash key: %s value: %s", key, walrus_hash_table_lookup(table, "Hello"));
        walrus_hash_table_destroy(table);
    }

    Walrus_AppError err      = WR_APP_SUCCESS;
    AppData        *app_data = walrus_app_userdata(app);
    Walrus_Window  *window   = walrus_engine_window();
    i32 const       width    = walrus_window_width(window);
    i32 const       height   = walrus_window_height(window);

    walrus_rhi_set_view_rect(0, 0, 0, width, height);
    walrus_rhi_set_view_clear(0, WR_RHI_CLEAR_COLOR | WR_RHI_CLEAR_DEPTH, 0xffffffff, 1.0, 0);

    glm_perspective(glm_rad(45.0), (float)width / height, 0.1, 100, app_data->viewproj);
    mat4 view = GLM_MAT4_IDENTITY_INIT;
    walrus_rhi_set_view_transform(0, view, app_data->viewproj);

    // clang-format off
    f32 vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };
    // clang-format on

    glGenBuffers(1, &app_data->vbo);

    glBindBuffer(GL_ARRAY_BUFFER, app_data->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    app_data->texture_handle = walrus_rhi_create_uniform("u_texture", WR_RHI_UNIFORM_SAMPLER, 1);

    Walrus_ShaderHandle vs = walrus_rhi_create_shader(WR_RHI_SHADER_VERTEX, vs_src);
    Walrus_ShaderHandle fs = walrus_rhi_create_shader(WR_RHI_SHADER_FRAGMENT, fs_src);
    app_data->shader       = walrus_rhi_create_program(vs, fs);

    app_data->u_texture = glGetUniformLocation(3, "u_texture");

    glm_mat4_identity(app_data->model);
    glm_translate(app_data->model, (vec3){0, 0, -2});

    i32 const array_len = walrus_array_len(app_data->textures);
    glGenTextures(array_len, app_data->textures);

    for (i32 i = 0; i < array_len; ++i) {
        walrus_trace("texture: %d", app_data->textures[i]);
    }

    /* stbi img test */
    stbi_set_flip_vertically_on_load(true);
    i32 x, y, c;
    u64 ts  = walrus_sysclock(WR_SYS_CLOCK_UNIT_MILLSEC);
    u8 *img = stbi_load("imgs/test.png", &x, &y, &c, 4);
    walrus_trace("stbi_load time: %llu ms", walrus_sysclock(WR_SYS_CLOCK_UNIT_MILLSEC) - ts);
    if (img != NULL) {
        walrus_trace("load image width: %d height: %d channel: %d", x, y, c);
        glBindTexture(GL_TEXTURE_2D, app_data->textures[0]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(img);
    }
    else {
        err = WR_APP_INIT_FAIL;

        walrus_error("fail to load image: %s", stbi_failure_reason());
    }

    return err;
}

void on_shutdown(Walrus_App *app)
{
    AppData *data = walrus_app_userdata(app);
    walrus_rhi_destroy_uniform(data->texture_handle);

    glDeleteTextures(walrus_array_len(data->textures), data->textures);
    glDeleteBuffers(1, &data->vbo);
}

int main(int argc, char *argv[])
{
    walrus_unused(argc);
    walrus_unused(argv);

    // cglm test
    vec3 ve = {1.0, 0, 0};
    walrus_trace("before rotate: %f, %f, %f", ve[0], ve[1], ve[2]);
    glm_vec3_rotate(ve, glm_rad(45), (vec3){0, 0, 1.0});
    walrus_trace("after rotate: %f, %f, %f", ve[0], ve[1], ve[2]);

    Walrus_EngineOption opt;
    opt.window_title  = "Rotate Cube";
    opt.window_width  = 640;
    opt.window_height = 480;
    opt.window_flags  = WR_WINDOW_FLAG_ASYNC | WR_WINDOW_FLAG_OPENGL;
    opt.minfps        = 30.f;

    Walrus_App *app = walrus_app_create(walrus_malloc(sizeof(AppData)));
    walrus_app_set_init(app, on_init);
    walrus_app_set_shutdown(app, on_shutdown);
    walrus_app_set_tick(app, on_tick);
    walrus_app_set_render(app, on_render);
    walrus_app_set_event(app, on_event);

    walrus_engine_init_run(&opt, app);

    return 0;
}

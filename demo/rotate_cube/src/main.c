#include <core/type.h>
#include <core/sys.h>
#include <core/macro.h>
#include <core/log.h>
#include <core/handle_alloc.h>

#include <engine/engine.h>
#include <rhi/rhi.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

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

static GLuint compile_shader(GLenum type, char const *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint succ;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &succ);
    if (!succ) {
        GLint logSize;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);

        char *infoLog    = (char *)malloc(logSize);
        infoLog[logSize] = 0;
        glGetShaderInfoLog(shader, logSize, NULL, infoLog);
        walrus_error("Shader compile error: %s", infoLog);
        free(infoLog);
    }
    return shader;
}

static GLuint link_program(GLuint vs, GLuint fs)
{
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint succ;
    glGetProgramiv(prog, GL_LINK_STATUS, &succ);
    if (!succ) {
        GLint logSize = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logSize);

        char *infoLog    = (char *)malloc(logSize);
        infoLog[logSize] = 0;
        glGetProgramInfoLog(prog, logSize, NULL, infoLog);
        walrus_error("Failed to link shader program: %s", infoLog);
        free(infoLog);
    }
    return prog;
}

typedef struct {
    GLuint shader;
    GLuint textures[2];
    GLuint vbo;

    mat4 viewproj;
    mat4 model;

    // Uniforms
    GLuint u_texture;
    GLuint u_viewproj;
    GLuint u_model;
} AppData;

void on_render(Walrus_App *app)
{
    AppData *data = walrus_app_userdata(app);

    glUniformMatrix4fv(data->u_model, 1, false, data->model[0]);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(data->shader);
    glBindTexture(GL_TEXTURE_2D, data->textures[0]);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(data->u_texture, 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindTexture(GL_TEXTURE_2D, 0);

    walrus_rhi_submit(0);
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

Walrus_AppError on_init(Walrus_App *app)
{
    Walrus_HandleAlloc *alloc   = walrus_handle_create(1234);
    Walrus_Handle       handle0 = walrus_handle_alloc(alloc);
    Walrus_Handle       handle1 = walrus_handle_alloc(alloc);
    walrus_handle_free(alloc, handle0);
    walrus_assert(!walrus_handle_valid(alloc, handle0));
    Walrus_Handle handle2 = walrus_handle_alloc(alloc);
    /* walrus_handle_free(alloc, handle1); */
    Walrus_Handle handle3 = walrus_handle_alloc(alloc);
    walrus_trace("handle alloc: %d, %d, %d, %d", handle0, handle1, handle2, handle3);

    Walrus_AppError err      = WR_APP_SUCCESS;
    AppData        *app_data = walrus_app_userdata(app);
    Walrus_Window  *window   = walrus_engine_window();
    i32 const       width    = walrus_window_width(window);
    i32 const       height   = walrus_window_height(window);

    walrus_rhi_set_view_rect(0, 0, 0, width, height);
    walrus_rhi_set_view_clear(0, WR_RHI_CLEAR_COLOR | WR_RHI_CLEAR_DEPTH, 0xffffffff, 1.0, 0);

    // clang-format off
    float vertices[] = {
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

    GLuint vs        = compile_shader(GL_VERTEX_SHADER, vs_src);
    GLuint fs        = compile_shader(GL_FRAGMENT_SHADER, fs_src);
    app_data->shader = link_program(vs, fs);

    app_data->u_texture  = glGetUniformLocation(app_data->shader, "u_texture");
    app_data->u_viewproj = glGetUniformLocation(app_data->shader, "u_viewproj");
    app_data->u_model    = glGetUniformLocation(app_data->shader, "u_model");
    walrus_trace("uniform \"u_texture\" loc: %d", app_data->u_texture);
    walrus_trace("uniform \"u_viewproj\" loc: %d", app_data->u_viewproj);
    walrus_trace("uniform \"u_model\" loc: %d", app_data->u_model);

    glUseProgram(app_data->shader);

    glm_perspective(glm_rad(45.0), (float)width / height, 0.1, 100, app_data->viewproj);
    glm_mat4_identity(app_data->model);
    glm_translate(app_data->model, (vec3){0, 0, -2});
    glUniformMatrix4fv(app_data->u_viewproj, 1, false, app_data->viewproj[0]);
    glUniformMatrix4fv(app_data->u_model, 1, false, app_data->model[0]);

    i32 const array_len = walrus_array_len(app_data->textures);
    glGenTextures(array_len, app_data->textures);

    for (int i = 0; i < array_len; ++i) {
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
    glDeleteTextures(walrus_array_len(data->textures), data->textures);
    glDeleteBuffers(1, &data->vbo);
}

int main(int argc, char *argv[])
{
    walrus_unused(argc) walrus_unused(argv);

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

    Walrus_App *app = walrus_app_create(malloc(sizeof(AppData)));
    walrus_app_set_init(app, on_init);
    walrus_app_set_shutdown(app, on_shutdown);
    walrus_app_set_tick(app, on_tick);
    walrus_app_set_render(app, on_render);
    walrus_app_set_event(app, on_event);

    walrus_engine_init_run(&opt, app);

    return 0;
}

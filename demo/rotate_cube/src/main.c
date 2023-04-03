#include <math.h>
#include <type.h>
#include <sys.h>
#include <macro.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <event.h>
#include <engine.h>
#include <rhi.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cglm/cglm.h>

char const *vsSource =
    "#version 300 es\n"
    "const vec2 quadVert[] = vec2[](vec2(-1.0f, 1.0f), vec2(-1.0f, -1.0f), vec2(1.0f, 1.0f), vec2(1.0f, -1.0f));"
    "const vec2 uv[] = vec2[](vec2(0.0f, 1.0f), vec2(0.0f, 0.0f), vec2(1.0f, 1.0f), vec2(1.0f, 0.0f));"
    "out vec2 v_pos;"
    "out vec2 v_uv;"
    "void main() { "
    "    gl_Position = vec4(quadVert[gl_VertexID], 0.0, 1.0);"
    "    v_pos = quadVert[gl_VertexID];"
    "    v_uv = uv[gl_VertexID];"
    "}";

char const *fsSource =
    "#version 300 es\n"
    "precision mediump float;"
    "out vec4 fragColor;"
    "in vec2 v_pos;"
    "in vec2 v_uv;"
    "uniform sampler2D u_texture;"
    "void main() { "
    "    fragColor = texture(u_texture, v_uv) * vec4(v_pos, 1.0, 1.0);"
    "}";

static GLuint compile_shader(GLenum type, const char *source)
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
        printf("Shader compile error: %s\n", infoLog);
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
        printf("Failed to link shader program: %s\n", infoLog);
        free(infoLog);
    }
    return prog;
}

typedef struct {
    GLuint shader;
    GLuint textures[2];
} AppData;

void on_render(App *app)
{
    AppData *data = app_get_userdata(app);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1, 0.2, 0.3, 1);
    glUseProgram(data->shader);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    /* printf("dt:%f\n", wajsGetFrameTime()); */
}

void on_tick(App *app, float dt)
{
    UNUSED(app) UNUSED(dt);
}

void on_event(App *app, Event *e)
{
    UNUSED(app)

    if (e->type == EVENT_TYPE_AXIS) {
        printf("mouse move: %d, %d\n", e->axis.x, e->axis.y);
    }
    else if (e->type == EVENT_TYPE_BUTTON) {
        printf("key: %d\n", e->button.code);
    }
}

void on_init(App *app)
{
    AppData *app_data = malloc(sizeof(AppData));
    Engine  *engine   = engine_get();

    app_set_userdata(app, app_data);

    glViewport(0, 0, engine->opt.window_width, engine->opt.window_height);

    GLuint vs        = compile_shader(GL_VERTEX_SHADER, vsSource);
    GLuint fs        = compile_shader(GL_FRAGMENT_SHADER, fsSource);
    app_data->shader = link_program(vs, fs);

    GLuint textureloc = glGetUniformLocation(app_data->shader, "u_texture");
    printf("uniform \"u_texture\" loc: %d\n", textureloc);

    glUseProgram(app_data->shader);

    i32 const arrayLen = ARRAY_LEN(app_data->textures);

    glGenTextures(arrayLen, app_data->textures);

    for (int i = 0; i < arrayLen; ++i) {
        printf("texture: %d\n", app_data->textures[i]);
    }

    /* stbi img test */
    stbi_set_flip_vertically_on_load(true);
    i32 x, y, c;
    u64 ts  = sysclock(SYS_CLOCK_UNIT_MILLSEC);
    u8 *img = stbi_load("test.png", &x, &y, &c, 4);
    printf("stbi_load time: %llu ms\n", sysclock(SYS_CLOCK_UNIT_MILLSEC) - ts);
    if (img != NULL) {
        printf("load image width: %d height: %d channel: %d\n", x, y, c);
        glBindTexture(GL_TEXTURE_2D, app_data->textures[0]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
        glGenerateMipmap(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(textureloc, 0);

        stbi_image_free(img);
    }
    else {
        printf("fail to load image: %s\n", stbi_failure_reason());
    }
}

void on_destroy(App *app)
{
    UNUSED(app);
}

int main(int argc, char *argv[])
{
    UNUSED(argc) UNUSED(argv);

    // cglm test
    vec3 ve = {1.0, 0, 0};
    printf("before rotate: %f, %f, %f\n", ve[0], ve[1], ve[2]);
    glm_vec3_rotate(ve, glm_rad(45), (vec3){0, 0, 1.0});
    printf("after rotate: %f, %f, %f\n", ve[0], ve[1], ve[2]);

    EngineOption opt;
    opt.window_width  = 640;
    opt.window_height = 480;
    opt.minfps        = 30.f;

    App *app = app_alloc();
    app_set_init(app, on_init);
    app_set_destroy(app, on_destroy);
    app_set_tick(app, on_tick);
    app_set_render(app, on_render);
    app_set_event(app, on_event);

    engine_init(&opt);

    engine_run(app);

    engine_destroy();

    return 0;
}
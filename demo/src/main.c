#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <wajs_gl.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

typedef float f32;
typedef double f64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef size_t usize;
typedef ssize_t isize;

#include <assert.h>
#define ASSERT(e, ...)                \
    if (!(e)) {                       \
        assert(false);                \
        fprintf(stderr, __VA_ARGS__); \
    }

const char *vsSource =
    "#version 300 es\n"
    "const vec2 triVert[] = vec2[](vec2(-0.5f, -0.5f), vec2(0.0f, 0.5f), vec2(0.5f, -0.5f));"
    "const vec2 quadVert[] = vec2[](vec2(-1.0f, 1.0f), vec2(-1.0f, -1.0f), vec2(1.0f, 1.0f), vec2(1.0f, -1.0f));"
    "out vec2 v_pos;"
    "void main() { "
    "    gl_Position = vec4(quadVert[gl_VertexID], 0.0, 1.0);"
    "    v_pos = quadVert[gl_VertexID];"
    "}";

const char *fsSource =
    "#version 300 es\n"
    "precision highp float;"
    "out vec4 FragColor;"
    "in vec2 v_pos;"
    "void main() { "
    "    FragColor = vec4(v_pos, 0, 1);"
    "}";

static GLuint glProg = 0;
const static int canvasWidth = 640;
const static int canvasHeight = 480;

void glRender()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1, 0.2, 0.3, 1);
    glUseProgram(glProg);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    // printf("dt:%f\n", wajsGetFrameTime());
}

GLuint compileShader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint succ;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &succ);
    if (!succ) {
        GLint logSize;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);

        char *infoLog = (char *)malloc(logSize);
        infoLog[logSize] = 0;
        glGetShaderInfoLog(shader, 255, NULL, infoLog);
        printf("Shader compile error: %s\n", infoLog);
        free(infoLog);
    }
    return shader;
}

GLuint linkProgram(GLuint vs, GLuint fs)
{
    GLuint prog;
    prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint succ;
    glGetProgramiv(prog, GL_LINK_STATUS, &succ);
    if (!succ) {
        GLint logSize = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logSize);

        char *infoLog = (char *)malloc(logSize);
        infoLog[logSize] = 0;
        glGetProgramInfoLog(prog, logSize, NULL, infoLog);
        printf("Failed to link shader program: %s\n", infoLog);
        free(infoLog);
    }
    return prog;
}

void glSetup()
{
    wajsSetupGlCanvas(canvasWidth, canvasHeight);
    wajsSetGlRenderCallback(glRender);
    glViewport(0, 0, canvasWidth, canvasHeight);

    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSource);
    glProg = linkProgram(vs, fs);

    GLuint textures[2];
    glGenTextures(2, textures);
    for (int i = 0; i < 2; ++i) {
        printf("texture: %d\n", textures[i]);
    }
    i32 x, y, c;
    u8 *img = stbi_load("test.png", &x, &y, &c, 4);
    if (img != NULL) {
        printf("load image width: %d height: %d channel: %d\n", x, y, c);
        glBindTexture(GL_TEXTURE_2D, textures[0]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
        stbi_image_free(img);
    }
    else {
        printf("fail to load image: %s\n", stbi_failure_reason());
    }

    glDeleteTextures(2, textures);
}

void printUnixTime()
{
    typedef struct timespec timespec;
    timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    time_t s = spec.tv_sec;
    u64 ms = round(spec.tv_nsec / 1.0e6);  // Convert nanoseconds to milliseconds
    if (ms > 999) {
        s++;
        ms = 0;
    }

    printf("Current time: %lld.%lld seconds since the Epoch\n", s, ms);
}

int main(int argc, char *argv[])
{
    printf("argc: %d\n", argc);
    for (i32 i = 0; i < argc; ++i) {
        printf("argv[%d]: %s ", i, argv[i]);
    }
    printf("\n");

    glSetup();

    printUnixTime();
    printf("sinf:%f\n", sinf(1.0));
    u8 *ptr = (u8 *)malloc(100);
    ptr[0] = 122;
    ptr[1] = 123;
    ptr[2] = 124;
    printf("malloc:0x%lx\n", (intptr_t)ptr);
    for (i32 i = 0; i < 3; ++i) {
        printf("ptr[%d]:%d ", i, ptr[i]);
    }
    printf("\n");

    return 0;
}

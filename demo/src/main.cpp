#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <wajsGl.h>

const char *vsSource = R"(#version 300 es
const vec2 triVert[] = vec2[](vec2(-0.5f, -0.5f), vec2(0.0f, 0.5f), vec2(0.5f, -0.5f));
void main() { 
     gl_Position = vec4(triVert[gl_VertexID], 0.0, 1.0);
}

)";

const char *fsSource = R"(#version 300 es
precision highp float;
out vec4 FragColor;
void main() { 
    FragColor = vec4(1, 0, 0, 1);
})";

GLuint prog = 0;

void glRender()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.1, 0.2, 0.3, 1);
    glUseProgram(prog);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // printf("dt:%f\n", wajsGetFrameTime());
}

GLuint compileShader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint succ;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &succ);

    if (!succ) {
        GLint logSize;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);

        char *buffer = (char *)malloc(logSize);
        glGetShaderInfoLog(shader, 255, nullptr, buffer);
        printf("shader compile error: %s\n", buffer);
        free(buffer);
    }
    return shader;
}

void glSetup()
{
    wajsSetupGlCanvas(640, 480);
    wajsSetGlRenderCallback(glRender);

    glViewport(0, 0, 640, 480);

    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSource);
    prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
}

void printUnixTime()
{
    timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    time_t s = spec.tv_sec;
    long ms = round(spec.tv_nsec / 1.0e6);  // Convert nanoseconds to milliseconds
    if (ms > 999) {
        s++;
        ms = 0;
    }

    printf("Current time: %lld.%ld seconds since the Epoch\n", s, ms);
}

int main(int argc, char *argv[])
{
    glSetup();

    printUnixTime();
    printf("sinf:%f\n", sinf(1.0));
    char *ptr = (char *)malloc(100);
    ptr[0] = 122;
    ptr[1] = 123;
    ptr[2] = 124;
    printf("malloc:0x%lx\n", (intptr_t)ptr);
    for (int i = 0; i < 3; ++i) {
        printf("ptr[%d]:%d ", i, ptr[i]);
    }
    printf("\n");

    return 0;
}

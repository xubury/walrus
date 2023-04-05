#include <glfw_window.h>

#include <macro.h>
#include <event.h>

#include <GLFW/glfw3.h>

static void error_callback(int error, const char *description)
{
    printf("GLFW Error ({%d}): {%s}", error, description);
}

static void framebuffer_size_callback(GLFWwindow *window, i32 width, i32 height)
{
    UNUSED(window);
    Event e;
    e.type              = EVENT_TYPE_RESOLUTION;
    e.resolution.width  = width;
    e.resolution.height = height;
    event_push(&e);
}

static void window_close_callback(GLFWwindow *window)
{
    UNUSED(window);
    Event e;
    e.type = EVENT_TYPE_EXIT;
    event_push(&e);
}

void *glfw_create_gl_context(i32 width, i32 height)
{
    ASSERT_MSG(glfwInit(), "Cannot initialize glfw!");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwSetErrorCallback(error_callback);

    void *handle = glfwCreateWindow(width, height, "null", NULL, NULL);
    if (handle != NULL) {
        glfwMakeContextCurrent(handle);

        glfwSwapInterval(1);
        glfwSetWindowCloseCallback(handle, window_close_callback);
        glfwSetFramebufferSizeCallback(handle, framebuffer_size_callback);
    }
    return handle;
}

void glfw_destroy_gl_context(void *handle)
{
    glfwDestroyWindow(handle);
}

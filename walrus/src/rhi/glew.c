#include <core/type.h>
#include <core/log.h>
#include <core/macro.h>

#include <GL/glew.h>


static void dbg_callback(GLenum p0, GLenum p1, GLuint p2, GLenum severity, i32 p3, char const *message, void const *p4)
{
    walrus_unused(p0);
    walrus_unused(p1);
    walrus_unused(p2);
    walrus_unused(p3);
    walrus_unused(p4);
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            walrus_assert_msg(false, message);
            return;
        case GL_DEBUG_SEVERITY_MEDIUM:
            walrus_error(message);
            return;
        case GL_DEBUG_SEVERITY_LOW:
            walrus_warn(message);
            return;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            walrus_trace(message);
            return;
        default:
            walrus_assert_msg(false, "Unknown severity level!");
    }
}

GLenum glew_init(void)
{
    GLenum err = glewInit();
    if (err == GLEW_OK) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(dbg_callback, NULL);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
    }
    return err;
}

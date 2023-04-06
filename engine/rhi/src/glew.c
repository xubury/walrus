#include <type.h>
#include <GL/glew.h>

#include <log.h>
#include <macro.h>

static void dbg_callback(GLenum p0, GLenum p1, GLuint p2, GLenum severity, i32 p3, const char *message, const void *p4)
{
    UNUSED(p0)
    UNUSED(p1)
    UNUSED(p2)
    UNUSED(p3)
    UNUSED(p4)
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            ASSERT_MSG(false, "%s", message);
            return;
        case GL_DEBUG_SEVERITY_MEDIUM:
            // BD_ASSERT(false, message);
            log_error(message);
            return;
        case GL_DEBUG_SEVERITY_LOW:
            log_warn(message);
            return;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            log_trace(message);
            return;
        default:
            ASSERT_MSG(false, "Unknown severity level!");
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

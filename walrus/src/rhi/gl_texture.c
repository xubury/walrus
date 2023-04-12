#include "gl_texture.h"

#include <core/math.h>

static const GlFormat s_format_info[WR_RHI_FORMAT_COUNT] = {
    {GL_ALPHA, GL_ZERO, GL_ALPHA, GL_UNSIGNED_BYTE},  // Alpha8

    {GL_R8, GL_ZERO, GL_RED, GL_UNSIGNED_BYTE},            // R8
    {GL_R8_SNORM, GL_ZERO, GL_RED, GL_BYTE},               // R8S
    {GL_R32I, GL_ZERO, GL_RED_INTEGER, GL_INT},            // R32I
    {GL_R32UI, GL_ZERO, GL_RED_INTEGER, GL_UNSIGNED_INT},  // R32UI
    {GL_R16F, GL_ZERO, GL_RED, GL_HALF_FLOAT},             // R16F
    {GL_R32F, GL_ZERO, GL_RED, GL_FLOAT},                  // R32F

    {GL_RG8, GL_ZERO, GL_RG, GL_UNSIGNED_BYTE},            // RG8
    {GL_RG8_SNORM, GL_ZERO, GL_RG, GL_BYTE},               // RG8S
    {GL_RG32I, GL_ZERO, GL_RG_INTEGER, GL_INT},            // RG32I
    {GL_RG32UI, GL_ZERO, GL_RG_INTEGER, GL_UNSIGNED_INT},  // RG32UI
    {GL_RG16F, GL_ZERO, GL_RG, GL_HALF_FLOAT},             // RG16F
    {GL_RG32F, GL_ZERO, GL_RG, GL_FLOAT},                  // RG32F

    {GL_RGB8, GL_SRGB8, GL_RGB, GL_UNSIGNED_BYTE},           // RGB8
    {GL_RGB8_SNORM, GL_ZERO, GL_RGB, GL_BYTE},               // RGB8S
    {GL_RGB32I, GL_ZERO, GL_RGB_INTEGER, GL_INT},            // RGB32I
    {GL_RGB32UI, GL_ZERO, GL_RGB_INTEGER, GL_UNSIGNED_INT},  // RGB32UI
    {GL_RGB16F, GL_ZERO, GL_RGB, GL_HALF_FLOAT},             // RGB16F
    {GL_RGB32F, GL_ZERO, GL_RGB, GL_FLOAT},                  // RGB32F

    {GL_RGBA8, GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE},   // RGBA8
    {GL_RGBA8_SNORM, GL_ZERO, GL_RGBA, GL_BYTE},              // RGBA8S
    {GL_RGBA32I, GL_ZERO, GL_RGB_INTEGER, GL_INT},            // RGBA32I
    {GL_RGBA32UI, GL_ZERO, GL_RGB_INTEGER, GL_UNSIGNED_INT},  // RGBA32UI
    {GL_RGBA16F, GL_ZERO, GL_RGBA, GL_HALF_FLOAT},            // RGBA16F
    {GL_RGBA32F, GL_ZERO, GL_RGBA, GL_FLOAT},                 // RGBA32F

    {GL_DEPTH_COMPONENT24, GL_ZERO, GL_DEPTH_COMPONENT, GL_FLOAT},          // Depth24
    {GL_STENCIL_INDEX8, GL_ZERO, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE},       // Stencil8
    {GL_DEPTH24_STENCIL8, GL_ZERO, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8}  // Depth24Stencil8
};

static void tex_image(GLenum target, uint8_t mip, uint16_t x, uint16_t y, uint16_t z, uint16_t width, uint16_t height,
                      uint16_t depth, GLenum format, GLenum type, const void *data)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (target == GL_TEXTURE_2D) {
        glTexSubImage2D(target, mip, x, y, width, height, format, type, data);
    }
    else if (target == GL_TEXTURE_2D_MULTISAMPLE) {
    }
    else if (target == GL_TEXTURE_3D || target == GL_TEXTURE_2D_ARRAY) {
        glTexSubImage3D(target, mip, x, y, z, width, height, depth, format, type, data);
    }

    glGenerateMipmap(target);
}

void gl_texture_create(Walrus_TextureHandle handle, Walrus_TextureCreateInfo const *info)
{
    GLuint id     = 0;
    GLuint rbo    = 0;
    GLenum target = 0;

    bool const msaa_sample    = info->flags & WR_RHI_TEXTURE_MSAA_SAMPLE;
    u32        msaa_quality   = ((info->flags & WR_RHI_TEXTURE_RT_MSAA_MASK) >> WR_RHI_TEXTURE_RT_MSAA_SHIFT);
    msaa_quality              = walrus_u32satsub(msaa_quality, 1);
    msaa_quality              = walrus_min(msaa_quality == 0 ? 0 : 1 << msaa_quality, 16);
    bool const srgb           = info->flags & WR_RHI_TEXTURE_SRGB;
    bool const render_target  = info->flags & WR_RHI_TEXTURE_RT_MASK;
    bool const write_only     = info->flags & WR_RHI_TEXTURE_RT_WRITE_ONLY;
    bool       create_texture = true;

    target = msaa_sample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

    if (info->cubemap) {
        target = GL_TEXTURE_CUBE_MAP;
    }
    else if (info->depth > 1) {
        target = GL_TEXTURE_3D;
    }

    if (info->num_layers > 1) {
        switch (target) {
            case GL_TEXTURE_CUBE_MAP:
                target = GL_TEXTURE_CUBE_MAP_ARRAY;
                break;
            case GL_TEXTURE_2D_MULTISAMPLE:
                target = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
                break;
            default:
                target = GL_TEXTURE_2D_ARRAY;
                break;
        }
    }

    glGenTextures(1, &id);
    glBindTexture(target, id);

    GlFormat     gl_format     = s_format_info[info->format];
    const GLenum internal_fmt  = srgb ? gl_format.internal_srgb_format : gl_format.internal_format;
    bool const   texture_array = target == GL_TEXTURE_2D_ARRAY || target == GL_TEXTURE_CUBE_MAP_ARRAY;
    if (!write_only || (render_target && texture_array)) {
        if (texture_array) {
            glTexStorage3D(target, info->num_mipmaps, internal_fmt, info->width, info->height, info->num_layers);
        }
        else {
            if (target == GL_TEXTURE_3D) {
                if (msaa_sample) {
                    glTexStorage3DMultisample(target, msaa_quality, info->num_mipmaps, internal_fmt, info->width,
                                              info->height, info->depth);
                }
                else {
                    glTexStorage3D(target, info->num_mipmaps, internal_fmt, info->width, info->height, info->depth);
                }
            }
            else {
                if (msaa_sample) {
                    glTexStorage2DMultisample(target, msaa_quality, info->num_mipmaps, internal_fmt, info->width,
                                              info->height);
                }
                else {
                    glTexStorage2D(target, info->num_mipmaps, internal_fmt, info->width, info->height);
                }
            }
        }
    }
    if (render_target) {
        if (!msaa_sample && (msaa_quality != 0 || write_only) && !texture_array) {
            glGenRenderbuffers(1, &rbo);
            glBindRenderbuffer(GL_RENDERBUFFER, rbo);
            if (msaa_quality == 0) {
                glRenderbufferStorage(GL_RENDERBUFFER, gl_format.internal_format, info->width, info->height);
            }
            else {
                glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa_quality, gl_format.internal_format, info->width,
                                                 info->height);
            }
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
        }
        if (write_only) {
            create_texture = false;
        }
    }
    if (create_texture) {
        if (info->data) {
            if (texture_array) {
                tex_image(target, 0, 0, 0, 0, info->width, info->height, info->num_layers, gl_format.format,
                          gl_format.type, info->data);
            }
            else {
                tex_image(target, 0, 0, 0, 0, info->width, info->height, info->depth, gl_format.format, gl_format.type,
                          info->data);
            }
        }
    }
    glBindTexture(target, 0);

    g_ctx->textures[handle.id].id     = id;
    g_ctx->textures[handle.id].rbo    = rbo;
    g_ctx->textures[handle.id].target = target;
    g_ctx->textures[handle.id].width  = info->width;
    g_ctx->textures[handle.id].height = info->height;
    g_ctx->textures[handle.id].format = info->format;
    g_ctx->textures[handle.id].flags  = info->flags;
    g_ctx->textures[handle.id].gl     = gl_format;
}

void gl_texture_destroy(Walrus_TextureHandle handle)
{
    glDeleteTextures(1, &g_ctx->textures[handle.id].id);
    glDeleteRenderbuffers(1, &g_ctx->textures[handle.id].rbo);
    g_ctx->textures[handle.id].id  = 0;
    g_ctx->textures[handle.id].rbo = 0;
}

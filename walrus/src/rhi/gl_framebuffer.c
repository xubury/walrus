#include "gl_framebuffer.h"

#include <core/assert.h>
#include <string.h>

static const char *glEnumName(GLenum _enum)
{
#define GLENUM(_ty) \
    case _ty:       \
        return #_ty

    switch (_enum) {
        GLENUM(GL_TEXTURE);
        GLENUM(GL_RENDERBUFFER);

        GLENUM(GL_INVALID_ENUM);
        GLENUM(GL_INVALID_FRAMEBUFFER_OPERATION);
        GLENUM(GL_INVALID_VALUE);
        GLENUM(GL_INVALID_OPERATION);
        GLENUM(GL_OUT_OF_MEMORY);

        GLENUM(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
        GLENUM(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
        //			GLENUM(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER);
        //			GLENUM(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER);
        GLENUM(GL_FRAMEBUFFER_UNSUPPORTED);
        GLENUM(GL_FRAMEBUFFER_COMPLETE_EXT);
    }

#undef GLENUM
    return "<GLenum?>";
}

static void framebuffer_validate(void)
{
    GLenum complete = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    walrus_assert_msg(GL_FRAMEBUFFER_COMPLETE == complete, "glCheckFramebufferStatus failed : {0}", complete,
                      glEnumName(complete));
}

void gl_framebuffer_create(Walrus_FramebufferHandle handle, Walrus_Attachment *attachments, u8 num)
{
    GlFramebuffer *fb = &gl_ctx->framebuffers[handle.id];
    memset(fb->fbo, 0, sizeof(fb->fbo));
    glGenFramebuffers(1, &fb->fbo[0]);
    memcpy(fb->attachments, attachments, num * sizeof(Walrus_Attachment));
    fb->num_textures = num;

    gl_framebuffer_post_reset(fb);
}

void gl_framebuffer_destroy(Walrus_FramebufferHandle handle)
{
    GlFramebuffer *fb = &gl_ctx->framebuffers[handle.id];
    if (fb->fbo[0] != 0) {
        glDeleteFramebuffers(fb->fbo[1] == 0 ? 1 : 2, &fb->fbo[0]);
    }
    memset(fb->fbo, 0, sizeof(fb->fbo));
}

void gl_framebuffer_post_reset(GlFramebuffer *fb)
{
    if (fb->fbo[0] != 0) {
        bool   need_resolve = false;
        u32    color_id     = 0;
        GLenum buffers[WR_RHI_MAX_FRAMEBUFFER_ATTACHMENTS];
        glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo[0]);
        for (u8 i = 0; i < fb->num_textures; ++i) {
            Walrus_Attachment *attach = &fb->attachments[i];
            if (attach->handle.id != WR_INVALID_HANDLE) {
                GlTexture *texture = &gl_ctx->textures[attach->handle.id];
                if (color_id == 0) {
                    fb->width  = texture->width;
                    fb->height = texture->height;
                }

                GLenum             gl_attach = GL_COLOR_ATTACHMENT0 + color_id;
                Walrus_PixelFormat format    = texture->format;
                if (format == WR_RHI_FORMAT_DEPTH24) {
                    gl_attach = GL_DEPTH_ATTACHMENT;
                }
                else if (format == WR_RHI_FORMAT_STENCIL8) {
                    gl_attach = GL_STENCIL_ATTACHMENT;
                }
                else if (format == WR_RHI_FORMAT_DEPTH24STENCIL8) {
                    gl_attach = GL_DEPTH_STENCIL_ATTACHMENT;
                }
                else if (attach->access == WR_RHI_ACCESS_WRITE) {
                    buffers[color_id] = gl_attach;
                    ++color_id;
                }
                if (texture->rbo != 0) {
                    glFramebufferRenderbuffer(GL_FRAMEBUFFER, gl_attach, GL_RENDERBUFFER, texture->rbo);
                }
                else {
                    glFramebufferTexture(GL_FRAMEBUFFER, gl_attach, texture->id, attach->mip);
                }
                need_resolve |= texture->id != 0 && texture->rbo != 0;
            }
        }
        if (color_id != 0) {
            glDrawBuffers(color_id, buffers);
        }
        else {
            glDrawBuffer(GL_NONE);
        }
        framebuffer_validate();
        if (need_resolve) {
            glGenFramebuffers(1, &fb->fbo[1]);
            glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo[1]);
            for (u8 i = 0; i < fb->num_textures; ++i) {
                Walrus_Attachment *attach = &fb->attachments[i];
                if (attach->handle.id != WR_INVALID_HANDLE) {
                    GlTexture *texture = &gl_ctx->textures[attach->handle.id];
                    if (texture->id != 0) {
                        GLenum             gl_attach = GL_COLOR_ATTACHMENT0 + color_id;
                        Walrus_PixelFormat format    = texture->format;
                        if (format == WR_RHI_FORMAT_DEPTH24) {
                            gl_attach = GL_DEPTH_ATTACHMENT;
                        }
                        else if (format == WR_RHI_FORMAT_STENCIL8) {
                            gl_attach = GL_STENCIL_ATTACHMENT;
                        }
                        else if (format == WR_RHI_FORMAT_DEPTH24STENCIL8) {
                            gl_attach = GL_DEPTH_STENCIL_ATTACHMENT;
                        }
                        else {
                            ++color_id;
                        }
                        glFramebufferTexture(GL_FRAMEBUFFER, gl_attach, texture->id, attach->mip);
                    }
                }
            }
            framebuffer_validate();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void gl_framebuffer_resolve(GlFramebuffer *fb)
{
    if (fb->fbo[1] != 0) {
        u32 color_id = 0;
        for (u32 i = 0; i < fb->num_textures; ++i) {
            Walrus_Attachment *attach = &fb->attachments[i];
            if (attach->handle.id != WR_INVALID_HANDLE) {
                GlTexture         *texture    = &gl_ctx->textures[attach->handle.id];
                bool const         write_only = texture->flags & WR_RHI_TEXTURE_RT_WRITE_ONLY;
                Walrus_PixelFormat format     = texture->format;
                if (format != WR_RHI_FORMAT_DEPTH24 && format != WR_RHI_FORMAT_STENCIL8 &&
                    format != WR_RHI_FORMAT_DEPTH24STENCIL8) {
                    glDisable(GL_SCISSOR_TEST);
                    glReadBuffer(GL_COLOR_ATTACHMENT0 + color_id);
                    glDrawBuffer(GL_COLOR_ATTACHMENT0 + color_id);
                    glBindFramebuffer(GL_READ_FRAMEBUFFER, fb->fbo[0]);
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb->fbo[1]);
                    glBlitFramebuffer(0, 0, fb->width, fb->height, 0, 0, fb->width, fb->height, GL_COLOR_BUFFER_BIT,
                                      GL_LINEAR);
                    ++color_id;
                }
                else if (write_only) {
                    glDisable(GL_SCISSOR_TEST);
                    glBindFramebuffer(GL_READ_FRAMEBUFFER, fb->fbo[0]);
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb->fbo[1]);
                    glBlitFramebuffer(0, 0, fb->width, fb->height, 0, 0, fb->width, fb->height, GL_DEPTH_BUFFER_BIT,
                                      GL_NEAREST);
                }
            }
        }
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
    for (u32 i = 0; i < fb->num_textures; ++i) {
        Walrus_Attachment *attach = &fb->attachments[i];
        if (attach->handle.id != WR_INVALID_HANDLE) {
            GlTexture *texture = &gl_ctx->textures[attach->handle.id];
            glGenerateTextureMipmap(texture->id);
        }
    }
}

void gl_framebuffer_discard(GlFramebuffer *fb, u32 discard)
{
    GLenum buffers[WR_RHI_MAX_FRAMEBUFFER_ATTACHMENTS + 2];
    u32    id = 0;
    if ((discard & WR_RHI_CLEAR_DISCARD_COLOR_MASK) != WR_RHI_CLEAR_NONE) {
        for (u32 i = 0; i < fb->num_textures; ++i) {
            if ((discard & (WR_RHI_CLEAR_DISCARD_COLOR_0 << i)) != WR_RHI_CLEAR_NONE) {
                buffers[id++] = GL_COLOR_ATTACHMENT0 + i;
            }
        }
    }
    u32 ds_flags = discard & (WR_RHI_CLEAR_DISCARD_STENCIL | WR_RHI_CLEAR_DISCARD_DEPTH);
    if (ds_flags != WR_RHI_CLEAR_NONE) {
        if (ds_flags == (WR_RHI_CLEAR_DISCARD_DEPTH | WR_RHI_CLEAR_DISCARD_STENCIL)) {
            buffers[id++] = GL_DEPTH_STENCIL_ATTACHMENT;
        }
        else if (ds_flags == WR_RHI_CLEAR_DISCARD_DEPTH) {
            buffers[id++] = GL_DEPTH_ATTACHMENT;
        }
        else if (ds_flags == WR_RHI_CLEAR_DISCARD_STENCIL) {
            buffers[id++] = GL_STENCIL_ATTACHMENT;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo[0]);
    glInvalidateFramebuffer(GL_FRAMEBUFFER, id, buffers);
}

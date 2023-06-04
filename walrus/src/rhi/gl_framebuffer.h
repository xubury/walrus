#pragma once

#include "gl_context.h"

void gl_framebuffer_create(Walrus_FramebufferHandle handle, Walrus_Attachment *attachments, u8 num);

void gl_framebuffer_destroy(Walrus_FramebufferHandle handle);

void gl_framebuffer_post_reset(GlFramebuffer *fb);

void gl_framebuffer_resolve(GlFramebuffer *fb);

void gl_framebuffer_discard(GlFramebuffer *fb, u32 discard);

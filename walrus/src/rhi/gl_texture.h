#include "gl_context.h"

void gl_texture_create(Walrus_TextureHandle handle, Walrus_TextureCreateInfo const *info);

void gl_texture_destroy(Walrus_TextureHandle handle);

void gl_texture_resize(Walrus_TextureHandle handle, u32 width, u32 height, u32 depth, u8 num_mipmaps, u8 num_layers);

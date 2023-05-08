#pragma once

#include <core/type.h>
#include <rhi/rhi.h>

typedef struct _Walrus_Font Walrus_Font;

typedef struct {
    Walrus_TextureHandle handle;
    u32                  width;
    u32                  height;
    u32                  first_unicode;
    u32                  num_characters;
    u32                  font_height;
    void                *characters;
} Walrus_FontTexture;

typedef struct {
    vec2 offset0;
    vec2 offset1;
    f32  advance;
} Walrus_GlyphMetrics;

Walrus_Font *walrus_font_load_from_file(const char *filename);
void         walrus_font_free(Walrus_Font *font);

// cook font bitmap atlas to a texture
void walrus_font_texture_cook(Walrus_Font *font, Walrus_FontTexture *texture, u32 width, u32 height, u32 font_height,
                              u8 num_mipmaps, u64 flags, u32 first_unicode, u32 num_chars);
void walrus_font_texture_unload(Walrus_FontTexture *texture);

bool walrus_font_texture_unicode_uv(Walrus_FontTexture *texture, u32 unicode, vec2 uv0, vec2 uv1);

bool walrus_font_texture_unicode_metrics(Walrus_FontTexture *texture, u32 unicode, Walrus_GlyphMetrics *metrics);

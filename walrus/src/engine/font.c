#include <engine/font.h>
#include <core/memory.h>
#include <core/log.h>

#include <stdio.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

struct Walrus_Font {
    u8 *ttf_buffer;
};

Walrus_Font *walrus_font_load_from_file(const char *filename)
{
    Walrus_Font *font = NULL;
    FILE        *fd   = fopen(filename, "rb");
    if (fd) {
        font = walrus_malloc(sizeof(Walrus_Font));
        fseek(fd, 0, SEEK_END);
        u32 size = ftell(fd);
        fseek(fd, 0, SEEK_SET);
        font->ttf_buffer = walrus_malloc(size);
        fread(font->ttf_buffer, 1, size, fd);
        fclose(fd);
    }
    else {
        walrus_error("walrus_font_load_from_file fail to fopen");
    }

    return font;
}

void walrus_font_free(Walrus_Font *font)
{
    walrus_free(font->ttf_buffer);
    walrus_free(font);
}

void walrus_font_texture_cook(Walrus_Font *font, Walrus_FontTexture *texture, u32 width, u32 height, u32 font_height,
                              u8 num_mipmaps, u64 flags, u32 first_unicode, u32 num_chars)
{
    texture->characters     = walrus_new(stbtt_packedchar, num_chars);
    texture->width          = width;
    texture->height         = height;
    texture->first_unicode  = first_unicode;
    texture->font_height    = font_height;
    texture->num_characters = num_chars;

    u8 *bitmap = walrus_malloc(width * height);

    stbtt_pack_context pc;
    stbtt_PackBegin(&pc, bitmap, width, height, 0, 1, NULL);
    stbtt_PackFontRange(&pc, font->ttf_buffer, 0, font_height, first_unicode, num_chars, texture->characters);
    stbtt_PackEnd(&pc);

    u32 const bytes = width * height * 4;
    u8       *rgba  = walrus_malloc(bytes);
    for (u64 i = 0; i < width * height; ++i) {
        u64 stride       = i * 4;
        rgba[stride + 0] = bitmap[i];
        rgba[stride + 1] = bitmap[i];
        rgba[stride + 2] = bitmap[i];
        rgba[stride + 3] = bitmap[i];
    }

    texture->handle = walrus_rhi_create_texture2d(width, height, WR_RHI_FORMAT_RGBA8, num_mipmaps, flags, rgba);

    walrus_free(bitmap);
    walrus_free(rgba);
}

void walrus_font_texture_unload(Walrus_FontTexture *texture)
{
    walrus_rhi_destroy_texture(texture->handle);
    walrus_free(texture->characters);
}

bool walrus_font_texture_unicode_uv(Walrus_FontTexture *texture, u32 unicode, vec2 uv0, vec2 uv1)
{
    if (unicode >= texture->first_unicode && unicode < texture->first_unicode + texture->num_characters) {
        u32 index = unicode - texture->first_unicode;

        stbtt_packedchar *c = &((stbtt_packedchar *)texture->characters)[index];
        uv0[0]              = (f32)c->x0 / texture->width;
        uv0[1]              = (f32)c->y0 / texture->height;
        uv1[0]              = (f32)c->x1 / texture->width;
        uv1[1]              = (f32)c->y1 / texture->height;

        return true;
    }

    return false;
}

bool walrus_font_texture_unicode_metrics(Walrus_FontTexture *texture, u32 unicode, Walrus_GlyphMetrics *metrics)
{
    if (unicode >= texture->first_unicode && unicode < texture->first_unicode + texture->num_characters) {
        u32 index = unicode - texture->first_unicode;

        stbtt_packedchar *c = &((stbtt_packedchar *)texture->characters)[index];
        metrics->advance    = c->xadvance;
        metrics->offset0[0] = c->xoff;
        metrics->offset0[1] = c->yoff;
        metrics->offset1[0] = c->xoff2;
        metrics->offset1[1] = c->yoff2;

        return true;
    }

    return false;
}

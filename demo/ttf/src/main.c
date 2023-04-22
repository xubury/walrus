#include <engine/engine.h>
#include <core/memory.h>
#include <core/log.h>
#include <stdio.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

u8 ttf_buffer[1 << 25];

Walrus_AppError on_init(Walrus_App *app)
{
    FILE *fd = fopen("c:/windows/fonts/arialbd.ttf", "rb");
    if (fd) {
        i32 w, h;
        walrus_trace("load ttf success");
        stbtt_fontinfo font;
        u8            *bitmap;
        fread(ttf_buffer, 1, 1 << 25, fd);
        stbtt_InitFont(&font, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0));
        bitmap = stbtt_GetCodepointBitmap(&font, 0, stbtt_ScaleForPixelHeight(&font, 20), 'a', &w, &h, 0, 0);
        for (i32 j = 0; j < h; ++j) {
            char pitch[w + 1];
            for (i32 i = 0; i < w; ++i) pitch[i] = " .:ioVM@"[bitmap[j * w + i] >> 5];
            pitch[w] = 0;
            walrus_trace(pitch);
        }
        fclose(fd);
    }
    else {
        walrus_trace("load ttf failed");
    }
    return WR_APP_SUCCESS;
}

int main(void)
{
    Walrus_EngineOption opt;
    opt.window_title  = "ttf demo";
    opt.window_width  = 1440;
    opt.window_height = 900;
    opt.window_flags  = WR_WINDOW_FLAG_VSYNC | WR_WINDOW_FLAG_OPENGL;
    opt.minfps        = 30.f;

    Walrus_App *app = walrus_app_create(NULL);
    walrus_app_set_init(app, on_init);

    walrus_engine_init_run(&opt, app);

    return 0;
}

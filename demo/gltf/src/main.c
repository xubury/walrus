#include <engine/engine.h>
#include <core/memory.h>

int main(void)
{
    Walrus_EngineOption opt;
    opt.window_title  = "gltf demo";
    opt.window_width  = 1440;
    opt.window_height = 900;
    opt.window_flags  = WR_WINDOW_FLAG_VSYNC | WR_WINDOW_FLAG_OPENGL;
    opt.minfps        = 30.f;

    Walrus_App *app = walrus_app_create(NULL);

    walrus_engine_init_run(&opt, app);

    return 0;
}

#include <engine/engine.h>
#include <core/memory.h>
#include <core/log.h>
#include <stdalign.h>

int main(void)
{
    Walrus_App *app = walrus_app_create(NULL);

    walrus_engine_init_run("gltf", 1440, 900, app);

    return 0;
}

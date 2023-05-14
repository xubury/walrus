#include <engine/engine.h>
#include <engine/batch_renderer.h>
#include <rhi/rhi.h>
#include <core/memory.h>
#include <core/log.h>

#include <engine/font.h>

typedef struct {
    Walrus_FontTexture font;
} AppData;

Walrus_AppError on_init(Walrus_App *app)
{
    AppData *data = walrus_app_userdata(app);

    u32 width  = 1440;
    u32 height = 900;
    walrus_rhi_set_view_rect(0, 0, 0, width, height);
    mat4 p;
    glm_ortho(0, width, height, 0, 0, 1000, p);
    walrus_rhi_set_view_transform(0, GLM_MAT4_IDENTITY, p);
    walrus_rhi_set_view_clear(0, WR_RHI_CLEAR_DEPTH | WR_RHI_CLEAR_COLOR, 0, 1.0, 0);

    Walrus_Font *font = walrus_font_load_from_file("c:/windows/fonts/arialbd.ttf");
    walrus_font_texture_cook(font, &data->font, 512, 512, 40, 0, WR_RHI_SAMPLER_LINEAR, 32, 126);
    walrus_font_free(font);
    return WR_APP_SUCCESS;
}

void on_render(Walrus_App *app)
{
    AppData *data = walrus_app_userdata(app);

    walrus_batch_render_begin(0, WR_RHI_STATE_DEFAULT | WR_RHI_STATE_BLEND_ALPHA);
    mat4 m = GLM_MAT4_IDENTITY_INIT;
    glm_rotate(m, glm_rad(45), (vec3){1, 0, 0});
    versor q;
    glm_mat4_quat(m, q);
    walrus_batch_render_circle((vec3){200, 200, 0}, GLM_QUAT_IDENTITY, 100.0, 0xffffffff, 0.1, 0xffffffff, 0.1);
    walrus_batch_render_quad((vec3){0, 0, 0}, GLM_QUAT_IDENTITY, (vec2){100, 100}, 0xffffffff, 0.1, 0xffffffff, 0.1);
    walrus_batch_render_texture(data->font.handle, (vec3){1024, 512, -1}, GLM_QUAT_IDENTITY, (vec2){512, 512},
                                0xffffffff, 0, 0xffffffff, 0);
    walrus_batch_render_subtexture(data->font.handle, (vec2){0.5, 0}, (vec2){1.0, 1.0}, (vec3){512, 512, -1},
                                   GLM_QUAT_IDENTITY, (vec2){256, 512}, 0xffffffff, 0, 0xffffffff, 0);
    walrus_batch_render_circle((vec3){200, 512, -1}, GLM_QUAT_IDENTITY, 100, 0xffffffff, 0.1, 0xffffffff, 0.1);
    walrus_batch_render_string(
        &data->font,
        "Lorem ipsum dolor sit amet, officia excepteur ex fugiat reprehenderit enim labore culpa sint ad nisi Lorem\n"
        "pariatur mollit ex esse exercitation amet. Nisi anim cupidatat excepteur officia. Reprehenderit nostrud\n"
        "nostrud ipsum Lorem est aliquip amet voluptate voluptate dolor minim nulla est proident. Nostrud officia\n"
        "pariatur ut officia. Sit irure elit esse ea nulla sunt ex occaecat reprehenderit commodo officia dolor Lorem\n"
        "duis laboris cupidatat officia voluptate. Culpa proident adipisicing id nulla nisi laboris ex in Lorem sunt\n"
        "duis officia eiusmod. Aliqua reprehenderit commodo ex non excepteur duis sunt velit enim. Voluptate laboris\n"
        "sint cupidatat ullamco ut ea consectetur et est culpa et culpa duis.",
        (vec3){200, 512, 0}, GLM_QUAT_IDENTITY, (vec2){1, 1}, 0x00ffffff);
    walrus_batch_render_end();
}

int main(void)
{
    Walrus_App *app = walrus_app_create(malloc(sizeof(AppData)));
    walrus_app_set_init(app, on_init);
    walrus_app_set_render(app, on_render);

    walrus_engine_init_run("ttf", 1440, 900, app);

    return 0;
}

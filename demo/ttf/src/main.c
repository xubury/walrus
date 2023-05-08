#include <engine/engine.h>
#include <engine/batch_renderer.h>
#include <rhi/rhi.h>
#include <core/memory.h>
#include <core/log.h>
#include <stdio.h>

#include <engine/font.h>

typedef struct {
    Walrus_ProgramHandle shader;
    Walrus_FontTexture   font;
    Walrus_UniformHandle u_texture;
} AppData;

char const *vs_src =
    "out vec2 v_uv;\n"
    "const vec2 quad_verts[] = vec2[](vec2(-1.0f, 1.0f), vec2(-1.0f, -1.0f), vec2(1.0f, 1.0f), vec2(1.0f, -1.0f));"
    "const vec2 uvs[] = vec2[](vec2(0.0f, 0.0f), vec2(0.0f, 1.0f), vec2(1.0f, 0.0f), vec2(1.0f, 1.0f));"
    "void main() { \n"
    "    gl_Position = vec4(quad_verts[gl_VertexID], 0, 1.0);\n"
    "    v_uv = uvs[gl_VertexID];\n"
    "}\n";

char const *fs_src =
    "out vec4 frag_color;"
    "in vec2 v_uv;"
    "uniform sampler2D u_texture;"
    "void main() { "
    "    frag_color = texture(u_texture, v_uv);"
    "}";

Walrus_AppError on_init(Walrus_App *app)
{
    AppData *data = walrus_app_userdata(app);

    data->u_texture = walrus_rhi_create_uniform("u_texture", WR_RHI_UNIFORM_SAMPLER, 1);

    Walrus_ShaderHandle vs = walrus_rhi_create_shader(WR_RHI_SHADER_VERTEX, vs_src);
    Walrus_ShaderHandle fs = walrus_rhi_create_shader(WR_RHI_SHADER_FRAGMENT, fs_src);
    data->shader           = walrus_rhi_create_program((Walrus_ShaderHandle[]){vs, fs}, 2, true);

    u32 width  = 1440;
    u32 height = 900;
    walrus_rhi_set_view_rect(0, 0, 0, width, height);
    mat4 p;
    glm_ortho(0, width, height, 0, 0, 1000, p);
    walrus_rhi_set_view_transform(0, GLM_MAT4_IDENTITY, p);
    walrus_rhi_set_view_clear(0, WR_RHI_CLEAR_DEPTH | WR_RHI_CLEAR_COLOR, 0, 1.0, 0);

    Walrus_Font *font = walrus_font_load_from_file("c:/windows/fonts/arialbd.ttf");
    walrus_font_texture_cook(font, &data->font, 512, 512, 20, 0, WR_RHI_SAMPLER_LINEAR, 0, 126);
    walrus_font_free(font);
    return WR_APP_SUCCESS;
}

void on_render(Walrus_App *app)
{
    AppData *data = walrus_app_userdata(app);

    walrus_batch_render_begin(0, WR_RHI_STATE_DEFAULT);
    mat4 m = GLM_MAT4_IDENTITY_INIT;
    glm_rotate(m, glm_rad(45), (vec3){1, 0, 0});
    versor q;
    glm_mat4_quat(m, q);
    warlus_batch_render_circle((vec3){200, 200, 0}, GLM_QUAT_IDENTITY, 100.0, 0xffffffff, 0.1, 0xffffffff, 0.1);
    warlus_batch_render_quad((vec3){0, 0, 0}, GLM_QUAT_IDENTITY, (vec2){100, 100}, 0xffffffff, 0.1, 0xffffffff, 0.1);
    warlus_batch_render_texture(data->font.handle, (vec3){1024, 512, -1}, GLM_QUAT_IDENTITY, (vec2){512, 512},
                                0xffffffff, 0, 0xffffffff, 0);
    walrus_batch_render_subtexture(data->font.handle, (vec2){0.5, 0}, (vec2){1.0, 1.0}, (vec3){512, 512, -1},
                                   GLM_QUAT_IDENTITY, (vec2){256, 512}, 0xffffffff, 0, 0xffffffff, 0);
    walrus_batch_render_string(&data->font, "hello world", (vec3){512, 512, 0}, GLM_QUAT_IDENTITY, (vec2){2, 2},
                               0x00ffffff);
    walrus_batch_render_end();
}

int main(void)
{
    Walrus_EngineOption opt;
    opt.window_title  = "ttf demo";
    opt.window_width  = 1440;
    opt.window_height = 900;
    opt.window_flags  = WR_WINDOW_FLAG_VSYNC | WR_WINDOW_FLAG_OPENGL;
    opt.minfps        = 30.f;

    Walrus_App *app = walrus_app_create(malloc(sizeof(AppData)));
    walrus_app_set_init(app, on_init);
    walrus_app_set_render(app, on_render);

    walrus_engine_init_run(&opt, app);

    return 0;
}

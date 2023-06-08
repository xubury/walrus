layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_texcoord;
layout (location = 2) in vec2 a_uv0;
layout (location = 3) in vec2 a_uv1;
layout (location = 4) in float a_thickness;
layout (location = 5) in float a_fade;
layout (location = 6) in vec4 a_color;
layout (location = 7) in vec4 a_border_color;
layout (location = 8) in float a_tex_id;
layout (location = 9) in mat4 a_model;
uniform mat4 u_viewproj;
out vec2 v_local_pos;
out vec4 v_color;
out float v_thickness;
out float v_fade;
out vec4 v_border_color;
out vec2 v_texcoord;
out float v_tex_id;
void main()
{
    gl_Position = u_viewproj * a_model * vec4(a_pos, 0, 1.0);
    v_local_pos = a_pos;
    v_texcoord = a_texcoord * (a_uv1 - a_uv0) + a_uv0;
    v_color = a_color;
    v_thickness = a_thickness;
    v_fade = a_fade;
    v_border_color = a_border_color;
    v_tex_id = a_tex_id;
}

#pragma vertex
layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec4 a_color;

out vec2 v_uv;
out vec4 v_color;

uniform mat4 u_viewproj;

void main()
{
    gl_Position =  u_viewproj * vec4(a_pos, 0.0, 1.0);
    v_uv = a_uv;
    v_color = a_color;
}

#pragma fragment

out vec4 fragcolor;

in vec2 v_uv;
in vec4 v_color;

uniform sampler2D u_texture;
uniform float u_lod;

void main()
{
    fragcolor = textureLod(u_texture, v_uv, u_lod) * v_color;
}

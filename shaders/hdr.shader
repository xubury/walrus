#pragma vertex

layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_uv;
out vec2 v_uv;
void main()
{
    gl_Position = vec4(a_pos, 0, 1.0);
    v_uv = a_uv;
}

#pragma fragment

#include "srgb.glsl"
#include "tonemap.glsl"

out vec3 fragcolor;

in vec2 v_uv;

uniform sampler2D u_color_buffer;

void main()
{
    vec3 color = texture(u_color_buffer, v_uv).rgb;

    color = gt_tonemapping(color);
    color = linear_to_srgb(color, 2.2);

    fragcolor = color;
}

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

out vec4 fragcolor;

in vec2 v_uv;

uniform sampler2D u_color_buffer;
uniform sampler2D u_depth_buffer;

void main()
{
    fragcolor = texture(u_color_buffer, v_uv);
    gl_FragDepth = texture(u_depth_buffer, v_uv).r;
}

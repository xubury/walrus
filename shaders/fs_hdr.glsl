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

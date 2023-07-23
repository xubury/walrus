#pragma vertex

layout (location = 0) in vec2 a_pos;
layout (location = 1) in float a_thickness;
layout (location = 2) in float a_fade;
layout (location = 3) in vec4 a_color;
layout (location = 4) in vec4 a_boarder_color;
layout (location = 5) in mat4 a_model;
out vec2 v_localpos;
out float v_thickness;
out float v_fade;
out vec4 v_color;
out vec4 v_boarder_color;
uniform mat4 u_viewproj;
void main()
{
    gl_Position = u_viewproj * a_model * vec4(a_pos, 0, 1.0);
    v_localpos = a_pos;
    v_thickness = a_thickness;
    v_fade = a_fade;
    v_color = a_color;
    v_boarder_color = a_boarder_color;
}

#pragma fragment

out vec4 fragcolor;
in vec2 v_localpos;
in float v_thickness;
in float v_fade;
in vec4 v_color;
in vec4 v_boarder_color;
void main()
{
    float dist = 1 - length(v_localpos);
    float circle = smoothstep(0.0, v_fade, dist);
    if (circle == 0.0) {
        discard;
    }
    float boarder = 1 - smoothstep(v_thickness, v_thickness + v_fade, dist);
    fragcolor = mix(v_color, v_boarder_color, boarder);
    fragcolor.a *= circle;
}

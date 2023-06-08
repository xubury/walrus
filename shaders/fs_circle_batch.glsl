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

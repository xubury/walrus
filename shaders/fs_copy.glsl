out vec4 fragcolor;

in vec2 v_uv;

uniform sampler2D u_color_buffer;
uniform sampler2D u_depth_buffer;

void main()
{
    fragcolor = texture(u_color_buffer, v_uv);
    gl_FragDepth = texture(u_depth_buffer, v_uv).r;
}

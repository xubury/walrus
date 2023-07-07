out vec4 fragcolor;

in vec2 v_uv;

uniform sampler2D u_color_buffer;

void main()
{
    fragcolor = texture(u_color_buffer, v_uv);
}

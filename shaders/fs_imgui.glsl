out vec4 fragcolor;

in vec2 v_uv;
in vec4 v_color;

uniform sampler2D u_texture;

void main()
{
    fragcolor = texture(u_texture, v_uv) * v_color;
}

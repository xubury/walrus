out vec4 fragcolor;

in vec2 v_uv;
in vec4 v_color;

uniform sampler2D u_texture;
uniform float u_lod;

void main()
{
    fragcolor = textureLod(u_texture, v_uv, u_lod) * v_color;
}

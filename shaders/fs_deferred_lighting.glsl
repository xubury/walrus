out vec4 fragcolor;

in vec2 v_uv;

uniform sampler2D u_albedo;

void main()
{
    fragcolor = texture(u_albedo, v_uv);
}

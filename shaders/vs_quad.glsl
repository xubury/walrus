layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_texcoord;
out vec2 v_uv;
void main()
{
    gl_Position = vec4(a_pos, 0, 1.0);
    v_uv = a_texcoord;
}

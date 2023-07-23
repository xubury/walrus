#pragma vertex

layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_texcoord;
layout (location = 2) in vec2 a_uv0;
layout (location = 3) in vec2 a_uv1;
layout (location = 4) in float a_thickness;
layout (location = 5) in float a_fade;
layout (location = 6) in vec4 a_color;
layout (location = 7) in vec4 a_border_color;
layout (location = 8) in float a_tex_id;
layout (location = 9) in mat4 a_model;
uniform mat4 u_viewproj;
out vec2 v_local_pos;
out vec4 v_color;
out float v_thickness;
out float v_fade;
out vec4 v_border_color;
out vec2 v_texcoord;
out float v_tex_id;
void main()
{
    gl_Position = u_viewproj * a_model * vec4(a_pos, 0, 1.0);
    v_local_pos = a_pos;
    v_texcoord = a_texcoord * (a_uv1 - a_uv0) + a_uv0;
    v_color = a_color;
    v_thickness = a_thickness;
    v_fade = a_fade;
    v_border_color = a_border_color;
    v_tex_id = a_tex_id;
}

#pragma fragment

out vec4 fragcolor;
in vec2 v_local_pos;
in vec4 v_color;
in float v_thickness;
in float v_fade;
in vec4 v_border_color;
in vec2 v_texcoord;
in float v_tex_id;
uniform sampler2D u_textures[16];
void main()
{
    float boader = max(abs(v_local_pos.x), abs(v_local_pos.y)) - (0.5 - v_thickness / 2.f);
    boader = smoothstep(0.0, v_fade, boader);
    vec4 color = vec4(0);
    switch(int(v_tex_id))
    {
        case  0: color = texture(u_textures[ 0], v_texcoord); break;
        case  1: color = texture(u_textures[ 1], v_texcoord); break;
        case  2: color = texture(u_textures[ 2], v_texcoord); break;
        case  3: color = texture(u_textures[ 3], v_texcoord); break;
        case  4: color = texture(u_textures[ 4], v_texcoord); break;
        case  5: color = texture(u_textures[ 5], v_texcoord); break;
        case  6: color = texture(u_textures[ 6], v_texcoord); break;
        case  7: color = texture(u_textures[ 7], v_texcoord); break;
        case  8: color = texture(u_textures[ 8], v_texcoord); break;
        case  9: color = texture(u_textures[ 9], v_texcoord); break;
        case 10: color = texture(u_textures[10], v_texcoord); break;
        case 11: color = texture(u_textures[11], v_texcoord); break;
        case 12: color = texture(u_textures[12], v_texcoord); break;
        case 13: color = texture(u_textures[13], v_texcoord); break;
        case 14: color = texture(u_textures[14], v_texcoord); break;
        case 15: color = texture(u_textures[15], v_texcoord); break;
    }
    fragcolor = mix(v_color, v_border_color, boader) * color;
}

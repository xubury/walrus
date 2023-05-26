layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_tangent;
layout(location = 3) in vec2 a_uv;
uniform mat4 u_viewproj;
uniform mat4 u_model;
out vec3 v_normal;
out vec2 v_uv;
out vec3 v_tangent;
out vec3 v_bitangent;
void main() {
    gl_Position = u_viewproj * u_model * vec4(a_pos, 1);
    mat3 nmat = transpose(inverse(mat3(u_model)));
    v_normal = nmat * a_normal;
    v_uv = a_uv;
    v_tangent = nmat * a_tangent.xyz;
    v_tangent = normalize(v_tangent - dot(v_tangent, v_normal) * v_normal);
    v_bitangent = a_tangent.w * cross(v_tangent, v_normal);
}

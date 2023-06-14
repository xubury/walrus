layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_tangent;
layout(location = 3) in vec2 a_uv;
layout(location = 5) in vec4 a_joints;
layout(location = 6) in vec4 a_weights;
uniform mat4 u_viewproj;
uniform mat4 u_model;
out vec3 v_normal;
out vec2 v_uv;
out vec3 v_tangent;
out vec3 v_bitangent;

layout(std430, binding = 0) buffer JointsSSBO
{
    mat4 ssbo_joints[];
};

void main() {
    mat4 world = a_weights.x * ssbo_joints[int(a_joints.x)] +
                 a_weights.y * ssbo_joints[int(a_joints.y)] +
                 a_weights.z * ssbo_joints[int(a_joints.z)] +
                 a_weights.w * ssbo_joints[int(a_joints.w)];
    world = u_model * world;
    gl_Position = u_viewproj * world * vec4(a_pos, 1);
    mat3 nmat = transpose(inverse(mat3(world)));
    v_normal = nmat * a_normal;
    v_uv = a_uv;
    v_tangent = nmat * a_tangent.xyz;
    v_tangent = normalize(v_tangent - dot(v_tangent, v_normal) * v_normal);
    v_bitangent = a_tangent.w * cross(v_tangent, v_normal);
}

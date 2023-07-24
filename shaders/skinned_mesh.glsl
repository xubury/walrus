layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_tangent;
layout(location = 3) in vec2 a_uv;
layout(location = 5) in vec4 a_joints;
layout(location = 6) in vec4 a_weights;
uniform mat4 u_viewproj;
uniform mat4 u_model;
out vec3 v_pos;
out vec3 v_normal;
out vec2 v_uv;
out vec3 v_tangent;
out vec3 v_bitangent;

layout(std430, binding = 0) buffer WeightsSSBO
{
    float morph_weights[];
};

layout(std430, binding = 1) buffer JointsSSBO
{
    mat4 ssbo_joints[];
};


uniform bool u_has_morph;
uniform sampler2DArray u_morph_texture;

vec3 sample_morph_texture(int layer)
{
    vec3 morph = vec3(0);
    if (u_has_morph) {
        ivec3 size = textureSize(u_morph_texture, 0); 
        if (layer < size.z) {
            ivec3 img_coord = ivec3(gl_VertexID, 0, layer);
            for (; img_coord.y < size.y; ++img_coord.y) {
                vec3 offset = texelFetch(u_morph_texture, img_coord, 0).xyz;
                morph += morph_weights[img_coord.y] * offset;
            }
        }
    }
    return morph;
}

void main() {
    mat4 world = a_weights.x * ssbo_joints[int(a_joints.x)] +
                 a_weights.y * ssbo_joints[int(a_joints.y)] +
                 a_weights.z * ssbo_joints[int(a_joints.z)] +
                 a_weights.w * ssbo_joints[int(a_joints.w)];
    world = u_model * world;
    mat3 nmat = transpose(inverse(mat3(world)));

    v_pos = a_pos;
    v_pos += sample_morph_texture(0);
    v_pos = (world * vec4(v_pos, 1)).xyz;

    v_normal = a_normal;
    v_normal += sample_morph_texture(1);
    v_normal = nmat * v_normal;

    v_tangent = a_tangent.xyz;
    v_tangent += sample_morph_texture(2);
    v_tangent = nmat * v_tangent;
    v_tangent = normalize(v_tangent - dot(v_tangent, v_normal) * v_normal);
    v_bitangent = a_tangent.w * cross(v_tangent, v_normal);

    v_uv = a_uv;
    v_uv += sample_morph_texture(3).xy;

    gl_Position = u_viewproj * vec4(v_pos, 1);
}

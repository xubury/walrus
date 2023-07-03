#include "srgb.glsl"

layout(location = 0) out vec3 out_pos;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_tangent;
layout(location = 3) out vec3 out_bitangent;
layout(location = 4) out vec4 out_albedo;
layout(location = 5) out vec3 out_emissive;

in vec3 v_pos;
in vec3 v_normal;
in vec2 v_uv;
in vec3 v_tangent;
in vec3 v_bitangent;

uniform sampler2D u_albedo;
uniform vec4 u_albedo_factor;
uniform sampler2D u_emissive;
uniform vec3 u_emissive_factor;
uniform sampler2D u_normal;
uniform float u_normal_scale;
uniform bool u_has_normal;
uniform float u_alpha_cutoff;

void main() {
    vec3 light_dir = normalize(vec3(0, 0, 1));
    vec3 normal = normalize(v_normal);
    if (u_has_normal) {
        mat3 TBN = mat3(normalize(v_tangent), normalize(v_bitangent), normal);
        vec3 nomral_map = texture(u_normal, v_uv).xyz;
        normal = TBN * normalize(nomral_map * 2.0 - 1.0) * vec3(u_normal_scale, u_normal_scale, 1.0);
    }
    // float diff = max(dot(normal, light_dir), 0.0);
    vec3 emissive = texture(u_emissive, v_uv).rgb * u_emissive_factor;
    vec4 albedo = texture(u_albedo, v_uv) * u_albedo_factor;
    // vec3 color = linear_to_srgb(diff * albedo.rgb + emissive, 2.2);

    out_pos = v_pos;
    out_normal = normal;
    out_tangent = v_tangent;
    out_bitangent = v_bitangent;
    out_albedo = albedo;
    out_emissive = emissive;
};

#pragma vertex

#include "skinned_mesh.glsl"

#pragma fragment

#include "lighting.glsl"

out vec4 fragcolor;

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
    vec3 normal = normalize(v_normal);
    if (u_has_normal) {
        mat3 TBN = mat3(normalize(v_tangent), normalize(v_bitangent), normal);
        vec3 nomral_map = texture(u_normal, v_uv).xyz;
        normal = TBN * normalize(nomral_map * 2.0 - 1.0) * vec3(u_normal_scale, u_normal_scale, 1.0);
    }
    vec3 emissive = texture(u_emissive, v_uv).rgb * u_emissive_factor;
    vec4 albedo = texture(u_albedo, v_uv) * u_albedo_factor;
    if (albedo.a <= u_alpha_cutoff) {
        discard;
    }
    fragcolor = vec4(debug_lighting(normal, albedo.rgb, emissive), albedo.a);
};

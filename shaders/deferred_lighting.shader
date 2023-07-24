#pragma vertex

#include "quad.glsl"

#pragma fragment

#include "lighting.glsl"

out vec3 fragcolor;

in vec2 v_uv;

uniform sampler2D u_gpos;
uniform sampler2D u_gnormal;
uniform sampler2D u_galbedo;
uniform sampler2D u_gemissive;

uniform mat4 u_viewproj;

void main()
{
    vec3 pos = texture(u_gpos, v_uv).rgb;
    vec3 normal = texture(u_gnormal, v_uv).rgb;
    vec4 albedo = texture(u_galbedo, v_uv);
    vec3 emissive = texture(u_gemissive, v_uv).rgb;

    float lnormal = length(normal);
    if (lnormal != 0) {
        normal /= lnormal;
    }
    else {
        discard;
    }

    vec3 light_dir = normalize(vec3(0, 0, 1));
    float diff = max(dot(normal, light_dir), 0.0);
    fragcolor = debug_lighting(normal, albedo.rgb, emissive);
}

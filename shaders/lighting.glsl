vec3 debug_lighting(vec3 normal, vec3 albedo, vec3 emissive)
{
    vec3 light_dir = normalize(vec3(0, 0, 1));
    float diff = max(dot(normal, light_dir), 0.0);
    return (diff + 0.15) * albedo.rgb + emissive;
}

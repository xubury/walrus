vec3 linear_to_srgb(vec3 linear, float gamma)
{
    bvec3 cutoff = lessThan(linear, vec3(0.0031308));
    vec3 higher = vec3(1.055) * pow(linear, vec3(1.0 / gamma)) - vec3(0.055);
    vec3 lower = linear * vec3(12.92);

    return mix(higher, lower, cutoff);
}

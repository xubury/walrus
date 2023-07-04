float linearize_depth(float d,float z_near,float z_far)
{
    float z_n = 2.0 * d - 1.0;
    return 2.0 * z_near * z_far / (z_far + z_near - z_n * (z_far - z_near));
}

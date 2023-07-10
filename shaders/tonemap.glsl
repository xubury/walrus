vec3 aces_tonemapping(vec3 color)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    color = color;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0, 1);
}


float gt_tonemapping(float x)
{
	float m = 0.22;
	float a = 1.0;
	float c = 1.33;
	float P = 1.0;
	float l = 0.4;
	float l0 = ((P-m)*l) / a;
	float S0 = m + l0;
	float S1 = m + a * l0;
	float C2 = (a*P) / (P - S1);
	float L = m + a * (x - m);
	float T = m * pow(x/m, c);
	float S = P - (P - S1) * exp(-C2*(x - S0)/P);
	float w0 = 1 - smoothstep(0.0, m, x);
	float w2 = (x < m+l)?0:1;
	float w1 = 1 - w0 - w2;
	return T * w0 + L * w1 + S * w2;
}

vec3 gt_tonemapping(vec3 col) 
{
    return vec3(gt_tonemapping(col.r), gt_tonemapping(col.g), gt_tonemapping(col.b));
}

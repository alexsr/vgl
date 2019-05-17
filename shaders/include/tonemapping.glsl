#ifndef TONEMAPPING_GLSL
#define TONEMAPPING_GLSL

vec4 gamma_correction(vec4 c, float gamma) {
    return pow(c, vec4(1.0 / gamma));
}

vec4 linear_tm(vec4 c, float expo, float gamma) {
    return gamma_correction(c * expo, gamma);
}

vec4 reinhard_tm(vec4 c, float gamma) {
    return gamma_correction(c / (c + 1), gamma);
}

vec4 hejl_burgess_dawson_tm(vec4 c, float expo) {
    vec4 ec = max(vec4(0), c * expo - 0.004);
    return (ec * (6.2 * ec + 0.5)) / (ec * (6.2 * ec + 1.7) + 0.06);
}

vec4 uncharted2_func(vec4 x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec4 uncharted2_tm(vec4 col, float expo, float gamma) {
    float W = 11.2;
    float exposure_bias = 2.0;
    vec4 x = uncharted2_func(col * expo * exposure_bias);
    vec4 white_scale = uncharted2_func(vec4(W));
    return gamma_correction(x / white_scale, gamma);
}

#endif // TONEMAPPING_GLSL

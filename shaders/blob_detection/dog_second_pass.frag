#version 460

layout (location = 0) in vec2 _uv;

layout (binding = 0) uniform sampler2D tex;

layout (location = 0) uniform int in_kernel_small;
layout (location = 1) uniform int in_kernel_large;

layout (location = 0) out vec4 _color;

#include "../include/math_consts.glsl"

void main() {
    int kernel_small = min(in_kernel_small, in_kernel_large);
    int kernel_large = max(in_kernel_small, in_kernel_large);
    ivec2 tex_size = textureSize(tex, 0);
    vec2 uv_offset = vec2(1, 1) / vec2(tex_size);
	float kernel_large_half = (kernel_large - 1) / 2.0;
    float sigma_small = kernel_small / 6.0;
    float sigma2_small = 2.0 * sigma_small * sigma_small;
    float denom_small = 1.0 / sqrt(PI * sigma2_small);
    float sigma_large = kernel_large / 6.0;
    float sigma2_large = 2.0 * sigma_large * sigma_large;
    float denom_large = 1.0 / sqrt(PI * sigma2_large);
    float mean_small = 0.0;
    float gauss_sum_small = 0.0;
    float mean_large = 0.0;
    float gauss_sum_large = 0.0;
    for (float i = -kernel_large_half; i <= kernel_large_half; i++) {
        float value = texture(tex, _uv + uv_offset * vec2(0, i)).r;
        float gauss = denom_large * exp(-(i * i) / sigma2_large);
        mean_large += gauss * value;
        gauss_sum_large += gauss;
        gauss = denom_small * exp(-(i * i) / sigma2_small);
        mean_small += gauss * value;
        gauss_sum_small += gauss;
    }
    _color = sigma2_small / (sigma2_large - sigma2_small) * vec4(mean_large / gauss_sum_large - mean_small / gauss_sum_small) * 10.0;
}

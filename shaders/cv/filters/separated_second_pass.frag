#version 460

layout (location = 0) in vec2 _uv;

layout (binding = 0) uniform sampler2D og_tex;
layout (binding = 1) uniform sampler2D tex;

layout (location = 0) uniform int kernel_size;
layout (location = 1) uniform int filter_option;
layout (location = 2) uniform float strength;

layout (location = 0) out vec4 _color;

#include "../../include/binomial.glsl"

#define PI 3.1415926535897932384626433832795

void main() {
    ivec2 tex_size = textureSize(tex, 0);
    vec2 uv_offset = vec2(1, 1) / vec2(tex_size);
	float half_size = (kernel_size - 1) / 2.0;
	if (kernel_size <= 0) {
        _color = texture(tex, _uv);
		return;
	}

    // box blur
    if ((filter_option == 1)) {
        vec3 mean = vec3(0.0);
        for (float i = -half_size; i <= half_size; i++) {
            mean += texture(tex, _uv + uv_offset * vec2(0, i)).rgb;
        }
        _color.rgb = mean / kernel_size;
		_color.a = 1.0f;
    }
    // binomial filter
    else if (filter_option == 2) {
        float n = kernel_size - 1.0;
        vec3 mean = vec3(0.0);
        for (int i = 0; i < kernel_size; i++) {
            float n_factorial = falling_factorial(n, i);
            mean += n_factorial / factorial(i)
                    * texture(tex, _uv + uv_offset * vec2(0, i - half_size)).rgb;
        }
		float factor = pow(2.0, float(n));
        _color.rgb = mean / factor;
		_color.a = 1.0f;
    }
	// Gaussian blur
    else if (filter_option == 3 || filter_option == 4) {
		float sigma = kernel_size / 6.0;
		float sigma2 = 2.0 * sigma * sigma;
		float denom = 1.0 / sqrt(PI * sigma2);
        vec3 mean = vec3(0.0);
		float gauss_sum = 0.0;
        for (float i = -half_size; i <= half_size; i++) {
			float gauss = denom * exp(-(i * i) / sigma2);
            mean += gauss * texture(tex, _uv + uv_offset * vec2(0, i)).rgb;
			gauss_sum += gauss;
        }
        _color.rgb = mean / gauss_sum;
		if (filter_option == 4) {
			vec3 og = texture(og_tex, _uv).rgb;
			_color.rgb = og + strength * (og - _color.rgb);
		}
		_color.a = 1.0f;
    }
    else {
        _color = texture(tex, _uv);
    }
}

#version 460

layout (location = 0) in vec2 _uv;

layout (binding = 0) uniform sampler2D tex;

layout (location = 0) uniform int kernel_size;

layout (location = 0) out vec4 _color;

#define PI 3.1415926535897932384626433832795

void main() {
    ivec2 tex_size = textureSize(tex, 0);
    vec2 uv_offset = vec2(1, 1) / vec2(tex_size);
	float half_size = (kernel_size - 1) / 2.0;
	if (kernel_size <= 0) {
        _color = texture(tex, _uv);
		return;
	}

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
    _color.a = 1.0f;
}

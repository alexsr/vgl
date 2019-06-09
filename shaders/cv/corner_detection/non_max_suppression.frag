#version 460

layout (location = 0) in vec2 _uv;

layout (binding = 0) uniform sampler2D tex;

layout (location = 0) uniform int kernel_size;
layout (location = 1) uniform float threshold;

layout (location = 0) out float _color;

void main() {
    ivec2 tex_size = textureSize(tex, 0);
    vec2 uv_offset = vec2(1, 1) / vec2(tex_size);
	float half_size = (kernel_size - 1) / 2.0;
    float mid = texture(tex, _uv).r;
    if (mid < threshold) {
        _color = 0.0;
        return;
    }
    for (float i = -half_size; i <= half_size; i++) {
        for (float j = -half_size; j <= half_size; j++) {
            float v = texture(tex, _uv + uv_offset * vec2(i, j)).r;
            if (mid < v) {
                _color = 0.0;
                return;
            }
        }
    }
    _color = mid;
}

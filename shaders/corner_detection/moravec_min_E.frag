#version 460

layout (location = 0) in vec2 _uv;

layout (binding = 0) uniform sampler2D tex;

layout (location = 0) uniform int kernel_size;

layout (location = 0) out float _color;

void main() {
    ivec2 tex_size = textureSize(tex, 0);
    vec2 uv_offset = vec2(1, 1) / vec2(tex_size);
	float half_size = (kernel_size - 1) / 2.0;
    float min_v = 1.0 / 0.0;
    for (int u = -1; u <= 1; u++) {
        for (int v = -1; v <= 1; v++) {
            if (u == 0 && v == 0) {
                continue;
            }
            float ssd = 0.0;
            for (float i = -half_size; i <= half_size; i++) {
                for (float j = -half_size; j <= half_size; j++) {
                    float value = texture(tex, _uv + uv_offset * vec2(i + u, j + v)).r
                             - texture(tex, _uv + uv_offset * vec2(i, j)).r;
                    ssd += value * value;
                }
            }
            min_v = min(min_v, ssd);
        }
    }
    _color = min_v;
}

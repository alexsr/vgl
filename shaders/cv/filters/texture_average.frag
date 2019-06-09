#version 460

layout (location = 0) in vec2 _uv;

layout (binding = 0) uniform sampler2D tex;

layout (location = 0) uniform int kernel_size;

layout (location = 0) out vec4 _color;

void main() {
    ivec2 uv = ivec2(_uv * textureSize(tex, 0));

    vec4 mid = vec4(0.0);
    if (kernel_size % 2 == 0) {
        _color = texture(tex, _uv);
    }
    else {
        int half_size = kernel_size / 2;
        for (int i = -half_size; i <= half_size; i++) {
            for (int j = -half_size; j <= half_size; j++) {
                mid += texelFetch(tex, uv + ivec2(i, j), 0);
            }
        }
        _color = mid / kernel_size / kernel_size;
    }
}

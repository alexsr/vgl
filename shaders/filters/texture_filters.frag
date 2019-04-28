#version 460

layout (location = 0) in vec2 _uv;

layout (binding = 0) uniform sampler2D tex;

layout (location = 0) uniform int kernel_size;
layout (location = 1) uniform int filter_option;

layout (location = 0) out vec4 _color;

void main() {
    ivec2 tex_size = textureSize(tex, 0);
    vec2 uv_offset = vec2(1, 1) / vec2(tex_size);


    vec4 mid = vec4(0.0);
    if (filter_option == 1 && kernel_size % 2 != 0) {
        int half_size = kernel_size / 2;
        for (int i = -half_size; i <= half_size; i++) {
            for (int j = -half_size; j <= half_size; j++) {
                mid += texture(tex, _uv + uv_offset * vec2(i, j));
            }
        }
        _color = mid / kernel_size / kernel_size;
    }
    else if (filter_option == 2) {
        vec4 left = -2.0 * texture(tex, _uv + uv_offset * vec2(-1, 0));
        vec4 left_top = -texture(tex, _uv + uv_offset * vec2(-1, 1));
        vec4 left_bottom = -texture(tex, _uv + uv_offset * vec2(-1, -1));
        vec4 right = 2.0 * texture(tex, _uv + uv_offset * vec2(1, 0));
        vec4 right_top = texture(tex, _uv + uv_offset * vec2(1, 1));
        vec4 right_bottom = texture(tex, _uv + uv_offset * vec2(1, -1));
        _color = 2.0 * (left + left_top + left_bottom + right + right_top + right_bottom) / 8.0;
    }
    else {
        _color = texture(tex, _uv);
    }
}

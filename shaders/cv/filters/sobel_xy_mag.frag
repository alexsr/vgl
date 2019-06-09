#version 460

layout (location = 0) in vec2 _uv;

layout (binding = 0) uniform sampler2D sobel;

layout (location = 0) out float _sobel;

void main() {
    ivec2 tex_size = textureSize(sobel, 0);
    vec2 uv_offset = vec2(1, 1) / vec2(tex_size);
    float top_x = texture(sobel, _uv + uv_offset * vec2(0, 1)).x;
    float mid_x = texture(sobel, _uv).x;
    float bottom_x = texture(sobel, _uv + uv_offset * vec2(0, -1)).x;
    float top_y = texture(sobel, _uv + uv_offset * vec2(0, 1)).y;
    float bottom_y = texture(sobel, _uv + uv_offset * vec2(0, -1)).y;
    float sobel_x = top_x + 2.0 * mid_x + bottom_x;
    float sobel_y = -top_y + bottom_y;
    _sobel = sqrt(sobel_x * sobel_x + sobel_y * sobel_y);
}

#version 460

layout (location = 0) in vec2 _uv;

layout (binding = 0) uniform sampler2D tex;
layout (location = 0) uniform int pass = 0;

layout (location = 0) out vec4 _sobel;

void main() {
    ivec2 tex_size = textureSize(tex, 0);
    vec2 uv_offset = vec2(pass == 0, pass == 1) / vec2(tex_size);
    float mid = texture(tex, _uv).r;
    if (pass == 0) {
        float left = texture(tex, _uv + uv_offset * vec2(-1, 0)).r;
        float right = texture(tex, _uv + uv_offset * vec2(1, 0)).r;
        _sobel.x = -left + right;
        _sobel.y = left + 2.0 * mid + right;
    }
    else if (pass == 1) {
        vec2 top = texture(tex, _uv + uv_offset * vec2(0, 1)).xy;
        vec2 bottom = texture(tex, _uv + uv_offset * vec2(0, -1)).xy;
        _sobel.x = top.x + 2.0 * mid + bottom.x;
        _sobel.y = -top.y + bottom.y;
    }
}

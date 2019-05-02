#version 460

layout (location = 0) in vec2 _uv;

layout (binding = 0) uniform sampler2D tex;

layout (location = 0) out vec4 _sobel;

void main() {
    ivec2 tex_size = textureSize(tex, 0);
    vec2 uv_offset = vec2(1, 1) / vec2(tex_size);
	float left = texture(tex, _uv + uv_offset * vec2(-1, 0)).r;
    float mid = texture(tex, _uv).r;
    float right = texture(tex, _uv + uv_offset * vec2(1, 0)).r;
    _sobel.x = -left + right;
    _sobel.y = left + 2.0 * mid + right;
}

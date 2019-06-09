#version 460

layout (location = 0) in vec2 _uv;

layout (binding = 0) uniform sampler2D tex;

layout (location = 0) out vec4 _color;

void main() {
    ivec2 tex_size = textureSize(tex, 0);
    vec2 uv_offset = vec2(1, 1) / vec2(tex_size);
    vec4 left = texture(tex, _uv + uv_offset * vec2(-1, 0));
    vec4 top = texture(tex, _uv + uv_offset * vec2(0, 1));
    vec4 mid = texture(tex, _uv);
    vec4 right = texture(tex, _uv + uv_offset * vec2(1, 0));
    vec4 bottom = texture(tex, _uv + uv_offset * vec2(0, -1));
    _color = left + top - 4.0 * mid + right + bottom;
    _color = _color * 10;
}

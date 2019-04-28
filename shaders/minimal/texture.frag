#version 460

layout (location = 0) in vec2 _uv;

layout (binding = 0) uniform sampler2D tex;

layout (location = 0) out vec4 _color;

void main() {
    _color = texture(tex, _uv);
}

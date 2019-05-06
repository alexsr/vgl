#version 460

layout (location = 0) in vec2 _uv;
layout (location = 1) in vec4 _color;

layout (location = 0) out vec4 _frag_color;

void main() {
    if (dot(_uv, _uv) > 0.1) {
        discard;
    }
    _frag_color = _color;
}

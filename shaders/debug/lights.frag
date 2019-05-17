#version 460

layout (location = 0) in vec4 _color;

layout (location = 0) out vec4 _frag_color;

void main() {
    _frag_color = _color;
}

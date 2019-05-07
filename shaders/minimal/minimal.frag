#version 460

layout (location = 0) in vec3 _position;

layout (location = 0) out vec4 _color;

void main() {
    _color.rgb = _position;
}

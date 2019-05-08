#version 460

#extension GL_ARB_bindless_texture : require

layout (location = 0) in vec2 _uv;

layout (location = 0) out vec4 _color;

layout (bindless_sampler) uniform sampler2D tex;

void main() {
    _color = texture(tex, _uv);
}

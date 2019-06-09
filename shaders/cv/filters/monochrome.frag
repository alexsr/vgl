#version 460

layout (location = 0) in vec2 _uv;

layout (binding = 0) uniform sampler2D tex;

layout (location = 0) out vec4 _color;

vec4 monochrome_linear(vec4 c) {
    return vec4(vec3(c.r * 0.2126 + c.g * 0.7154 + c.b * 0.0722), 1.0);
}

void main() {
    vec4 mid = texture(tex, _uv);
    _color = monochrome_linear(mid);
}

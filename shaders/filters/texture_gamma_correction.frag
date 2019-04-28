#version 460

layout (location = 0) in vec2 _uv;

layout (binding = 0) uniform sampler2D tex;

layout (location = 0) out vec4 _color;

vec4 pow(vec4 v, float exponent) {
    return vec4(pow(v.x, exponent), pow(v.y, exponent), pow(v.z, exponent), 1.0);
}

void main() {
    _color = pow(texture(tex, _uv), 2.2);
}

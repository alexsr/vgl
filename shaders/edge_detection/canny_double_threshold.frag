#version 460

layout (location = 0) in vec2 _uv;

layout (binding = 0) uniform sampler2D tex;

layout (location = 0) uniform float threshold_low;
layout (location = 1) uniform float threshold_high;

layout (location = 0) out float _color;

void main() {
    float mid = texture(tex, _uv).r;
    if (mid < threshold_low) {
        _color = 0.0;
    }
    else if (mid < threshold_high) {
        _color = 0.5;
    }
    else {
        _color = 1.0;
    }
}

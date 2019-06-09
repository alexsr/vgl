#version 460

layout (location = 0) in vec2 _uv;

layout (binding = 0) uniform sampler2D tex;
layout (location = 0) uniform int x;

layout (location = 0) out vec4 _color;

#include "../../include/color_conversion.glsl"

vec3 rot_x(vec3 color, float x) {
    vec3 hsv = rgb_to_hsv(color);
    hsv.r = hsv.r + x;
    if (hsv.r > 360.0) {
        hsv.r -= 360.0;
    }
    return hsv_to_rgb(hsv);
}

void main() {
    ivec2 tex_size = textureSize(tex, 0);
    _color = vec4(rot_x(texture(tex, _uv).rgb, float(x)), 1.0);
}

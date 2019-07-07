#version 460

#extension GL_ARB_bindless_texture : require

#include "../include/math_consts.glsl"
#include "colors.glsl"

layout (location = 0) uniform float aspect_ratio;

layout (location = 0) out vec4 _color;

layout (location = 0) in vec2 _uv;

vec4 scene(in vec2 uv) {
    vec2 circle_uv = (2.0 * uv - 1.0) * vec2(aspect_ratio, 1.0);
    vec4 bg_color = 1.3 * mix(CYAN, MAGENTA, clamp(uv.y * uv.y, 0, 1));
    float radius = 0.5;
    vec2 diff = vec2(0, 0.2) - circle_uv;
    float dist = length(diff) / radius;
    float dist_from_top = (radius + diff.y);
    bool stripes = !(dist_from_top > 0.59 && dist_from_top < 0.65) && !(dist_from_top > 0.71 && dist_from_top < 0.745)
     && !(dist_from_top > 0.78 && dist_from_top < 0.805) && !(dist_from_top > 0.825 && dist_from_top < 0.84)
     && !(dist_from_top > 0.86 && dist_from_top < 0.872);
    if (dist < 1.0 && stripes) {
        float sun_grad = exp(-dist_from_top * dist_from_top);
        return mix(2.0 * mix(vec4(1.0), MAGENTA, sun_grad), bg_color, smoothstep(0.0, 1.0, dist * dist * dist * dist));
    }
    else {
        return bg_color;
    }
}

void main() {
    _color = scene(_uv);
    _color.a = 1.0;
}
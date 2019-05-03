#version 460

layout (location = 0) in vec2 _uv;

layout (binding = 0) uniform sampler2D tex;

layout (location = 0) uniform int kernel_size;
layout (location = 1) uniform float threshold;

layout (location = 0) out float _color;

#include "../include/math_consts.glsl"

void main() {
    ivec2 tex_size = textureSize(tex, 0);
    vec2 uv_offset = vec2(1, 1) / vec2(tex_size);
	float half_size = (kernel_size - 1) / 2.0;
    vec2 mid = texture(tex, _uv).rg;
    if (mid.r < threshold) {
        _color = 0.0;
        return;
    }
    float angle = sign(mid.g) * mid.g;
    float angle_45 = PI * 0.25;
    float angle_90 = PI * 0.5;
    float angle_135 = PI * 0.75;
    if (angle <= angle_45) {
        float t = angle / angle_45;
        float v_ene = (1.0 - t) * texture(tex, _uv + uv_offset * vec2(1, 0)).r
                      + t * texture(tex, _uv + uv_offset * vec2(1, 1)).r;
        float v_wsw = (1.0 - t) * texture(tex, _uv + uv_offset * vec2(-1, 0)).r
                      + t * texture(tex, _uv + uv_offset * vec2(-1, -1)).r;
        if (mid.r < max(v_ene, v_wsw)) {
            _color = 0.0;
            return;
        }
    }
    else if (angle <= angle_90) {
        float t = (angle - angle_45) / angle_45;
        float v_nen = (1.0 - t) * texture(tex, _uv + uv_offset * vec2(1, 1)).r
                      + t * texture(tex, _uv + uv_offset * vec2(0, 1)).r;
        float v_sws = (1.0 - t) * texture(tex, _uv + uv_offset * vec2(-1, -1)).r
                      + t * texture(tex, _uv + uv_offset * vec2(0, -1)).r;
        if (mid.r < max(v_nen, v_sws)) {
            _color = 0.0;
            return;
        }
    }
    else if (angle <= angle_135) {
        float t = (angle - angle_90) / angle_45;
        float v_nnw = (1.0 - t) * texture(tex, _uv + uv_offset * vec2(0, 1)).r
                      + t * texture(tex, _uv + uv_offset * vec2(-1, 1)).r;
        float v_sse = (1.0 - t) * texture(tex, _uv + uv_offset * vec2(0, -1)).r
                      + t * texture(tex, _uv + uv_offset * vec2(1, -1)).r;
        if (mid.r < max(v_nnw, v_sse)) {
            _color = 0.0;
            return;
        }
    }
    else if (angle <= PI) {
        float t = (angle - angle_135) / angle_45;
        float v_nww = (1.0 - t) * texture(tex, _uv + uv_offset * vec2(-1, 1)).r
                      + t * texture(tex, _uv + uv_offset * vec2(-1, 0)).r;
        float v_see = (1.0 - t) * texture(tex, _uv + uv_offset * vec2(1, -1)).r
                      + t * texture(tex, _uv + uv_offset * vec2(1, 0)).r;
        if (mid.r < max(v_nww, v_see)) {
            _color = 0.0;
            return;
        }
    }
    _color = mid.r;
}

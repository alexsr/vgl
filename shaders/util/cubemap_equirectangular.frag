#version 460

#extension GL_ARB_bindless_texture : require

layout (location = 0) in vec3 _uv;
layout (location = 0) out vec4 _color;

layout (location = 0, bindless_sampler) uniform sampler2D tex;

#include "../include/bindings.glsl"
#include "../include/tonemapping.glsl"
#include "../include/math_consts.glsl"

struct Cubemap_config {
    bool hdr;
    float gamma;
    float exposure;
    int tone_mapping;
};

layout (std430, binding = CUBEMAP_CONFIG_BINDING) readonly buffer cubemap_buffer {
    Cubemap_config cm;
};

void main() {
    vec3 sphere = normalize(_uv);
    vec2 uv = vec2((atan(sphere.z, sphere.x) + PI) / PI / 2.0, acos(sphere.y) / PI);
    _color = texture(tex, uv);
    if (cm.hdr) {
        switch (cm.tone_mapping) {
            case 0: // Linear
                _color = linear_tm(_color, cm.exposure, cm.gamma);
                break;
            case 1: // Reinhard
                _color = reinhard_tm(_color, cm.gamma);
                break;
            case 2: // Jim Hejl and Richard Burgess-Dawson
                _color = hejl_burgess_dawson_tm(_color, cm.exposure);
                break;
            case 3: // Uncharted 2
                _color = uncharted2_tm(_color, cm.exposure, cm.gamma);
        }
    }
}

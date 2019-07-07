#version 460

#extension GL_ARB_bindless_texture : require

#include "binding.glsl"

layout (location = 0) in float _lifetime;
layout (location = 1) flat in int _tex_id;

layout (location = 0) out vec4 color;

layout (std430, binding = ICED_TEA_TEXTURE_BINDING) buffer iced_ssbo {
    sampler2D iced_textures[];
};

void main() {
    color = texture(iced_textures[_tex_id], vec2(gl_PointCoord.x * 0.8, gl_PointCoord.y));
    color.a = min(1.0, color.a * _lifetime);
    // color = vec4(1.0);
        // discard;
    if (color.a <= 0.0) {
        discard;
    }
}

#version 460

#extension GL_ARB_bindless_texture : require

layout (location = 0) in flat int _mat_id;
layout (location = 1) in flat int _tex_diffuse;
layout (location = 2) in vec2 _uv;

#include "../include/material.glsl"
#include "../include/bindings.glsl"

layout (std430, binding = MATERIAL_BINDING) buffer material_buffer {
    Material materials[];
};

layout (std430, binding = TEXTURE_REFS_BINDING) buffer tex_ref_buffer {
    sampler2D textures[];
};

void main() {
    if (_mat_id != -1) {
        float alpha = materials[_mat_id].diffuse.a;
        if (textures.length() > 0) {
            if (_tex_diffuse != -1) {
                alpha = texture(textures[_tex_diffuse], _uv).a;
            }
        }
        if (alpha == 0) {
            discard;
        }
    }
}

#version 460

#extension GL_ARB_bindless_texture : require

layout (location = 0) in vec3 _position;
layout (location = 1) in vec3 _normal;
layout (location = 2) in vec2 _uv;
layout (location = 3) in vec3 _cam_pos;
layout (location = 4) in flat int _mat_id;
layout (location = 5) in flat int _tex_diffuse;
layout (location = 6) in flat int _tex_specular;

layout (location = 0) out vec4 _color;

#include "../include/material.glsl"
#include "../include/bindings.glsl"
#include "../include/shading.glsl"

layout (std430, binding = MATERIAL_BINDING) buffer material_buffer {
    Material materials[];
};

layout (std430, binding = TEXTURE_REFS_BINDING) buffer tex_ref_buffer {
    sampler2D textures[];
};

layout (location = 0, bindless_sampler) uniform samplerCube cubemap;
layout (location = 1, bindless_sampler) uniform sampler2D cubemap_equirect;
layout (location = 2) uniform bool equirect;

#include "../include/math_consts.glsl"
#include "../include/tonemapping.glsl"
#include "../include/cubemap.glsl"

layout (std430, binding = CUBEMAP_CONFIG_BINDING) readonly buffer cubemap_buffer {
    Cubemap_config cm;
};

void main() {
    _color.rgb = vec3(0);
    vec3 normal = _normal;
    vec3 v = normalize(_cam_pos - _position);
    vec3 r = reflect(-v, normal);
    vec3 sphere = normalize(r);
    if (equirect) {
        vec2 uv = vec2((atan(sphere.z, sphere.x) + PI) / PI / 2.0, acos(sphere.y) / PI);
        _color.rgb = texture(cubemap_equirect, uv).rgb;
    }
    else {
        _color.rgb = texture(cubemap, sphere).rgb;
    }
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
    if (_mat_id != -1) {
        vec3 specular_color = materials[_mat_id].specular.rgb;
        float shininess = materials[_mat_id].specular.w;
        vec4 diffuse_color = materials[_mat_id].diffuse;
        if (textures.length() > 0) {
            if (_tex_diffuse != -1) {
                diffuse_color = texture(textures[_tex_diffuse], _uv);
                if (diffuse_color.a == 0.0) {
                    discard;
                }
            }
            if (_tex_specular != -1) {
                vec4 spec = texture(textures[_tex_specular], _uv);
                specular_color = spec.rgb;
                shininess = spec.w;
            }
        }
    }
    _color.w = 1.0;
}

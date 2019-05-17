#version 460

#extension GL_ARB_bindless_texture : require

layout (location = 0) in vec3 _position;
layout (location = 1) in vec3 _normal;
layout (location = 2) in vec2 _uv;
layout (location = 3) in vec3 _cam_pos;
layout (location = 4) in flat int _mat_id;
layout (location = 5) in flat int _tex_diffuse;
layout (location = 6) in flat int _tex_specular;

layout (location = 0) out vec4 _diffuse;
layout (location = 1) out vec4 _specular;
layout (location = 2) out vec3 o_normal;
layout (location = 3) out vec3 o_pos;

#include "../include/light.glsl"
#include "../include/material.glsl"
#include "../include/bindings.glsl"

layout (std430, binding = LIGHTS_BINDING) buffer light_buffer {
    Light lights[];
};

layout (std430, binding = MATERIAL_BINDING) buffer material_buffer {
    Material materials[];
};

layout (std430, binding = TEXTURE_REFS_BINDING) buffer tex_ref_buffer {
    sampler2D textures[];
};

float phong_spec(vec3 light_dir, vec3 view_dir, vec3 normal, float shininess) {
    return pow(max(dot(reflect(light_dir, normal), view_dir), 0), shininess);
}

float blinn_phong_spec(vec3 light_dir, vec3 view_dir, vec3 normal, float shininess) {
    vec3 halfway_dir = normalize(light_dir + view_dir);
    return pow(max(dot(normal, halfway_dir), 0), shininess);
}

void main() {
    _diffuse = vec4(1);
    _specular = vec4(0);
    o_normal = normalize(_normal);
    o_pos = _position;
    if (_mat_id != -1) {
        _diffuse = materials[_mat_id].diffuse;
        _specular = materials[_mat_id].specular;
        if (textures.length() > 0) {
            if (_tex_diffuse != -1) {
                _diffuse = texture(textures[_tex_diffuse], _uv);
            }
            if (_tex_specular != -1) {
                _specular = texture(textures[_tex_specular], _uv);
            }
        }
    }
}

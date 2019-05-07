#version 460

layout (location = 0) in vec3 _position;
layout (location = 1) in vec3 _normal;
layout (location = 2) in vec2 _uv;
layout (location = 3) in vec3 _cam_pos;
layout (location = 4) in flat int _mat_id;
layout (location = 5) in flat int _tex_diffuse;
layout (location = 6) in flat int _tex_specular;

layout (location = 0) out vec4 _color;

layout (constant_id = 0) const uint texture_count = gl_MaxCombinedTextureImageUnits - 1;

layout (binding = 0) uniform sampler2D textures[texture_count];

#include "../include/light.glsl"
#include "../include/material.glsl"
#include "../include/bindings.glsl"

layout (std430, binding = LIGHTS_BINDING) buffer light_buffer {
    Light lights[];
};

layout (std430, binding = MATERIAL_BINDING) buffer material_buffer {
    Material materials[];
};

void main() {
    _color.rgb = vec3(1);
    if (_mat_id != -1) {
        _color = materials[_mat_id].emissive;
        vec3 specular_color = materials[_mat_id].specular.rgb;
        float shininess = materials[_mat_id].specular.w;
        vec4 diffuse_color = materials[_mat_id].diffuse;
        vec3 normal = normalize(_normal);
        vec3 pos = _position;
        if (texture_count > 0) {
            if (_tex_diffuse != -1) {
                diffuse_color = texture(textures[_tex_diffuse], _uv);
                if (diffuse_color.a == 0.0) {
                    diffuse_color = materials[_mat_id].diffuse;
                }
            }
            if (_tex_specular != -1) {
                vec4 spec = texture(textures[_tex_specular], _uv);
                specular_color = spec.rgb;
            }
        }
        vec3 v = normalize(_cam_pos - pos);
        for (int i = 0; i < lights.length(); i++) {
            if (lights[i].type == 0) {
                _color += lights[i].color * diffuse_color;
            }
            else if (lights[i].type == 1) {
                vec3 light_dir = normalize(lights[i].pos.xyz - _position);
                float cos_phi = max(dot(light_dir, normal), 0.0);
                float cos_psi_n = pow(max(dot(reflect(light_dir, normal), v), 0), shininess);
                _color.rgb += lights[i].color.rgb * (cos_phi * diffuse_color.rgb + cos_psi_n * specular_color);
            }
        }
    }
    _color.w = 1.0;
}

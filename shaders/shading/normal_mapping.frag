#version 460

#extension GL_ARB_bindless_texture : require

layout (location = 0) in vec3 _position;
layout (location = 1) in vec3 _normal;
layout (location = 2) in vec2 _uv;
layout (location = 3) in vec3 _cam_pos;
layout (location = 4) in flat int _mat_id;
layout (location = 5) in flat int _tex_diffuse;
layout (location = 6) in flat int _tex_specular;
layout (location = 7) in mat3 _from_tspace;

layout (location = 0) out vec4 _color;

#include "../include/light.glsl"
#include "../include/material.glsl"
#include "../include/bindings.glsl"
#include "../include/shading.glsl"

layout (std430, binding = LIGHTS_BINDING) buffer light_buffer {
    Light lights[];
};

layout (std430, binding = MATERIAL_BINDING) buffer material_buffer {
    Material materials[];
};

layout (std430, binding = TEXTURE_REFS_BINDING) buffer tex_ref_buffer {
    sampler2D textures[];
};

layout (location = 0, bindless_sampler) uniform sampler2D n_map;

void main() {
    _color.rgb = vec3(0);
    if (_mat_id != -1) {
        _color = materials[_mat_id].emissive;
        vec3 specular_color = materials[_mat_id].specular.rgb;
        float shininess = materials[_mat_id].specular.w;
        vec4 diffuse_color = materials[_mat_id].diffuse;
        vec3 normal = (normalize(_from_tspace * (normalize(texture(n_map, _uv).xyz * 2.0 - 1.0))));
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
        vec3 v = normalize(_cam_pos - _position);
        for (int i = 0; i < lights.length(); i++) {
            vec3 pos_to_light = lights[i].pos.xyz - _position;
            float dist2 = dot(pos_to_light, pos_to_light);
            float dist = sqrt(dist2);
            float attenuation = 1.0 / (lights[i].attenuation.constant + lights[i].attenuation.linear * dist
                + dist2 * lights[i].attenuation.quadratic);
            vec4 light_color = attenuation * lights[i].color;
            float cos_phi = 0;
            float cos_psi_n = 0;
            if (lights[i].type == 0) {
                cos_phi = 1;
            }
            if (lights[i].type == 1) {
                vec3 pos_to_light_norm = normalize(pos_to_light);
                cos_phi = max(dot(pos_to_light_norm, normal), 0.0);
                if (shininess != 0) {
                    cos_psi_n = blinn_phong_spec(pos_to_light_norm, v, normal, shininess);
                }
            }
            else if (lights[i].type == 2) {
                vec3 light_dir = normalize(lights[i].dir.xyz);
                cos_phi = max(dot(-light_dir, normal), 0.0);
                if (shininess != 0) {
                    cos_psi_n = blinn_phong_spec(light_dir, v, normal, shininess);
                }
            }
            else if (lights[i].type == 3) {
                vec3 pos_to_light_norm = normalize(pos_to_light);
                vec3 light_dir = normalize(lights[i].dir.xyz);
                float cos_angle = dot(light_dir, -pos_to_light_norm);
                if (cos_angle > lights[i].outer_cutoff) {
                    float soft_factor = smoothstep(0.0, mix(0.0, 0.1, 1 - cos_angle + 0.01), cos_angle - lights[i].outer_cutoff);
                    cos_phi = soft_factor * dot(pos_to_light_norm, normal);
                    if (shininess != 0) {
                        cos_psi_n = soft_factor * blinn_phong_spec(pos_to_light_norm, v, normal, shininess);
                    }
                }
            }
            _color.rgb += light_color.rgb * (cos_phi * diffuse_color.rgb + cos_psi_n * specular_color);
        }
    }
    _color.w = 1.0;
}

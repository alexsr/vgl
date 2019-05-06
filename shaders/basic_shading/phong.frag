#version 460

layout (location = 0) in vec4 _position;
layout (location = 1) in vec4 _normal;
layout (location = 2) in flat int _obj_id;

layout (location = 0) out vec4 _color;

layout (location = 0) uniform float shininess;

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
    vec3 normal = normalize(vec3(_normal));
    vec3 v = -normalize(vec3(_position));
    _color = vec4(0.0);
    for (int i = 0; i < lights.length(); i++) {
        if (lights[i].type == 1) {
            vec3 light_dir = normalize(vec3(lights[i].pos - _position));
            float cos_phi = max(dot(light_dir, normal), 0.0);
            float cos_psi_n = pow(max(dot(reflect(light_dir, normal), v), 0), materials[_obj_id].specular.w);
            _color.rgb += lights[i].color.rgb * (cos_phi * materials[_obj_id].diffuse.rgb + cos_psi_n * materials[_obj_id].specular.rgb);
        }
    }
    _color.w = 1.0;
}

#version 460

layout (location = 0) in vec4 _position;
layout (location = 1) in vec4 _normal;

layout (location = 0) out vec4 _color;

layout (location = 0) uniform float shininess;

#include "../include/light.glsl"
#include "../include/bindings.glsl"

layout (std430, binding = LIGHTS_BINDING) buffer light_buffer {
    Light lights[];
};

void main() {
    vec3 normal = normalize(vec3(_normal));
    vec3 v = -normalize(vec3(_position));
    _color = vec4(0.0);
    for (int i = 0; i < lights.length(); i++) {
        if (lights[i].type == 1) {
            vec3 light_dir = normalize(vec3(lights[i].pos - _position));
            float cos_phi = max(dot(light_dir, normal), 0.0);
            float cos_psi_n = pow(max(dot(reflect(light_dir, normal), v), 0), shininess);
            _color += lights[i].color * (cos_phi * vec4(1.0) + cos_psi_n * vec4(1.0));
        }
    }
    _color.w = 1.0;
}

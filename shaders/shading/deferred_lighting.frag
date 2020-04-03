#version 460

#extension GL_ARB_bindless_texture : require

layout (location = 0) in vec2 _uv;

layout (location = 0, bindless_sampler) uniform sampler2D _diffuse;
layout (location = 1, bindless_sampler) uniform sampler2D _specular;
layout (location = 2, bindless_sampler) uniform sampler2D _normal;
layout (location = 3, bindless_sampler) uniform sampler2D _position;

layout (location = 0) out vec4 _color;

#include "../include/light.glsl"
#include "../include/bindings.glsl"
#include "../include/shading.glsl"
#include "../include/cam.glsl"

layout (std430, binding = CAM_BINDING) buffer cam_buffer {
    Camera cam;
};

layout (std430, binding = LIGHTS_BINDING) buffer light_buffer {
    Light lights[];
};

void main() {
    vec3 position = texture(_position, _uv).xyz;
    vec3 normal = texture(_normal, _uv).xyz;
    if (dot(normal, normal) > 2.0) {
        discard;
    }
    normal = normalize(normal);
    vec4 diffuse = texture(_diffuse, _uv);
    vec4 specular_temp = texture(_specular, _uv);
    vec3 specular = specular_temp.rgb;
    float shininess = specular_temp.a;
    vec3 v = normalize(cam.position.xyz - position);
    for (int i = 0; i < lights.length(); i++) {
        vec3 pos_to_light = lights[i].pos.xyz - position;
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
        _color.rgb += light_color.rgb * (cos_phi * diffuse.rgb + cos_psi_n * specular);
    }
    _color.a = 1.0;
}

#version 460

#extension GL_ARB_bindless_texture : require

#include "binding.glsl"
#include "../include/cam.glsl"
#include "particle.glsl"

layout (std430, binding = CAM_BINDING) buffer cam_buffer {
    Camera cam;
};

layout (std430, binding = PARTICLE_BINDING) buffer particle_ssbo {
    Particle particles[];
};

struct Particle_system_config {
    int update_count;
    int offset;
    float speed_scale;
    float lifetime_scale;
    float point_size;
};

layout (std430, binding = SYSTEM_CONFIG_BINDING) readonly buffer config_ssbo {
    Particle_system_config config;
};

layout (std430, binding = ICED_TEA_TEXTURE_BINDING) buffer iced_ssbo {
    sampler2D iced_textures[];
};

layout (location = 0) uniform int water_system_count;

layout (location = 0) out float _lifetime;
layout (location = 1) flat out int _tex_id;

void main() {
    int id = water_system_count + gl_InstanceID;
    // _ppos = normalize(position);
    _tex_id = int(min(int(mod(float(gl_InstanceID), 6.0)), 4));//clamp(, 0, iced_textures.length() - 1);
    // _tex_id = 0;
    _lifetime = particles[id].lifetime;
    gl_Position = cam.proj * cam.view * vec4(particles[id].pos.xyz, 1.0);
    gl_PointSize = 5.0 / gl_Position.w;
}

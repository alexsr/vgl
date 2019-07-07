#version 460

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

layout (std430, binding = WATER_SYSTEM_CONFIG_BINDING) readonly buffer config_ssbo {
    Particle_system_config config;
};

layout (location = 0) out vec4 _color;

void main() {
    int id = gl_InstanceID;
    // _ppos = normalize(position);
    _color = vec4(particles[id].color, particles[id].lifetime);
    gl_Position = cam.proj * cam.view * vec4(particles[id].pos.xyz, 1.0);
    gl_PointSize = config.point_size / gl_Position.w;
}

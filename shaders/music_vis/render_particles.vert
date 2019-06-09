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

layout (location = 0) uniform float point_size;

layout (location = 0) out vec4 _color;
// layout (location = 1) out vec3 _ppos;

void main() {
    int id = gl_InstanceID;
    // _ppos = normalize(position);
    _color = vec4(particles[id].color, particles[id].lifetime);
    gl_Position = cam.proj * cam.view * vec4(particles[id].pos.xyz, 1.0);
    gl_PointSize = point_size / gl_Position.w;
}

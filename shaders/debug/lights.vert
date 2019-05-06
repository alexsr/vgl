#version 460

#include "../include/cam.glsl"
#include "../include/light.glsl"
#include "../include/bindings.glsl"

layout (std430, binding = LIGHTS_BINDING) buffer light_buffer {
    Light lights[];
};

layout (std430, binding = CAM_BINDING) buffer cam_buffer {
    Camera cam;
};

layout (location = 0) out vec2 _uv;
layout (location = 1) out vec4 _color;

void main() {
    _uv = vec2(gl_VertexID & 1, (gl_VertexID & 2) >> 1);
    _uv = _uv * 2.0 - 1.0;
    vec4 position = transpose(cam.view) * vec4(_uv * 0.1, 0.0, 0.0) + lights[gl_InstanceID].pos;
    _color = lights[gl_InstanceID].color;
    gl_Position = cam.proj * cam.view * vec4(position.xyz, 1.0);
}
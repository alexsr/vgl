#version 460

#include "../include/cam.glsl"

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 normal;

#include "../include/bindings.glsl"

layout (std430, binding = CAM_BINDING) buffer cam_buffer {
    Camera cam;
};

layout (location = 0) out vec4 _position;
layout (location = 1) out vec4 _normal;

void main() {
    _position = position;
    _normal = normal;
    gl_Position = cam.proj * cam.view * _position;
}
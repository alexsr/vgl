#version 460

#include "../include/bindings.glsl"
#include "../include/cam.glsl"

layout (location = 0) in vec4 position;

layout (std430, binding = CAM_BINDING) buffer cam_buffer {
    Camera cam;
};

layout (location = 0) out vec4 _position;

void main() {
    _position = cam.proj * cam.view * position;
    gl_Position = cam.proj * cam.view * position;
}
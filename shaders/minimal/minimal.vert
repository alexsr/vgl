#version 460

#include "../include/bindings.glsl"
#include "../include/cam.glsl"

layout (location = 0) in vec3 position;

layout (std430, binding = CAM_BINDING) buffer cam_buffer {
    Camera cam;
};

layout (location = 0) out vec3 _position;

void main() {
    _position = position;
    gl_Position = cam.proj * vec4(cam.rotation * (cam.position + _position), 1.0);
}

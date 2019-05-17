#version 460

layout (location = 0) in vec3 cube_pos;
layout (location = 0) out vec3 _uv;

#include "../include/bindings.glsl"
#include "../include/cam.glsl"

layout (std430, binding = CAM_BINDING) buffer cam_buffer {
    Camera cam;
};

void main() {
    _uv = cube_pos;
    gl_Position = (cam.proj * mat4(mat3(cam.view)) * vec4(cube_pos, 1.0)).xyww;
}

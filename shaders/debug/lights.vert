#version 460

layout (location = 0) in vec3 cube_pos;

#include "../include/cam.glsl"
#include "../include/light.glsl"
#include "../include/bindings.glsl"

layout (std430, binding = LIGHTS_BINDING) buffer light_buffer {
    Light lights[];
};

layout (std430, binding = CAM_BINDING) buffer cam_buffer {
    Camera cam;
};

layout (location = 0) out vec4 _color;

void main() {
    _color = lights[gl_InstanceID].color;
    gl_Position = (cam.proj * cam.view * vec4(lights[gl_InstanceID].pos.xyz + cube_pos * 0.1, 1.0));
}
#version 460

#include "../include/bindings.glsl"
#include "../include/cam.glsl"
#include "../include/scene_object.glsl"

layout (location = 0) in vec3 position;
layout (location = 2) in vec2 uv;

layout (std430, binding = CAM_BINDING) buffer cam_buffer {
    Camera cam;
};

layout (std430, binding = SCENE_OBJECT_BINDING) buffer scene_object_buffer {
    Scene_object objects[];
};

layout (location = 0) out int _mat_id;
layout (location = 1) out int _tex_diffuse;
layout (location = 2) out vec2 _uv;

void main() {
    _uv = uv;
    _mat_id = objects[gl_DrawID].material_id;
    _tex_diffuse = objects[gl_DrawID].texture_diffuse;
    gl_Position = cam.proj * vec4(cam.rotation * (cam.position + position), 1.0);
}

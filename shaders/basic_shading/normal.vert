#version 460

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 normal;

#include "../include/bindings.glsl"
#include "../include/cam.glsl"
#include "../include/scene_object.glsl"

layout (std430, binding = CAM_BINDING) buffer cam_buffer {
    Camera cam;
};

layout (std430, binding = SCENE_OBJECT_BINDING) buffer scene_object_buffer {
    Scene_object objects[];
};

layout (location = 0) out vec4 _position;
layout (location = 1) out vec4 _normal;
layout (location = 2) out flat int _obj_id;

void main() {
    _position = position;
    _normal = normal;
    _obj_id = objects[gl_DrawID].material_id;
    gl_Position = cam.proj * cam.view * _position;
}
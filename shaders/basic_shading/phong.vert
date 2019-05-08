#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

#include "../include/bindings.glsl"
#include "../include/cam.glsl"
#include "../include/scene_object.glsl"

layout (std430, binding = CAM_BINDING) buffer cam_buffer {
    Camera cam;
};

layout (std430, binding = SCENE_OBJECT_BINDING) buffer scene_object_buffer {
    Scene_object objects[];
};

layout (location = 0) out vec3 _position;
layout (location = 1) out vec3 _normal;
layout (location = 2) out vec2 _uv;
layout (location = 3) out vec3 _cam_pos;
layout (location = 4) out flat int _mat_id;
layout (location = 5) out flat int _tex_diffuse;
layout (location = 6) out flat int _tex_specular;

void main() {
    _position = position;
    _cam_pos = cam.position;
    _normal = normal;
    _uv = uv;
    _mat_id = objects[gl_DrawID].material_id;
    _tex_diffuse = objects[gl_DrawID].texture_diffuse;
    _tex_specular = objects[gl_DrawID].texture_specular;
    gl_Position = cam.proj * vec4(cam.rotation * (cam.position + _position), 1.0);
}

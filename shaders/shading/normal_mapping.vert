#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 tangent;

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
layout (location = 7) out mat3 _from_tspace;

void main() {
    int id = gl_InstanceID;
    _position = vec3(objects[id].model * vec4(position, 1.0));
    _cam_pos = cam.position;
    mat3 inv_model = mat3(inverse(transpose(objects[id].model)));
    vec3 n_normal = normalize(normal);
    vec3 n_tangent = normalize(tangent);
    _normal = normalize(inv_model * normal);
    vec3 renorm_tangent = normalize(n_tangent - dot(n_tangent, n_normal) * n_normal);
    _from_tspace = inv_model * mat3(normalize(renorm_tangent), normalize(cross(n_normal, renorm_tangent)), n_normal);
    _uv = uv;
    _mat_id = objects[id].material_id;
    _tex_diffuse = objects[id].texture_diffuse;
    _tex_specular = objects[id].texture_specular;
    gl_Position = cam.proj * cam.view * vec4(_position, 1.0);
}

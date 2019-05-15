#version 460

layout (location = 0) out vec2 _uv;
layout (location = 1) out vec3 _ray_dir;

#include "../include/bindings.glsl"
#include "../include/cam.glsl"

layout (std430, binding = CAM_BINDING) buffer cam_buffer {
    Camera cam;
};

void main() {
    _uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    vec4 pos = vec4(_uv - 1.0, 0.0, 1.0);
	vec4 temp_ray = cam.inv_vp * pos;
	temp_ray.xyz /= temp_ray.w;
    _ray_dir = temp_ray.xyz;
    gl_Position = pos;
}

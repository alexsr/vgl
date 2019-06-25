#version 460

#extension GL_ARB_bindless_texture : require

#include "binding.glsl"
#include "../include/cam.glsl"

layout (location = 0) in vec2 pos;
layout (location = 0) out vec4 _pos;
layout (location = 1) out vec2 _uv;

layout (std430, binding = HEIGHTS_BINDING) buffer heights_ssbo {
    float heights[];
};

layout (std430, binding = CAM_BINDING) buffer cam_buffer {
    Camera cam;
};

layout (std430, binding = VISUALIZATION_SETTINGS_BINDING) buffer vis_settings {
    vec2 mesh_size;
    ivec2 grid_res;
};

layout (location = 0, bindless_sampler) uniform sampler2D height_map;

void main() {
    _uv = pos.xy / vec2(grid_res);
    vec2 uv = pos.xy / vec2(grid_res - 1);
    _pos = vec4(1.0);
    // _pos.y = heights[int(pos.y) * grid_res.x + grid_res.x - abs(2 * int(pos.x) - grid_res.x)] * 0.5;
    _pos.xz = uv * mesh_size - vec2(mesh_size.x / 2.0, 0.0);
    uv.x = 2.0 * abs(abs(uv.x - 0.5) - 0.5);
    // _pos.x *= 1.0/((_uv.y) * (_uv.y + 1));
    _pos.y = texture(height_map, uv).x * 0.5;
    _pos = cam.view * _pos;
    gl_Position = cam.proj * _pos;
}

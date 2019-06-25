#version 460
layout (quads, equal_spacing, ccw) in;

layout (location = 0) in vec4 _pos_te[];
// layout (location = 1) in vec2 _uv_te[];

layout (location = 0) out vec4 _pos;
layout (location = 1) out vec2 _uv;

#include "binding.glsl"
#include "../include/cam.glsl"

layout (std430, binding = CAM_BINDING) buffer cam_buffer {
    Camera cam;
};

layout (std430, binding = HEIGHTS_BINDING) buffer heights_ssbo {
    float heights[];
};

layout (std430, binding = VISUALIZATION_SETTINGS_BINDING) buffer vis_settings {
    vec2 mesh_size;
    ivec2 grid_res;
    ivec2 patch_size;
};

void main(void) {
    // interpolate in horizontal direction between vert. 0 and 3
    vec4 p0 = mix(_pos_te[0], _pos_te[3], gl_TessCoord.x);
    // interpolate in horizontal direction between vert. 1 and 2
    vec4 p1 = mix(_pos_te[1], _pos_te[2], gl_TessCoord.x);
    // interpolate in vert direction
    vec4 p = mix(p0, p1, gl_TessCoord.y);
    _uv = p.xz / vec2(grid_res);// * mesh_size;//mesh_size.y);
    p.y = heights[min(int(p.z) * grid_res.x + int(p.x), heights.length() - 1)];
    p.xz = p.xz / vec2(grid_res) * mesh_size - vec2(mesh_size.x / 2.0, 0.0);
    _pos = cam.view * p;
    // tePatchDistance = gl_TessCoord.xy;
    gl_Position = cam.proj * _pos;
}
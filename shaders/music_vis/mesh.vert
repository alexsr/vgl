#version 460

#extension GL_ARB_bindless_texture : require

#include "binding.glsl"
#include "../include/cam.glsl"
#include "../include/math_consts.glsl"

layout (location = 0) in vec2 pos;
layout (location = 0) out vec4 _pos;
layout (location = 1) out vec2 _uv;
layout (location = 2) out float _height;

layout (std430, binding = CAM_BINDING) buffer cam_buffer {
    Camera cam;
};

layout (std430, binding = VISUALIZATION_SETTINGS_BINDING) buffer vis_settings {
    vec2 mesh_size;
    ivec2 grid_res;
};

layout (location = 0, bindless_sampler) uniform sampler2D height_map;
layout (location = 1, bindless_sampler) uniform sampler2D noise_tex;
layout (location = 2) uniform float time;
layout (location = 3) uniform float offset;

void main() {
    _uv = pos.xy / vec2(grid_res - 1);
    vec2 uv = pos.xy / vec2(grid_res - 1);
    _pos = vec4(1.0);
    // _pos.y = heights[int(pos.y) * grid_res.x + grid_res.x - abs(2 * int(pos.x) - grid_res.x)] * 0.5;
    _pos.xz = uv * mesh_size - vec2(mesh_size.x / 2.0, 0.0);
    uv.x = 2.0 * abs(abs(uv.x - 0.5) - 0.5);
    // _pos.x *= 1.0/((_uv.y) * (_uv.y + 1));
    _height = texture(height_map, uv).x * 0.3;
    if (uv.x > 0.94) {
        _height -= 0.005;
        if (uv.x > 0.95) {
            _height += 0.01 * sin(0.1 * texture(noise_tex, vec2((uv.x - 0.94) / 0.06, uv.y) + vec2(sin(time * 0.003), cos(time * 0.0009)) + vec2(0, offset)).r);
        }
    }
    _pos.y = _height - 0.01 * max(0, (exp(uv.y * uv.y / 1.12) - 2.0));
    _pos = cam.view * _pos;
    gl_Position = cam.proj * _pos;
}

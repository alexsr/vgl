#version 460

#extension GL_ARB_bindless_texture : require

#include "binding.glsl"
#include "../include/cam.glsl"
#include "../include/math_consts.glsl"
#include "billboard.glsl"

layout (std430, binding = CAM_BINDING) buffer cam_buffer {
    Camera cam;
};

layout (std430, binding = VISUALIZATION_SETTINGS_BINDING) buffer vis_settings {
    vec2 mesh_size;
    ivec2 grid_res;
};

layout (std430, binding = BILLBOARD_BINDING) buffer billboard_ssbo {
    Billboard billboards[];
};

layout (std430, binding = BILLBOARD_TEXTURES_BINDING) buffer bb_tex_ssbo {
    Billboard_texture bb_tex[];
};

layout (location = 0) uniform float offset;
layout (location = 1, bindless_sampler) uniform sampler2D noise_tex;

layout (location = 0) out vec4 _pos;
layout (location = 1) out vec2 _uv;
layout (location = 2) flat out int _id;

void main() {
    _uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    Billboard bb = billboards[gl_InstanceID];
    float z = mod(bb.pos.y + 2.0 * offset, 2.0);
    if (z <= 0.01) {
        billboards[gl_InstanceID].noise_offset += gl_InstanceID * 3.4129;
        billboards[gl_InstanceID].height_noise += gl_InstanceID * 4.412;
    }
    float noise = texture(noise_tex, vec2(gl_InstanceID / float(billboards.length()), cos(billboards[gl_InstanceID].noise_offset))).r;
    _id = int(mod(gl_InstanceID / float(billboards.length()) + noise, 1.0) * (bb_tex.length() - 1));
    _pos = cam.view * vec4(vec2(2.0f * _uv
        * vec2(bb_tex[_id].aspect_ratio, 1.0 + (1.0 + sin(billboards[gl_InstanceID].height_noise)) / 4.0)
    - 1.0f + vec2(bb.pos.x, 0.99)) * bb.scale + vec2(0.0, -0.01 * (exp(z * z / 1.12) - 1.0)), 2.0 - z, 1.0);
    gl_Position = cam.proj * _pos;    
}

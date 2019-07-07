#version 460

#extension GL_ARB_bindless_texture : require

#include "binding.glsl"
#include "../include/cam.glsl"
#include "../include/math_consts.glsl"
#include "billboard.glsl"
#include "colors.glsl"

layout (std430, binding = CAM_BINDING) buffer cam_buffer {
    Camera cam;
};

layout (std430, binding = VISUALIZATION_SETTINGS_BINDING) buffer vis_settings {
    vec2 mesh_size;
    ivec2 grid_res;
};

layout (std430, binding = BILLBOARD_TEXTURES_BINDING) buffer bb_tex_ssbo {
    Billboard_texture bb_tex[];
};

layout (location = 0) in vec4 _pos;
layout (location = 1) in vec2 _uv;
layout (location = 2) flat in int _id;

layout (location = 0) out vec4 _color;

void main() {
    _color = texture(bb_tex[_id].handle, _uv);
    if (_color.a < 0.4) {
        discard;
    }
    else {
        float fog_const = 1.0;
        float fog_exp = _pos.z * fog_const;
        float fog_value = exp(-fog_exp * fog_exp);
        vec4 fog_color = vec4(PURPLE.xyz, 0.5);//vec4(0.749, 0.067, 0.706, 0.8f);
        _color = vec4(0, 0, 0, 1);
        _color = (1.0 - fog_value) * fog_color + fog_value * _color;
    }
}
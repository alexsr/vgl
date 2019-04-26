#version 460 core
#include "../api.glsl"

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;

layout(loc_gl(0) loc_vk(1, 0)) uniform sampler2D tex;

layout(location = 0) out vec4 out_color;

void main()
{
	out_color = color * texture(tex, uv);
}
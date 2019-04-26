#version 460 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 color;

out gl_PerVertex
{
	vec4 gl_Position;
};

layout(binding = 0) uniform Data
{
	mat4 projection;
};

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_color;

void main()
{
	gl_Position = projection * vec4(position, 0, 1);
	out_uv = uv;
	out_color = color;
}
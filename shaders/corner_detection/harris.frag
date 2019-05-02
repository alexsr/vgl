#version 460

layout (location = 0) in vec2 _uv;

layout (binding = 0) uniform sampler2D sobel;

layout (location = 0) uniform int kernel_size;
layout (location = 1) uniform float sensitivity;

layout (location = 0) out vec4 _color;

#include "../include/matrix_op.glsl"
#include "../include/math_consts.glsl"

void main() {
    ivec2 tex_size = textureSize(sobel, 0);
    vec2 uv_offset = vec2(1, 1) / vec2(tex_size);
	float half_size = (kernel_size - 1) / 2.0;
    float sigma = kernel_size / 6.0;
    float sigma2 = 2.0 * sigma * sigma;
    float denom = 1.0 / (PI * sigma2);
    mat2 A = mat2(0.0);
    for (float i = -half_size; i <= half_size; i++) {
        for (float j = -half_size; j <= half_size; j++) {
            float gauss = denom * exp(-(i * i + j * j) / sigma2);
            float Ix = texture(sobel, _uv + uv_offset * vec2(i, j)).x;
            float Iy = texture(sobel, _uv + uv_offset * vec2(i, j)).y;
            float Ixy = Ix * Iy;
            A += gauss * mat2(Ix * Ix, Ixy, Ixy, Iy * Iy);
        }
    }
    float trace_A = trace(A);
    float m_c = det(A) - sensitivity * trace_A * trace_A;
    _color = vec4(m_c);
}

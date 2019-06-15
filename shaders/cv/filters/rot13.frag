#version 460

layout (location = 0) in vec2 _uv;

layout (binding = 0) uniform sampler2D tex;
layout (location = 0) uniform int x;
layout (location = 1) uniform int type;
layout (location = 2) uniform mat3 hill_mat;
layout (location = 3) uniform int xor_value;

layout (std430, binding = 0) buffer onetimepad_ssbo {
    vec4 one_time_pad[];
};

layout (location = 0) out vec4 _color;

#include "../../include/color_conversion.glsl"

vec3 rot_x_hsv(vec3 color, float x) {
    vec3 hsv = rgb_to_hsv(color);
    hsv.r = hsv.r + x;
    if (hsv.r > 360.0) {
        hsv.r -= 360.0;
    }
    return hsv_to_rgb(hsv);
}

vec3 rot_x_rgb(vec3 color, float x) {
    vec3 temp = color + x / 256.0;
    if (temp.x > 1.0) {
        temp.x -= 1.0;
    }
    if (temp.y > 1.0) {
        temp.y -= 1.0;
    }
    if (temp.z > 1.0) {
        temp.z -= 1.0;
    }
    return temp;
}

vec3 onetimepad_rot(vec3 color, vec3 x) {
    vec3 temp = color + x;
    if (temp.x > 1.0) {
        temp.x -= 1.0;
    }
    if (temp.y > 1.0) {
        temp.y -= 1.0;
    }
    if (temp.z > 1.0) {
        temp.z -= 1.0;
    }
    if (temp.x < 0.0) {
        temp.x += 1.0;
    }
    if (temp.y < 0.0) {
        temp.y += 1.0;
    }
    if (temp.z < 0.0) {
        temp.z += 1.0;
    }
    return temp;
}

vec3 hill(vec3 color, mat3 mat) {
    vec3 temp = color * mat;
    while (temp.x > 1.0) {
        temp.x -= 1.0;
    }
    while (temp.y > 1.0) {
        temp.y -= 1.0;
    }
    while (temp.z > 1.0) {
        temp.z -= 1.0;
    }
    while (temp.x < 0.0) {
        temp.x += 1.0;
    }
    while (temp.y < 0.0) {
        temp.y += 1.0;
    }
    while (temp.z < 0.0) {
        temp.z += 1.0;
    }
    return temp;
}

vec3 xor_cypher(vec3 color, int xor) {
    ivec3 temp = ivec3(color * 255.0);
    return vec3(temp ^ xor) / 255.0;
}

vec3 bitrev(vec3 color) {
    uvec3 temp = uvec3(color * 255.0);
    uvec3 temp_copy = temp;
    uvec3 N = uvec3(1) << 8;
    for (uint i = 1; i < 8; i++) {
        temp >>= 1;
        temp_copy <<= 1;
        temp_copy |= temp & 1;
    }
    temp_copy &= N - 1;

    return vec3(temp_copy) / 255.0;
}

void main() {
    vec3 temp_color = texture(tex, _uv).rgb;
    if (type == 0) {
        _color = vec4(rot_x_hsv(temp_color, x), 1.0);
    }
    else if (type == 1) {
        _color = vec4(rot_x_rgb(temp_color, x), 1.0);
    }
    else if (type == 2) {
        _color = vec4(hill(temp_color, hill_mat), 1.0);
    }
    else if (type == 3) {
        _color = vec4(xor_cypher(temp_color, xor_value), 1.0);
    }
    else if (type == 4) {
        _color = vec4(bitrev(temp_color), 1.0);
    }
    else if (type == 5) {
        ivec2 tex_size = textureSize(tex, 0);
        ivec2 iuv = ivec2(tex_size * _uv);
        _color = vec4(onetimepad_rot(temp_color, one_time_pad[iuv.x * tex_size.y + iuv.y].rgb), 1.0);
    }
    else if (type == 6) {
        ivec2 tex_size = textureSize(tex, 0);
        ivec2 iuv = ivec2(tex_size * _uv);
        _color = vec4(onetimepad_rot(temp_color, 1.0 - one_time_pad[iuv.x * tex_size.y + iuv.y].rgb), 1.0);
    }
}

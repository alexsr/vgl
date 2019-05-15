#version 460

layout (lines) in;

#include "../include/cam.glsl"
#include "../include/bindings.glsl"

layout (std430, binding = CAM_BINDING) buffer cam_buffer {
    Camera cam;
};

layout (triangle_strip, max_vertices = 14) out;
void main() {
    vec3 min_vert = gl_in[0].gl_Position.xyz;
    vec3 max_vert = gl_in[1].gl_Position.xyz;
    gl_Position = cam.proj * cam.view * vec4(min_vert.x, max_vert.y, max_vert.z, 1.0); // 4
    EmitVertex();
    gl_Position = cam.proj * cam.view * vec4(max_vert.x, max_vert.y, max_vert.z, 1.0); // 3
    EmitVertex();
    gl_Position = cam.proj * cam.view * vec4(min_vert.x, min_vert.y, max_vert.z, 1.0); // 7
    EmitVertex();
    gl_Position = cam.proj * cam.view * vec4(max_vert.x, min_vert.y, max_vert.z, 1.0); // 8
    EmitVertex();
    gl_Position = cam.proj * cam.view * vec4(max_vert.x, min_vert.y, min_vert.z, 1.0); // 5
    EmitVertex();
    gl_Position = cam.proj * cam.view * vec4(max_vert.x, max_vert.y, max_vert.z, 1.0); // 3
    EmitVertex();
    gl_Position = cam.proj * cam.view * vec4(max_vert.x, max_vert.y, min_vert.z, 1.0); // 1
    EmitVertex();
    gl_Position = cam.proj * cam.view * vec4(min_vert.x, max_vert.y, max_vert.z, 1.0); // 4
    EmitVertex();
    gl_Position = cam.proj * cam.view * vec4(min_vert.x, max_vert.y, min_vert.z, 1.0); // 2
    EmitVertex();
    gl_Position = cam.proj * cam.view * vec4(min_vert.x, min_vert.y, max_vert.z, 1.0); // 7
    EmitVertex();
    gl_Position = cam.proj * cam.view * vec4(min_vert.x, min_vert.y, min_vert.z, 1.0); // 6
    EmitVertex();
    gl_Position = cam.proj * cam.view * vec4(max_vert.x, min_vert.y, min_vert.z, 1.0); // 5
    EmitVertex();
    gl_Position = cam.proj * cam.view * vec4(min_vert.x, max_vert.y, min_vert.z, 1.0); // 2
    EmitVertex();
    gl_Position = cam.proj * cam.view * vec4(max_vert.x, max_vert.y, min_vert.z, 1.0); // 1
    EmitVertex();
    EndPrimitive();
}
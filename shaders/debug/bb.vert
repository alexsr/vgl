#version 460

#include "../include/bindings.glsl"
#include "../include/bounds.glsl"

layout (std430, binding = BOUNDS_BINDING) buffer bounds_buffer {
    Bounds bounds[];
};

void main() {
    if (gl_VertexID == 0) {
        gl_Position = bounds[gl_InstanceID].min_bounds;
    }
    else {
        gl_Position = bounds[gl_InstanceID].max_bounds;
    }
}

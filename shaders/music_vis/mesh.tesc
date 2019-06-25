#version 460

#include "binding.glsl"

layout (vertices = 4) out;

layout (location = 0) in vec4 _pos_tc[];

layout (location = 0) out vec4 _pos_te[];

layout (std430, binding = VISUALIZATION_SETTINGS_BINDING) buffer vis_settings {
    vec2 mesh_size;
    ivec2 grid_res;
    ivec2 patch_size;
};

void main(void){
    if (gl_InvocationID == 0) {
        int patch_max = max(patch_size.x, patch_size.y);
        gl_TessLevelInner[0] = 2;//patch_max;// patch_size.y;
        gl_TessLevelInner[1] = 2;//patch_size.x;
        gl_TessLevelOuter[0] = 2;//patch_size.x;
        gl_TessLevelOuter[1] = 2;//patch_size.y;
        gl_TessLevelOuter[2] = 2;//patch_size.x;
        gl_TessLevelOuter[3] = 2;//patch_size.y;
    } 
    // gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    _pos_te[gl_InvocationID] = _pos_tc[gl_InvocationID];
    // _uv_te[gl_InvocationID] = _uv_tc[gl_InvocationID];
}
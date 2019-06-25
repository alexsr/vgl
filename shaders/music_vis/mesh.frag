#version 460

layout (early_fragment_tests) in;

#include "binding.glsl"
#include "colors.glsl"

layout (location = 0) in vec4 _pos;
layout (location = 1) in vec2 _uv;

layout (location = 0) out vec4 _color;

layout (location = 1) uniform float offset;

layout (std430, binding = VISUALIZATION_SETTINGS_BINDING) buffer vis_settings {
    vec2 mesh_size;
    ivec2 grid_res;
};

float grid( in vec2 p, in vec2 dpdx, in vec2 dpdy ) {
    const float N = 10.0; // grid ratio
    vec2 w = max(abs(dpdx), abs(dpdy));
    vec2 a = p + 0.5*w;
    vec2 b = p - 0.5*w;
    vec2 i = (floor(a)+min(fract(a)*N,1.0)-
              floor(b)-min(fract(b)*N,1.0))/(N*w);
    return (1.0-i.x)*(1.0-i.y);
}
// #define GRID_SIZE 
#define GRID_LINE_SIZE 2.0
float gridTexture(in vec2 uv, float GRID_SIZE)
{
    if(uv.y < 0.0)
    {
    	return 0.0;
    }
    float thickness = 0.1;
    
    float xPhase = mod(GRID_SIZE*uv.x, 1.0);
    float yPhase = mod(GRID_SIZE*uv.y, 1.0);
            
    float xIntensity = max(0.0, 1.0-abs(0.5-xPhase)/thickness);
    float yIntensity = max(0.0, 1.0-abs(0.5-yPhase)/thickness);
    
    return yIntensity+xIntensity;
}

void main() {
    float GRID_SIZE = 1./grid_res.x;
    vec2 uv = abs(mod(_uv.xy + vec2(0, offset) + GRID_SIZE/2.0, GRID_SIZE) - GRID_SIZE/2.0);
    uv /= fwidth(_uv.xy);
    float gln = min(min(uv.x, uv.y), 1.) / GRID_SIZE;
    vec4 ground_color = mix(DARK_BLUE, MAGENTA, 0.8 - smoothstep(0, 1, 2.0 * abs(_uv.x - 0.5)));
    ground_color.a = 1.0f;
    // vec4 ground_color = vec4(DARK_BLUE.xyz, 1.0);
    float fog_const = 1.0;
    float fog_exp = _pos.z * fog_const;
    float fog_value = exp(-fog_exp * fog_exp);
    vec4 fog_color = vec4(PURPLE.xyz, 0.8);//vec4(0.749, 0.067, 0.706, 0.8f);
    _color = (1.0 - fog_value) * fog_color + fog_value * ground_color;
    _color += fog_value * mix(vec4(0.0, 0.0, 0.0, 0.0), CYAN * 1.7, 0.6 - smoothstep(0.0, GRID_LINE_SIZE / GRID_SIZE, gln));
    // _color = normalize(_color);
    
    // _color = mix(ground_color, vec4(0.212, 0.804, 0.769, 1.0), gridTexture(_uv + vec2(0, offset), 1./GRID_SIZE));
    // _color = vec4(1, 0, 0, 1);
    
    // _color = // + 1.0f);
    // _color = (1. - grid(_uv * 64.0f + vec2(0, -offset / float(grid_res.y) * 64.0f), dFdx(_uv), dFdy(_uv) * 64.0f)) * vec4(1, 0, 0, 1.0);
    
}

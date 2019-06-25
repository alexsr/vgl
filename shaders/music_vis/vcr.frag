#version 460

#extension GL_ARB_bindless_texture : require

layout (early_fragment_tests) in;

#include "../include/math_consts.glsl"
#include "colors.glsl"

layout (location = 0) in vec2 _uv;

layout (location = 0, bindless_sampler) uniform sampler2D color_tex;
layout (location = 1, bindless_sampler) uniform sampler2D noise_tex;
layout (location = 2) uniform float time;
layout (location = 3) uniform float aspect_ratio;

layout (location = 0) out vec4 _color;

vec2 screenDistort(vec2 uv) {
	uv -= vec2(.5,.5);
	uv = uv*1.2*(1./1.2+2.*uv.x*uv.x*uv.y*uv.y);
	uv += vec2(.5,.5);
	return uv;
}

float scanline(float y ) {
	float scanlines = 160.0;
    return 0.3 * sin(y * scanlines * 2.0 * PI);
}

vec4 scene(in vec2 uv) {
    vec2 circle_uv = (2.0 * uv - 1.0) * vec2(aspect_ratio, 1.0);
    vec4 color = texture(color_tex, uv);
    vec4 bg_color = 1.3 * mix(CYAN, MAGENTA, clamp(uv.y * uv.y, 0, 1));
    float radius = 0.5;
    vec2 diff = vec2(0, 0.2) - circle_uv;
    float dist = length(diff) / radius;
    float dist_from_top = (radius + diff.y);
    bool stripes = !(dist_from_top > 0.59 && dist_from_top < 0.65) && !(dist_from_top > 0.71 && dist_from_top < 0.745)
     && !(dist_from_top > 0.78 && dist_from_top < 0.805) && !(dist_from_top > 0.825 && dist_from_top < 0.84)
     && !(dist_from_top > 0.86 && dist_from_top < 0.872);
    if (dist < 1.0 && color.a < 0.8 && stripes) {
        float sun_grad = exp(-dist_from_top * dist_from_top);
        return mix(2.0 * mix(vec4(1.0), MAGENTA, sun_grad), bg_color, smoothstep(0.0, 1.0, dist * dist * dist * dist));
    }
    else {
        return mix(bg_color, color, color.a);
    }
}

// vec4 flickering(float y) {
//     int flickers_count = int(texture(noise_tex, vec2(0.3, 0.44)).r * 3);
//     float range = 0.03;
//     float pos = flickers_count / 3.0 * y;

// }

void main() {
    // vec2 uv = screenDistort(_uv);
    vec2 uv = _uv;
    float cos_time = cos(time);
    float noise_r = texture(noise_tex, uv + vec2(cos_time * 2.3, sin(time * 0.312))).r * 0.001;
    float noise_g = texture(noise_tex, uv + vec2(cos(time * 35.0) * 6.7, time / 3.99)).r * 0.001;
    float noise_b = texture(noise_tex, uv + vec2(sin(time * 8.0) * 0.3, sin(time) * 9.0)).r * 0.001;
    vec2 r_offset = vec2(noise_r, noise_g);
    vec2 g_offset = vec2(noise_b, sin(noise_r) * 0.001);
    vec2 b_offset = vec2(noise_g, noise_b);
    _color.r = scene(uv + r_offset).r;
    _color.g = scene(uv + g_offset).g;
    _color.b = scene(uv + b_offset).b;
    // float noise = texture(noise_tex, vec2(1,(sin(time) + 1.0) / 2.0)).r;
    float additive_noise = texture(noise_tex,vec2(1.,2.*cos_time)*time*8. + uv*1.).r;
    _color += additive_noise * 0.07;// mod(uv + vec2(0, time * 100), vec2(1.0))).r * 0.1;
    _color += scanline(uv.y) * _color;
    // _color += flickering(uv.y);
    float vigAmt = 2.0+1.5*sin(0 + 5.*cos(1*5.));
	float vignette = (1.-vigAmt*(uv.y-.5)*(uv.y-.5))*(1.-vigAmt*(uv.x-.5)*(uv.x-.5));
    _color *= vignette;
}

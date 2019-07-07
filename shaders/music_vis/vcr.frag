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
layout (location = 4) uniform float chromatic_abboration = 1.0;

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

// vec4 flickering(float y) {
//     int flickers_count = int(texture(noise_tex, vec2(0.3, 0.44)).r * 3);
//     float range = 0.03;
//     float pos = flickers_count / 3.0 * y;

// }

void main() {
    // vec2 uv = screenDistort(_uv);
    vec2 uv = _uv;
    // float cos_time = cos(time / 100.0);
    // float noise_r = texture(noise_tex, uv + vec2(sin(time * 100.0), cos(time / 77.0))).r * 0.01;
    // // float noise_g = texture(noise_tex, uv + vec2(cos(time / 35.0) * 6.7, time / 33.99)).r * 0.001;
    // // float noise_b = texture(noise_tex, uv + vec2(sin(time / 8.0) * 0.3, sin(time / 1211.0) * 9.0)).r * 0.001;
    // vec2 r_offset = vec2(noise_r, noise_r);
    // // vec2 g_offset = vec2(noise_r, sin(noise_r) * 0.001);
    // // vec2 b_offset = vec2(noise_r, noise_r);
    // _color.r = scene(uv + r_offset).r;
    // _color.b = scene(uv + r_offset.yx).b;
    // _color.g = scene(uv).g;
    _color = texture(color_tex, uv);
    if (chromatic_abboration > 0.0) {
        float blur = 0.05 * chromatic_abboration / 2.5;
        vec2 blur_time = vec2(cos(time * 0.089), sin(time * 0.03124)) * blur;
        _color.r = texture( color_tex, uv + blur_time).r;
        _color.g = texture( color_tex, uv).g;
        _color.b = texture( color_tex, uv - blur_time ).b;
    }
    else {
        _color = texture(color_tex, uv);
    }
    // float noise = texture(noise_tex, vec2(1,(sin(time) + 1.0) / 2.0)).r;
    float additive_noise = texture(noise_tex,vec2(1.,2.*cos(time))*time*8. + uv*1.).r;
    _color += additive_noise * 0.03;// mod(uv + vec2(0, time * 100), vec2(1.0))).r * 0.1;
    _color += scanline(uv.y) * _color;
    // _color += flickering(uv.y);
    float vigAmt = 2.0+1.5*sin(0 + 5.*cos(1*5.));
	float vignette = (1.-vigAmt*(uv.y-.5)*(uv.y-.5))*(1.-vigAmt*(uv.x-.5)*(uv.x-.5));
    _color *= vignette;
}

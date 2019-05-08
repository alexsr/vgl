#ifndef LIGHT_GLSL
#define LIGHT_GLSL

struct Attenuation {
    float constant;
    float linear;
    float quadratic;
    float pad1;
};

struct Light {
    vec4 pos;
    vec4 color;
    vec4 dir;
    Attenuation attenuation;
    float outer_cutoff;
    float inner_cutoff;
    int type;

};

#endif // LIGHT_GLSL

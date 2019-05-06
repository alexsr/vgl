#ifndef LIGHT_GLSL
#define LIGHT_GLSL

struct Light {
    vec4 pos;
    vec4 color;
    vec4 dir;
    float radius;
    int type;
};

#endif // LIGHT_GLSL

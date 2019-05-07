#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

struct Material {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 emissive;
    vec4 reflective;
    vec4 transparent;
};

#endif // MATERIAL_GLSL

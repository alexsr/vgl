#ifndef CAM_GLSL
#define CAM_GLSL

struct Camera {
    mat3 rotation;
    vec4 pad;
    vec3 position;
    mat4 proj;
};

#endif // CAM_GLSL

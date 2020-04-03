#ifndef CAM_GLSL
#define CAM_GLSL

struct Camera {
    mat4 view;
    mat4 proj;
    mat4 inv_vp;
    vec4 position;
};

#endif // CAM_GLSL

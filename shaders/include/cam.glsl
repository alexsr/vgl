#ifndef CAM_GLSL
#define CAM_GLSL

struct Camera {
    mat4 view;
    vec3 position;
    mat4 proj;
    mat4 inv_vp;
};

#endif // CAM_GLSL

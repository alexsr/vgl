#ifndef SCENE_OBJECT_GLSL
#define SCENE_OBJECT_GLSL

struct Scene_object {
    mat4 model;
    int material_id;
    int texture_diffuse;
    int texture_specular;
    int texture_normal;
    int texture_height;
    int texture_emissive;
    int pad1;
    int pad2;
};

#endif // SCENE_OBJECT_GLSL

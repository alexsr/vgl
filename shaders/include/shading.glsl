#ifndef SHADING_GLSL
#define SHADING_GLSL

float phong_spec(vec3 light_dir, vec3 view_dir, vec3 normal, float shininess) {
    return pow(max(dot(reflect(light_dir, normal), view_dir), 0), shininess);
}

float blinn_phong_spec(vec3 light_dir, vec3 view_dir, vec3 normal, float shininess) {
    vec3 halfway_dir = normalize(light_dir + view_dir);
    return pow(max(dot(normal, halfway_dir), 0), shininess);
}

#endif // SHADING_GLSL

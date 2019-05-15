#version 460

layout (location = 0) in vec2 _uv;
layout (location = 1) in vec3 _ray_dir;

#include "../include/bindings.glsl"
#include "../include/light.glsl"
#include "../include/cam.glsl"
#include "../include/vertex.glsl"

layout (std430, binding = CAM_BINDING) buffer cam_buffer {
    Camera cam;
};

layout (std430, binding = LIGHTS_BINDING) buffer light_buffer {
    Light lights[];
};

layout (location = 0) out vec4 _color;

struct ray {
    vec3 dir;
    vec3 origin;
};

float hit_sphere(vec3 center, float radius, ray r){
vec3 oc = r.origin - center;
float a = dot(r.dir, r.dir);
float b = 2.0 * dot(oc, r.dir);
float c = dot(oc,oc) - radius*radius;
float discriminant = b*b - 4*a*c;
if(discriminant < 0.0){
return -1.0;
}
else{
float numerator = -b - sqrt(discriminant);
if (numerator > 0.0) {
return numerator / (2.0 * a);
}

numerator = -b + sqrt(discriminant);
if (numerator > 0.0) {
return numerator / (2.0 * a);
}
else {
return -1;
}

}
}

void main() {
    float t_min = 1.0 / 0.0;
    int hit = -1;
    _color.rgb = normalize(_ray_dir);
    for (int i = 0; i < lights.length(); ++i) {
        float t = hit_sphere(vec3(lights[i].pos.xyz), 1.0f, ray(normalize(_ray_dir), cam.position));
        if (t > 0 && t < t_min) {
            t_min = t;
            _color.rgb = lights[i].color.rgb;
            hit = i;
        }
    }
}

#version 460

layout (location = 0) in vec2 _uv;
layout (location = 1) in vec3 _ray_dir;

#include "../include/bindings.glsl"
#include "../include/cam.glsl"
#include "../include/light.glsl"
#include "../include/vertex.glsl"
#include "../include/scene_object.glsl"
#include "../include/material.glsl"

layout (std430, binding = CAM_BINDING) buffer cam_buffer {
    Camera cam;
};

struct Triangle {
    ivec3 idx;
    int obj_id;
};

layout (std430, binding = TRIANGLES_BINDING) buffer triangle_buffer {
    Triangle triangles[];
};

layout (std430, binding = VERTICES_BINDING) buffer vertex_buffer {
    Vertex vertices[];
};

layout (std430, binding = LIGHTS_BINDING) buffer light_buffer {
    Light lights[];
};

layout (std430, binding = SCENE_OBJECT_BINDING) buffer scene_object_buffer {
    Scene_object objects[];
};

layout (std430, binding = MATERIAL_BINDING) buffer material_buffer {
    Material materials[];
};

layout (location = 0) out vec4 _color;

struct ray {
    vec3 dir;
    vec3 origin;
};

bool rayTriangleIntersect(const ray r, Vertex v0, Vertex v1, Vertex v2, inout float t, inout float u, inout float v) {
    vec3 v0v1 = vec3(v1.pos - v0.pos);
    vec3 v0v2 = vec3(v2.pos - v0.pos);
    vec3 pvec = cross(r.dir, v0v2);
    float det = dot(v0v1, pvec);
    // if the determinant is negative the triangle is backfacing
    // if the determinant is close to 0, the ray misses the triangle
    if (abs(det) < 0.000001) return false;
    float invDet = 1.0 / det;

    vec3 tvec = r.origin - v0.pos.xyz;
    u = dot(tvec, pvec) * invDet;
    if (u < 0 || u > 1) return false;

    vec3 qvec = cross(tvec, v0v1);
    v = dot(r.dir, qvec) * invDet;
    if (v < 0 || u + v > 1) return false;
    t = dot(v0v2, qvec) * invDet;
    if (t > 0.000001) {
        return true;
    }
    else // This means that there is a line intersection but not a ray intersection.
        return false;
}

bool any_hit(const ray r, float t_near) {
    float u, v, t = 0.0;
    for (int i = 0; i < triangles.length(); ++i) {
        ivec3 idx = triangles[i].idx;
        if (rayTriangleIntersect(r, vertices[idx.x], vertices[idx.y], vertices[idx.z], t, u, v)) {
            if (t < t_near)
            return true;
        }
    }
    return false;
}

float blinn_phong_spec(vec3 light_dir, vec3 view_dir, vec3 normal, float shininess) {
    vec3 halfway_dir = normalize(light_dir + view_dir);
    return pow(max(dot(normal, halfway_dir), 0), shininess);
}

void main() {
    float t_min = 1.0 / 0.0;
    float t = 0.0;
    int hit = -1;
    vec3 n = vec3(0);
    _color.a = 1.0;
    for (int i = 0; i < triangles.length(); i++) {
        ivec3 idx = triangles[i].idx;
        float u = 0;
        float v = 0;
        bool intersect = rayTriangleIntersect(ray(normalize(_ray_dir), cam.position), vertices[idx.x], vertices[idx.y], vertices[idx.z], t, u, v);
        if (intersect) {
            if (t_min > t) {
                t_min = min(t_min, t);
                hit = i;
                n = normalize(vec3((1.0 - u - v) * vertices[idx.x].normal + u * vertices[idx.y].normal + v * vertices[idx.z].normal));
            }
        }
    }
    _color.rgb = vec3(0);
    if (hit != -1) {
        vec3 p = cam.position + t_min * normalize(_ray_dir);
        vec3 shadow_p = p + n * 0.0001;
        vec3 v = -normalize(_ray_dir);
        float shininess = 10.0f;
        for (int i = 0; i < lights.length(); i++) {
            vec3 pos_to_light = lights[i].pos.xyz - shadow_p;
            vec3 pos_to_light_norm = normalize(pos_to_light);
            if (any_hit(ray(pos_to_light_norm, shadow_p), length(pos_to_light))) {
                continue;
            }
            vec4 light_color = lights[i].color;
            
            float cos_phi = max(dot(pos_to_light_norm, n), 0.0);
            _color.rgb += light_color.rgb * cos_phi * materials[objects[triangles[hit].obj_id].material_id].diffuse.rgb;
        }
    }
}

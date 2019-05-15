#version 460

layout (location = 0) in vec2 _uv;
layout (location = 1) in vec3 _ray_dir;

#include "../include/bindings.glsl"
#include "../include/cam.glsl"
#include "../include/light.glsl"
#include "../include/vertex.glsl"

layout (std430, binding = CAM_BINDING) buffer cam_buffer {
    Camera cam;
};

struct Triangle {
    ivec3 idx;
    int pad;
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

layout (location = 0) out vec4 _color;

struct ray {
    vec3 dir;
    vec3 origin;
};

bool rayTriangleIntersect(const ray r, Vertex v0, Vertex v1, Vertex v2, inout float t) {
    vec3 v0v1 = vec3(v1.pos - v0.pos);
    vec3 v0v2 = vec3(v2.pos - v0.pos);
    vec3 pvec = cross(r.dir, v0v2);
    float det = dot(v0v1, pvec);
    // if the determinant is negative the triangle is backfacing
    // if the determinant is close to 0, the ray misses the triangle
    if (abs(det) < 0.000001) return false;
    float invDet = 1.0 / det;

    vec3 tvec = r.origin - v0.pos.xyz;
    float u = dot(tvec, pvec) * invDet;
    if (u < 0 || u > 1) return false;

    vec3 qvec = cross(tvec, v0v1);
    float v = dot(r.dir, qvec) * invDet;
    if (v < 0 || u + v > 1) return false;
    t = dot(v0v2, qvec) * invDet;
    if (t > 0.000001) {
        return true;
    }
    else // This means that there is a line intersection but not a ray intersection.
        return false;
}

void main() {
    float t_min = 1.0 / 0.0;
    float t = 0.0;
    int hit = -1;
    vec3 n = vec3(0);
    _color.a = 1.0;
    for (int i = 0; i < triangles.length(); i++) {
        ivec3 idx = triangles[i].idx;
        bool intersect = rayTriangleIntersect(ray(normalize(_ray_dir), cam.position), vertices[idx.x], vertices[idx.y], vertices[idx.z], t);
        if (intersect) {
            hit = i;
            t_min = min(t_min, t);
        }
    }
    _color.rgb = vec3(0);
    if (hit != -1) {
        vec3 p = cam.position + t_min * normalize(_ray_dir);
        _color.rgb = p;
    }
}

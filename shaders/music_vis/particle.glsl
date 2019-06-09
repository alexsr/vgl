#ifndef PARTICLE_GLSL
#define PARTICLE_GLSL

struct Particle {
    vec4 pos;
    vec4 vel;
    vec4 accel;
    vec3 color;
    float lifetime;
};

#endif // PARTICLE_GLSL

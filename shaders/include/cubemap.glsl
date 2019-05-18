#ifndef CUBEMAP_GLSL
#define CUBEMAP_GLSL

struct Cubemap_config {
    bool hdr;
    float gamma;
    float exposure;
    int tone_mapping;
};

#endif // CUBEMAP_GLSL

struct Billboard {
    vec2 pos;
    vec2 scale;
    float noise_offset;
    float height_noise;
    int pad1, pad2;
};

struct Billboard_texture {
    sampler2D handle;
    float aspect_ratio;
};
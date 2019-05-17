#ifndef DRAW_CMD_GLSL
#define DRAW_CMD_GLSL

struct Indirect_elements_command {
    uint count;
    uint instance_count;
    uint first_index;
    uint base_vertex;
    uint base_instance;
};

#endif // DRAW_CMD_GLSL

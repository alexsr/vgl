#pragma once

namespace vgl::gl {
    struct Indirect_elements_command {
        unsigned int count;
        unsigned int instance_count;
        unsigned int first_index;
        unsigned int base_vertex;
        unsigned int base_instance;
    };
}

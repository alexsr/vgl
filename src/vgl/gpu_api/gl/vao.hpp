#pragma once

#include "handle.hpp"

namespace vgl::gl {
    glvertexarray create_vertex_array() {
        GLuint va = 0;
        glCreateVertexArrays(1, &va);
        return glvertexarray(va);
    }
}

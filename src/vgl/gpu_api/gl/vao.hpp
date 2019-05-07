#pragma once

#include "glad/glad.h"

namespace vgl
{
    namespace gl
    {
        GLuint create_vertex_array() {
            GLuint va = 0;
            glCreateVertexArrays(1, &va);
            return va;
        }

        void delete_vertex_array(GLuint va) {
            if (glIsVertexArray(va)) {
                glDeleteVertexArrays(1, &va);
            }
        }
    }
}

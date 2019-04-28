#pragma once

#include "glad/glad.h"

namespace vgl
{
    namespace gl
    {
        GLuint create_texture(GLenum target) {
			GLuint texture = 0;
			glCreateTextures(target, 1, &texture);
			return texture;
        }
    }
}

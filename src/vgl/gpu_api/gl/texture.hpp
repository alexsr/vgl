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

        GLuint64 gen_resident_handle(const GLuint texture) {
            const auto handle = glGetTextureHandleARB(texture);
            glMakeTextureHandleResidentARB(handle);
            return handle;
        }

    }
}

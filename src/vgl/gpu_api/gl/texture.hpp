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

        void delete_texture(GLuint texture) {
            if (glIsTexture(texture)) {
                const auto handle = glGetTextureHandleARB(texture);
                if (glIsTextureHandleResidentARB(handle)) {
                    glMakeTextureHandleNonResidentARB(handle);
                }
                glDeleteTextures(1, &texture);
            }
        }

        GLuint64 get_texture_handle(const GLuint texture) {
            const auto handle = glGetTextureHandleARB(texture);
            if (!glIsTextureHandleResidentARB(handle)) {
                glMakeTextureHandleResidentARB(handle);
            }
            return handle;
        }

    }
}

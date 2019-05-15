#pragma once

#include "handle.hpp"

namespace vgl::gl
{
    gltexture create_texture(GLenum target) {
        GLuint texture = 0;
        glCreateTextures(target, 1, &texture);
        return texture;
    }

    GLuint64 get_texture_handle(const GLuint texture) {
        const auto handle = glGetTextureHandleARB(texture);
        if (!glIsTextureHandleResidentARB(handle)) {
            glMakeTextureHandleResidentARB(handle);
        }
        return handle;
    }
}

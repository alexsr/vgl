#pragma once

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/gtc/quaternion.hpp"

namespace vgl {
    struct Cam_data {
        glm::mat4 view{};
        glm::vec4 position{};
        glm::mat4 projection{};
        glm::mat4 inv_vp{};
    };

    struct Camera {
        Cam_data get_cam_data();
        glm::mat4 get_rotation();
        glm::vec4 get_position();
        void move(glm::vec3 dir, float dt);
        void rotate(glm::vec2 angles, float dt);
        void reset();
        void change_aspect_ratio(float aspect_ratio);
        void change_fovy(float fovy);

        glm::vec3 position{ 0.0f, 0.0f, 1.0f };
        glm::quat rotation{ 1, 0, 0, 0 };
        glm::mat4 projection;
        float move_speed = 1.0f;
        float rotation_speed = 3.0f;
    private:
        glm::vec2 _angles{}; 
    };
}
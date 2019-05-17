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
        inline Cam_data get_cam_data() {
            Cam_data data;
            data.view = glm::translate(get_rotation(), -position);
            data.position = get_position();
            data.projection = projection;
            data.inv_vp = glm::inverse(projection * get_rotation());
            return data;
        }
        inline glm::mat4 get_rotation() {
            return glm::mat4_cast(rotation);
        }
        inline glm::vec4 get_position() {
            return glm::vec4(position, 1.0f);
        }
        inline void move(glm::vec3 dir, float dt) {
            position += glm::conjugate(rotation) * dir * dt * move_speed;
        }
        inline void rotate(glm::vec2 angles, float dt) {
            _angles += angles * dt * rotation_speed;
            rotation = glm::normalize(glm::quat(glm::vec3(_angles.y, 0, 0)) * glm::quat(glm::vec3(0, _angles.x, 0)));
        }
        inline void reset() {
            position = glm::vec3{ 0.0f, 0.0f, 1.0f };
            rotation = glm::quat{ 1, 0, 0, 0 };
            _angles = glm::vec3(0.0f);
        }
        inline void change_aspect_ratio(float aspect_ratio) {
            projection[0][0] = projection[1][1] / aspect_ratio;
        }
        inline void change_fovy(float fovy) {
            auto old = projection[1][1];
            auto tan_half = glm::tan(fovy / 2.0f);
            projection[0][0] = projection[0][0] / old / tan_half;
            projection[1][1] = 1.0f / tan_half;
        }
        glm::vec3 position{ 0.0f, 0.0f, 1.0f };
        glm::quat rotation{ 1, 0, 0, 0 };
        glm::mat4 projection;
        float move_speed = 1.0f;
        float rotation_speed = 3.0f;
    private:
        glm::vec2 _angles{}; 
    };
}
#include "camera.hpp"

vgl::Cam_data vgl::Camera::get_cam_data() {
    Cam_data data;
    data.view = glm::translate(get_rotation(), -position);
    data.position = get_position();
    data.projection = projection;
    data.inv_vp = glm::inverse(projection * get_rotation());
    return data;
}

glm::mat4 vgl::Camera::get_rotation() {
    return glm::mat4_cast(rotation);
}

glm::vec4 vgl::Camera::get_position() {
    return glm::vec4(position, 1.0f);
}

void vgl::Camera::move(glm::vec3 dir, float dt) {
    position += glm::conjugate(rotation) * dir * dt * move_speed;
}

void vgl::Camera::rotate(glm::vec2 angles, float dt) {
    _angles += angles * dt * rotation_speed;
    rotation = glm::normalize(glm::quat(glm::vec3(_angles.y, 0, 0)) * glm::quat(glm::vec3(0, _angles.x, 0)));
}

void vgl::Camera::reset() {
    position = glm::vec3{ 0.0f, 0.0f, 1.0f };
    rotation = glm::quat{ 1, 0, 0, 0 };
    _angles = glm::vec3(0.0f);
}

void vgl::Camera::change_aspect_ratio(float aspect_ratio) {
    projection[0][0] = projection[1][1] / aspect_ratio;
}

void vgl::Camera::change_fovy(float fovy) {
    auto old = projection[1][1];
    auto tan_half = glm::tan(fovy / 2.0f);
    projection[0][0] = projection[0][0] / old / tan_half;
    projection[1][1] = 1.0f / tan_half;
}

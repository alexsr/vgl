#include "camera.hpp"

vgl::Camera_controller::Camera_controller() {
    move_callback = [](Eigen::Vector3d&) { return false; };
    rotate_callback = [](Eigen::Vector2d&) { return false; };
}

vgl::Camera_controller::Camera_controller(move_callback_func const& move_func, rotate_callback_func const& rotate_func)
    : move_callback(move_func), rotate_callback(rotate_func) {}

void vgl::Camera_controller::update_camera(GLCamera& cam, double dt) {
    Eigen::Vector2d angles;
    if (rotate_callback(angles)) {
        rotate(cam, angles, dt);
    }
    Eigen::Vector3d dir;
    if (move_callback(dir)) {
        move(cam, dir, dt);
    }
}

void vgl::Camera_controller::set_move_callback(move_callback_func const& func) { move_callback = func; }
void vgl::Camera_controller::set_rotate_callback(rotate_callback_func const& func) { rotate_callback = func; }
void vgl::Camera_controller::set_move_speed(double const move_speed) { move_speed_ = move_speed; }
void vgl::Camera_controller::set_rotate_speed(double const rotate_speed) { rotate_speed_ = rotate_speed; }

vgl::Fly_through_controller::Fly_through_controller() : Camera_controller() {}

vgl::Fly_through_controller::Fly_through_controller(move_callback_func const& move_func,
                                                    rotate_callback_func const& rotate_func)
    : Camera_controller(move_func, rotate_func) {}

void vgl::Fly_through_controller::reset() { }

void vgl::Fly_through_controller::move(GLCamera& cam, Eigen::Vector3d const& dir, double dt) {
    curr_pos_ += cam.pose.rotation.conjugate() * dir * dt * move_speed_;
    cam.pose.translation = cam.pose.rotation * -curr_pos_;
}

void vgl::Fly_through_controller::rotate(GLCamera& cam, Eigen::Vector2d const& angles, double dt) {
    angles_ += angles * dt * rotate_speed_;
    cam.pose.rotation = (Eigen::AngleAxisd(angles_.x(), Eigen::Vector3d::UnitX()) *
                         Eigen::AngleAxisd(angles_.y(), Eigen::Vector3d::UnitY()));
    cam.pose.translation = cam.pose.rotation * -curr_pos_;
}

vgl::Cam_data vgl::GLCamera::get_cam_data() {
    Cam_data data;
    data.view = view_f();
    data.position = pose.translation.homogeneous().cast<float>();
    data.position.w() = 1.0f;
    data.projection = proj_f();
    data.inv_vp = (proj_f() * view_f()).inverse();
    return data;
}

Eigen::Matrix4d vgl::GLCamera::view() { return pose.to_matrix(); }
Eigen::Matrix4f vgl::GLCamera::view_f() { return view().cast<float>(); }
Eigen::Matrix4d vgl::GLCamera::proj() { return projection; }
Eigen::Matrix4f vgl::GLCamera::proj_f() { return projection.cast<float>(); }

void vgl::GLCamera::reset() {
    pose = Pose();
}

vgl::GLCamera::GLCamera() {
    reset();
    projection = perspective_projection<double>(1.0, 90.0, 0.1, 100.0);
}

void vgl::GLCamera::change_aspect_ratio(double aspect_ratio) { vgl::change_aspect_ratio(projection, aspect_ratio); }

void vgl::GLCamera::change_fovy(double fovy) { vgl::change_fovy(projection, fovy); }

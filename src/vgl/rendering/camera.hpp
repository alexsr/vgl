#pragma once

#include <functional>
#include <memory>
#include "vgl/math/pose.hpp"
#include "vgl/math/projection.hpp"
#undef near
#undef far

namespace vgl {
struct Cam_data {
    Eigen::Matrix4f view{};
    Eigen::Matrix4f projection{};
    Eigen::Matrix4f inv_vp{};
    Eigen::Vector4f position{};
};

class GLCamera {
public:
    GLCamera();
    Cam_data get_cam_data();
    Eigen::Matrix4d view();
    Eigen::Matrix4f view_f();
    Eigen::Matrix4d proj();
    Eigen::Matrix4f proj_f();
    void reset();
    void change_aspect_ratio(double aspect_ratio);
    void change_fovy(double fovy);

    Pose pose{};
    Eigen::Matrix4d projection;
};

class Camera_controller {
public:
    using move_callback_func = std::function<bool(Eigen::Vector3d&)>;
    using rotate_callback_func = std::function<bool(Eigen::Vector2d&)>;
    Camera_controller();
    Camera_controller(move_callback_func const& move_func, rotate_callback_func const& rotate_func);
    void update_camera(GLCamera& cam, double dt);
    void set_move_callback(move_callback_func const& func);
    void set_rotate_callback(rotate_callback_func const& func);
    void set_move_speed(double const move_speed);
    void set_rotate_speed(double const rotate_speed);
    virtual void reset() = 0;

protected:
    virtual void move(GLCamera& cam, Eigen::Vector3d const& dir, double dt) = 0;
    virtual void rotate(GLCamera& cam, Eigen::Vector2d const& angles, double dt) = 0;
    double move_speed_ = 1.0;
    double rotate_speed_ = 1.0;

private:
    move_callback_func move_callback;
    rotate_callback_func rotate_callback;
};

class Fly_through_controller : public Camera_controller {
public:
    Fly_through_controller();
    Fly_through_controller(move_callback_func const& move_func, rotate_callback_func const& rotate_func);
    void reset();

protected:
    Eigen::Vector2d angles_ = Eigen::Vector2d::Zero();
    Eigen::Vector3d curr_pos_ = Eigen::Vector3d::Zero();
    virtual void move(GLCamera& cam, Eigen::Vector3d const& dir, double dt);
    virtual void rotate(GLCamera& cam, Eigen::Vector2d const& angles, double dt);
};
} // namespace vgl

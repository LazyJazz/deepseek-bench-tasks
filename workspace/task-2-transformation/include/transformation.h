#pragma once

#include <Eigen/Eigen>

Eigen::Matrix4f Translate(const Eigen::Vector3f &offset);
Eigen::Matrix4f Rotate(const Eigen::Vector3f &axis, float radians);
Eigen::Matrix4f LookAt(const Eigen::Vector3f &eye, const Eigen::Vector3f &center,
                       const Eigen::Vector3f &up);
Eigen::Matrix4f Perspective(float fov_y, float aspect, float near_z, float far_z);

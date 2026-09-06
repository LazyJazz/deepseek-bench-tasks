#include "transformation.h"

#include <cmath>

Eigen::Matrix4f Translate(const Eigen::Vector3f &offset) {
  Eigen::Matrix4f result = Eigen::Matrix4f::Identity();
  result.block<3, 1>(0, 3) = offset;
  return result;
}

Eigen::Matrix4f Rotate(const Eigen::Vector3f &axis, float radians) {
  const Eigen::Vector3f unit = axis.normalized();
  Eigen::Matrix3f cross;
  cross << 0.0f, -unit.z(), unit.y(),
           unit.z(), 0.0f, -unit.x(),
           -unit.y(), unit.x(), 0.0f;
  const float cosine = std::cos(radians);
  Eigen::Matrix4f result = Eigen::Matrix4f::Identity();
  result.block<3, 3>(0, 0) =
      cosine * Eigen::Matrix3f::Identity() +
      (1.0f - cosine) * unit * unit.transpose() + std::sin(radians) * cross;
  return result;
}

Eigen::Matrix4f LookAt(const Eigen::Vector3f &eye,
                       const Eigen::Vector3f &center,
                       const Eigen::Vector3f &up) {
  const Eigen::Vector3f forward = (center - eye).normalized();
  const Eigen::Vector3f right = forward.cross(up).normalized();
  const Eigen::Vector3f camera_up = right.cross(forward);
  Eigen::Matrix4f result = Eigen::Matrix4f::Identity();
  result.block<1, 3>(0, 0) = right.transpose();
  result.block<1, 3>(1, 0) = camera_up.transpose();
  result.block<1, 3>(2, 0) = -forward.transpose();
  result(0, 3) = -right.dot(eye);
  result(1, 3) = -camera_up.dot(eye);
  result(2, 3) = forward.dot(eye);
  return result;
}

Eigen::Matrix4f Perspective(float fov_y, float aspect, float near_z, float far_z) {
  const float inverse_tangent = 1.0f / std::tan(fov_y * 0.5f);
  Eigen::Matrix4f result = Eigen::Matrix4f::Zero();
  result(0, 0) = inverse_tangent / aspect;
  result(1, 1) = inverse_tangent;
  result(2, 2) = far_z / (near_z - far_z);
  result(2, 3) = far_z * near_z / (near_z - far_z);
  result(3, 2) = -1.0f;
  return result;
}

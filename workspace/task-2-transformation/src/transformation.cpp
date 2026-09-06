#include "transformation.h"

Eigen::Matrix4f Translate(const Eigen::Vector3f &) { return Eigen::Matrix4f::Identity(); }
Eigen::Matrix4f Rotate(const Eigen::Vector3f &, float) { return Eigen::Matrix4f::Identity(); }
Eigen::Matrix4f LookAt(const Eigen::Vector3f &, const Eigen::Vector3f &,
                       const Eigen::Vector3f &) {
  return Eigen::Matrix4f::Identity();
}
Eigen::Matrix4f Perspective(float, float, float, float) {
  return Eigen::Matrix4f::Identity();
}

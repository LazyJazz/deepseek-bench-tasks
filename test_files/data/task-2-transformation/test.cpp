#include "transformation.h"
#include "test_support.h"

#include <cmath>
#include <fstream>
#include <string>

bool Near(float a, float b, float epsilon = 1e-5f) {
  return std::abs(a - b) <= epsilon;
}
void ExpectVector(const Eigen::Vector4f &actual, const Eigen::Vector4f &expected,
                  float epsilon = 1e-5f) {
  for (int i = 0; i < 4; ++i) CHECK(Near(actual[i], expected[i], epsilon));
}
void ExpectMatrix(const Eigen::Matrix4f &actual, const Eigen::Matrix4f &expected,
                  float epsilon = 1e-2f) {
  for (int row = 0; row < 4; ++row)
    for (int column = 0; column < 4; ++column)
      CHECK(Near(actual(row, column), expected(row, column), epsilon));
}
std::ifstream Fixture(const char *name) {
  std::ifstream file(std::string(FIXTURE_DIR) + "/" + name, std::ios::binary);
  CHECK(file.is_open());
  return file;
}
template <class T> T Read(std::ifstream &file) {
  T value;
  CHECK(bool(file.read(reinterpret_cast<char *>(&value), sizeof(value))));
  return value;
}

int main(int argc, char **argv) {
  return Run([&] {
    CHECK(argc == 2);
    std::string group = argv[1];
    if (group == "translate") {
      auto matrix = Translate({3.0f, -4.0f, 2.0f});
      ExpectVector(matrix * Eigen::Vector4f(1, 2, 3, 1), {4, -2, 5, 1});
      ExpectVector(matrix * Eigen::Vector4f(1, 2, 3, 0), {1, 2, 3, 0});
      auto file = Fixture("translation.data");
      for (int i = 0; i < 10; ++i) {
        auto offset = Read<Eigen::Vector3f>(file);
        auto expected = Read<Eigen::Matrix4f>(file);
        ExpectMatrix(Translate(offset), expected);
      }
    } else if (group == "rotate") {
      const float pi = std::acos(-1.0f);
      auto matrix = Rotate({0, 0, 7}, pi / 2);
      ExpectVector(matrix * Eigen::Vector4f(1, 0, 0, 1), {0, 1, 0, 1});
      Eigen::Vector3f axis(1, 2, 3), value(1.5f, -2.0f, .25f);
      auto rotated = (Rotate(axis, -.731f) * Eigen::Vector4f(value.x(), value.y(), value.z(), 0)).head<3>();
      CHECK(Near(rotated.squaredNorm(), value.squaredNorm()));
      auto normalized = axis.normalized();
      ExpectVector(Rotate(axis, -.731f) * Eigen::Vector4f(normalized.x(), normalized.y(), normalized.z(), 0),
                   {normalized.x(), normalized.y(), normalized.z(), 0});
      auto file = Fixture("rotation.data");
      for (int i = 0; i < 10; ++i) {
        auto fixture_axis = Read<Eigen::Vector3f>(file);
        auto angle = Read<float>(file);
        auto expected = Read<Eigen::Matrix4f>(file);
        ExpectMatrix(Rotate(fixture_axis, angle), expected);
      }
    } else if (group == "lookat") {
      Eigen::Vector3f eye(3, 4, 5), center(-1, 2, -2), up(.2f, 1, .3f);
      auto matrix = LookAt(eye, center, up);
      ExpectVector(matrix * Eigen::Vector4f(eye.x(), eye.y(), eye.z(), 1), {0, 0, 0, 1});
      auto target = matrix * Eigen::Vector4f(center.x(), center.y(), center.z(), 1);
      CHECK(Near(target.x(), 0) && Near(target.y(), 0) && target.z() < 0);
      auto right = (center - eye).normalized().cross(up);
      auto transformed = matrix * Eigen::Vector4f(right.x(), right.y(), right.z(), 0);
      CHECK(transformed.x() > 0 && Near(transformed.y(), 0) && Near(transformed.z(), 0));
      auto file = Fixture("lookat.data");
      for (int i = 0; i < 10; ++i) {
        auto fixture_eye = Read<Eigen::Vector3f>(file);
        auto fixture_center = Read<Eigen::Vector3f>(file);
        auto expected = Read<Eigen::Matrix4f>(file);
        ExpectMatrix(LookAt(fixture_eye, fixture_center, {0, 1, 0}), expected);
      }
    } else if (group == "perspective") {
      const float pi = std::acos(-1.0f);
      const float cases[][4] = {{pi / 2, 1, .1f, 100}, {.7f, 16.0f / 9, 1, 11}};
      for (const auto &values : cases) {
        auto matrix = Perspective(values[0], values[1], values[2], values[3]);
        auto near_point = matrix * Eigen::Vector4f(0, 0, -values[2], 1);
        auto far_point = matrix * Eigen::Vector4f(0, 0, -values[3], 1);
        CHECK(Near(near_point.z() / near_point.w(), 0));
        CHECK(Near(far_point.z() / far_point.w(), 1));
        auto top = matrix * Eigen::Vector4f(0, std::tan(values[0] / 2) * values[2], -values[2], 1);
        CHECK(Near(top.y() / top.w(), 1));
      }
      auto file = Fixture("perspective.data");
      float fov[] = {10, 40, 70, 100, 130}, aspect[] = {4.0f/3, 16.0f/9, 1, 21.0f/9, 9.0f/16};
      float near_z[] = {.1f, 1, .1f, 10, 100}, far_z[] = {10, 10000, .2f, 1000, 110};
      for (int i = 0; i < 5; ++i) {
        auto expected = Read<Eigen::Matrix4f>(file);
        ExpectMatrix(Perspective(fov[i] * pi / 180, aspect[i], near_z[i], far_z[i]), expected);
      }
    } else {
      CHECK(false);
    }
  });
}

#pragma once

#include <cstddef>
#include <vector>

#include <glm/glm.hpp>

enum class MaterialType { kLambertian, kSpecular };
struct Material { glm::vec3 albedo; MaterialType type{MaterialType::kLambertian}; };
struct Sphere { glm::vec3 center; float radius{}; Material material; };
struct Triangle { glm::vec3 v0, v1, v2; Material material; };
struct PointLight { glm::vec3 position; glm::vec3 power; };
struct Ray { glm::vec3 origin; glm::vec3 direction; };
struct HitRecord {
  float t{};
  glm::vec3 point;
  glm::vec3 normal;
  Material material;
};
struct Scene {
  std::vector<Sphere> spheres;
  std::vector<Triangle> triangles;
  std::vector<PointLight> lights;
  glm::vec3 ambient{0.0f};
};

bool IntersectSphere(const Ray &ray, const Sphere &sphere, float t_min,
                     float t_max, HitRecord &hit);
bool IntersectTriangle(const Ray &ray, const Triangle &triangle, float t_min,
                       float t_max, HitRecord &hit);
bool CastRay(const Scene &scene, const Ray &ray, float t_min, float t_max,
             HitRecord &hit);
glm::vec3 CalculateLighting(const Scene &scene, const HitRecord &hit);
glm::vec3 SampleRay(const Scene &scene, Ray ray, std::size_t max_bounces = 8);

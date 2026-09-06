#include "raytracer.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
constexpr float kEpsilon = 1e-3f;
void FaceForward(const Ray &ray, HitRecord &hit, const glm::vec3 &outward) {
  hit.normal = glm::dot(ray.direction, outward) < 0.0f ? outward : -outward;
}
}

bool IntersectSphere(const Ray &input, const Sphere &sphere, float t_min,
                     float t_max, HitRecord &hit) {
  const Ray ray{input.origin, glm::normalize(input.direction)};
  const glm::vec3 offset = ray.origin - sphere.center;
  const float half_b = glm::dot(offset, ray.direction);
  const float discriminant = half_b * half_b - glm::dot(offset, offset) +
                             sphere.radius * sphere.radius;
  if (discriminant < 0.0f) return false;
  const float square_root = std::sqrt(discriminant);
  float root = -half_b - square_root;
  if (root < t_min || root > t_max) {
    root = -half_b + square_root;
    if (root < t_min || root > t_max) return false;
  }
  hit.t = root;
  hit.point = ray.origin + root * ray.direction;
  FaceForward(ray, hit, (hit.point - sphere.center) / sphere.radius);
  hit.material = sphere.material;
  return true;
}

bool IntersectTriangle(const Ray &input, const Triangle &triangle, float t_min,
                       float t_max, HitRecord &hit) {
  const Ray ray{input.origin, glm::normalize(input.direction)};
  const glm::vec3 edge1 = triangle.v1 - triangle.v0;
  const glm::vec3 edge2 = triangle.v2 - triangle.v0;
  const glm::vec3 p = glm::cross(ray.direction, edge2);
  const float determinant = glm::dot(edge1, p);
  if (std::abs(determinant) < 1e-7f) return false;
  const float inverse = 1.0f / determinant;
  const glm::vec3 from_vertex = ray.origin - triangle.v0;
  const float u = glm::dot(from_vertex, p) * inverse;
  if (u < 0.0f || u > 1.0f) return false;
  const glm::vec3 q = glm::cross(from_vertex, edge1);
  const float v = glm::dot(ray.direction, q) * inverse;
  if (v < 0.0f || u + v > 1.0f) return false;
  const float distance = glm::dot(edge2, q) * inverse;
  if (distance < t_min || distance > t_max) return false;
  const glm::vec3 outward = glm::cross(edge1, edge2);
  if (glm::dot(outward, outward) < 1e-14f) return false;
  hit.t = distance;
  hit.point = ray.origin + distance * ray.direction;
  FaceForward(ray, hit, glm::normalize(outward));
  hit.material = triangle.material;
  return true;
}

bool CastRay(const Scene &scene, const Ray &ray, float t_min, float t_max,
             HitRecord &hit) {
  bool found = false;
  HitRecord candidate;
  float closest = t_max;
  for (const auto &sphere : scene.spheres) {
    if (IntersectSphere(ray, sphere, t_min, closest, candidate)) {
      found = true;
      closest = candidate.t;
      hit = candidate;
    }
  }
  for (const auto &triangle : scene.triangles) {
    if (IntersectTriangle(ray, triangle, t_min, closest, candidate)) {
      found = true;
      closest = candidate.t;
      hit = candidate;
    }
  }
  return found;
}

glm::vec3 CalculateLighting(const Scene &scene, const HitRecord &hit) {
  glm::vec3 illumination = scene.ambient;
  for (const auto &light : scene.lights) {
    const glm::vec3 delta = light.position - hit.point;
    const float distance = glm::length(delta);
    if (distance <= kEpsilon) continue;
    const glm::vec3 direction = delta / distance;
    const float cosine = std::max(0.0f, glm::dot(hit.normal, direction));
    if (cosine == 0.0f) continue;
    HitRecord blocker;
    if (CastRay(scene, {hit.point + kEpsilon * hit.normal, direction},
                kEpsilon, distance - kEpsilon, blocker)) continue;
    illumination += light.power * (cosine / (distance * distance));
  }
  return hit.material.albedo * illumination;
}

glm::vec3 SampleRay(const Scene &scene, Ray ray, std::size_t max_bounces) {
  glm::vec3 throughput(1.0f);
  for (std::size_t bounce = 0; bounce < max_bounces; ++bounce) {
    ray.direction = glm::normalize(ray.direction);
    HitRecord hit;
    if (!CastRay(scene, ray, kEpsilon, std::numeric_limits<float>::infinity(), hit))
      return throughput * scene.ambient;
    if (hit.material.type == MaterialType::kLambertian)
      return throughput * CalculateLighting(scene, hit);
    throughput *= hit.material.albedo;
    ray = {hit.point + kEpsilon * hit.normal,
           glm::reflect(ray.direction, hit.normal)};
  }
  return glm::vec3(0.0f);
}

#include "raytracer.h"

bool IntersectSphere(const Ray &, const Sphere &, float, float, HitRecord &) { return false; }
bool IntersectTriangle(const Ray &, const Triangle &, float, float, HitRecord &) { return false; }
bool CastRay(const Scene &, const Ray &, float, float, HitRecord &) { return false; }
glm::vec3 CalculateLighting(const Scene &, const HitRecord &) { return {}; }
glm::vec3 SampleRay(const Scene &, Ray, std::size_t) { return {}; }

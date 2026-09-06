# Task 4 — CPU Ray Tracing

在 `src/raytracer.cpp` 中完成纯 C++ CPU ray tracer。向量类型使用 `glm::vec3`，公开场景类型位于 `include/raytracer.h`，只修改实现文件。

- 所有 ray direction 均按归一化方向处理。
- 球求交返回 `[t_min,t_max]` 内最近根，支持从球内发出的射线。
- 三角形双面可见；退化或平行时不命中。返回法线必须与入射方向相反。
- `CastRay` 在全部球和三角形中返回最近命中。
- Lambertian 表面颜色为 `albedo * (ambient + sum(power/d^2 * max(dot(n,l),0)))`。被任何几何体遮挡的灯没有贡献；阴影射线必须避免自相交。
- Specular 表面产生理想反射，吞吐量逐次乘以 albedo。首次 Lambertian 命中返回吞吐量乘其光照；逃逸返回吞吐量乘 ambient；达到 `max_bounces` 返回黑色。
- `max_bounces=0` 直接返回黑色。计算不主动 clamp 颜色。

测试既直接调用几何和着色函数，也由 CPU 渲染固定场景并与参考图比较；不创建窗口、不编译 shader。需要 C++17，无 GPU、Vulkan 或 HLSL；GLM 和可用于图片输出的 stb 由 `../external/` 下的 submodule 提供。

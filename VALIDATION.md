# Graphics benchmark 验证记录

验证日期：2026-09-06。环境：macOS arm64、AppleClang 17.0.0、CMake 3.31.6、Python 3。

## 任务

- `task-1-gol`：原地 Game of Life。
- `task-2-transformation`：平移、轴角旋转、LookAt 和右手透视矩阵。
- `task-3-rasterization`：纯 CPU 像素、线、矩形、圆和三角形光栅化。
- `task-4-raytracing`：纯 C++ CPU 球/三角形求交、最近命中、阴影、Lambert 光照和镜面反射。

任务不需要 GPU、Vulkan、LongMarch 或 HLSL，也不创建窗口。Eigen、GLM 和 stb 以固定提交的 submodule 放在 `workspace/external/`。Transformation 和 Rasterization 使用固定回归数据；Ray tracing 在 CPU 上渲染固定场景并与参考图比较。PNG 同时保存为预解码 RGBA 副本。

Transformation 的公开接口直接使用 `Eigen::Vector3f/Matrix4f`；Ray tracing 的向量、场景和求交计算直接使用 `glm::vec3`。自定义 `Mat4`、`Vec3` 及配套向量运算已删除。Rasterization 只使用 `Pixel/Image`，没有额外数学向量层。

## 验证结果

从 `eval/` 运行统一入口，参考实现的 22 个测试组全部通过：

```json
{"resolved": true, "score": 1.0, "reason": "all tests passed"}
```

`main` 起始实现可以完整编译，21 个有权重的功能组全部失败，仅 GOL 的零权重内存策略检查通过，得到 `resolved: false, score: 0.0`。

所有参考实现还使用 `-Wall -Wextra -Wpedantic -Werror` 独立构建通过。评分器的 21 项维护者测试通过，覆盖多任务部分得分、0.8 阈值、GOL 0.6 限分、单题构建失败、CTest XML、超时、日志和结果格式。

测试执行层使用 CPU C++ 程序，覆盖四组矩阵数据、矩形输入、四张光栅化参考图和 ray tracing 参考图。临时可视化测试使用 stb 输出 `local_output/transformation.png`、`rasterization.png` 和 `raytracing.png`；该目录已由 Git 忽略。

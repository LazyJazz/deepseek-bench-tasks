# Task 2 — 3D Transformation

实现 `src/transformation.cpp` 中的平移、轴角旋转、观察和透视矩阵。接口使用 `Eigen::Vector3f` 和 `Eigen::Matrix4f`，矩阵左乘列向量并使用右手坐标系。只修改该实现文件。

- `Translate` 对点施加给定偏移，对方向向量无影响。
- `Rotate` 的轴保证非零但不保证已归一化；正角度遵循右手定则。
- `LookAt` 返回 world-to-camera 矩阵。相机看向负 z，正 y 为上，正 x 为右；`eye != center`，且 up 不与视线平行。
- `Perspective` 的参数满足 `0<fov_y<pi`、`aspect>0`、`0<near_z<far_z`。近平面映射到 NDC z=0，远平面映射到 z=1，相机前方为负 z。

结果使用单精度浮点数，回归测试误差容许范围为 `1e-2`。需要 C++17，无 GPU；Eigen 由 `../external/eigen` submodule 提供。

评测除性质与边界检查外，还会读取 `translation.data`、`rotation.data`、`lookat.data` 和 `perspective.data` 中的 Eigen 单精度回归样例。

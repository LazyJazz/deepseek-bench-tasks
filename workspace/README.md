# C++ Agent Benchmark Tasks

| 目录 | 任务 |
| --- | --- |
| [task-1-gol](task-1-gol/README.md) | Game of Life 原地更新 |
| [task-2-transformation](task-2-transformation/README.md) | 3D transformation matrices |
| [task-3-rasterization](task-3-rasterization/README.md) | CPU rasterization |
| [task-4-raytracing](task-4-raytracing/README.md) | CPU ray tracing |

所有任务使用 C++17 和 CMake 3.16+，不依赖图形系统或 GPU。Eigen、GLM 和 stb 位于 `external/`；克隆仓库后可用 `git submodule update --init --recursive` 初始化。在当前目录可离线编译全部起始代码：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

请保持公开接口，只修改各任务 `task.json` 中列出的文件。测试数据、测试脚本与评分不属于工作目录。

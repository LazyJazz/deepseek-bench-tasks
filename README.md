# C++ Graphics Agent Benchmarks

四个离线 C++17 benchmark，任务、可信测试和评测输出分别位于 `workspace/`、`test_files/data/` 和 `eval/`。

| 任务 | 内容 |
| --- | --- |
| `task-1-gol` | Game of Life 原地更新 |
| `task-2-transformation` | 3D transformation matrices |
| `task-3-rasterization` | CPU rasterization |
| `task-4-raytracing` | CPU ray tracing |

任务无需窗口、GPU、Vulkan 或 HLSL。Eigen、GLM 和 stb 以固定版本的 submodule 提供在 `workspace/external/`，无需由 agent 下载。需要 CMake 3.16+、C++17 编译器和 Python 3.8+。

## 统一评测

```sh
cd eval
python3 test_by_code.py
```

结果写入 `eval/code_result.json`，只包含 `resolved`、`score`、`reason`。四题等权，任务内按测试组加权；功能总分达到 0.8 时 `resolved` 为 true。GOL 若开辟新的临时内存区域，最终 `score` 最高为 0.6，但功能正确时 `resolved` 仍为 true。

评分器从 `test_files/data/` 读取只读测试，为每题建立新的临时构建目录，并把日志写入 `eval/task-*/`。评分不执行候选工作区的 CMake 文件。

## 分支

- `main`：可编译的起始任务。
- `test`：通过同一套可信测试的参考答案。

维护者测试：

```sh
python3 -m unittest discover -s test_files/data/tests -v
```

直接运行全部 C++ 测试：

```sh
cmake -S test_files/data -B build-tests -DCMAKE_BUILD_TYPE=Release
cmake --build build-tests --parallel
ctest --test-dir build-tests --output-on-failure
```

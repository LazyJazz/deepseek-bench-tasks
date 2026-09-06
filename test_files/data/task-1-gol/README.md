# task-1-gol 测试说明

对应 `workspace/task-1-gol/`。从仓库的 `eval/` 运行 `python3 test_by_code.py` 统一评分。

测试覆盖经典图案、边界、任意非零存活值、全部 512 种 3×3 邻域、固定历史回归数据和大网格。
固定数据覆盖 10×10、15×25、25×15、40×40；缓冲区哨兵检查越界写入。
`gol.no_allocation` 在 `update_step` 调用期间拦截全局 `new/new[]`，并检查源码中的常见 C 分配函数、动态容器和辅助数组；失败会把最终分数限制为 0.6，但不改变功能正确时的 resolved。

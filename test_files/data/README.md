# 可信测试和统一评分

本目录对应 `workspace/` 中的四个任务，评测期间只读。以 `eval/` 为工作目录运行 `python3 test_by_code.py`。

四题各占总分 25%，每题测试组权重见 `grading.json`。测试组全部通过才获得该组权重：

```text
task_score = sum(passed_group_weights) / sum(all_group_weights)
functional_score = average(task_scores)
resolved = (functional_score >= 0.8)
score = round(min(functional_score, applicable_score_caps), 6)
```

`gol.no_allocation` 权重为 0；失败时将 score 限制为 0.6，不改变功能正确时的 resolved。配置或编译失败时该题得 0 分，其余任务继续运行。

测试组共 22 个：GOL 7 个，Transformation 4 个，Rasterization 5 个，Ray tracing 6 个。图形任务使用固定的二进制和参考图数据，并通过预解码 RGBA 副本离线读取。C++ 测试均在 CPU 上运行。

评分器读取 CTest XML。构建、CTest 报告和日志均生成在 `eval/`，不会写入本目录。

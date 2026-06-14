# AI Report Edition 变更说明

新增功能：根据预计就餐人数导出 AI 决策报告。

## 新增文件

- `include/utils/DecisionReportGenerator.h`
- `src/utils/DecisionReportGenerator.cpp`
- `docs/AI_DECISION_REPORT.md`

## 修改/实现点

- `main.cpp` 新增 `--people` 参数：输入预计就餐人数。
- `main.cpp` 新增 `--report` 参数：输出 Markdown 决策报告。
- `StatisticsLogger` 提供汇总指标、CSV 导出和报告所需数据。
- `DecisionReportGenerator` 根据仿真结果判断窗口瓶颈、座位瓶颈和流失风险。
- 报告末尾自动生成可复制给 AI 的提示词。

## 验证

已用以下流程验证：

```bash
cmake -S . -B build -DBDSS_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/BDSS --config resources/default_config.json --output simulation_log.csv --people 3500 --report decision_report.md
```

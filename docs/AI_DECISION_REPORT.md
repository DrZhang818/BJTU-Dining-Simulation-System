# AI 决策报告功能说明

本源码包新增了“根据预计人数输出 AI 决策报告”的功能。

## 入口

```bash
./build/BDSS \
  --config resources/default_config.json \
  --output simulation_log.csv \
  --people 3500 \
  --report decision_report.md
```

## 输出

- `simulation_log.csv`：逐秒仿真数据，可用于画图。
- `decision_report.md`：自动生成的 Markdown 决策报告。

## 报告包含

1. 场景信息。
2. 核心指标。
3. 窗口、座位、流失风险瓶颈判断。
4. 短期、中期、长期调整建议。
5. 按预计人数放大后的窗口数、座位数、外带比例参考值。
6. 可直接复制给 AI 的分析提示词。

## 建议工作流

1. 修改配置或命令行参数。
2. 运行仿真并导出 CSV 和报告。
3. 用 `tools/plot_metrics.py` 生成趋势图。
4. 将报告、CSV、趋势图一起交给 AI，让 AI 生成面向决策者的最终调整方案。

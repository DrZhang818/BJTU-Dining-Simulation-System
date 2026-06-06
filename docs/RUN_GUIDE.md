# 运行指南

## 1. 命令行快速运行

```bash
cmake -S . -B build -DBDSS_BUILD_GUI=OFF
cmake --build build --parallel
./build/BDSS --headless --config resources/default_config.json --output simulation_log.csv
```

## 2. 运行 10 个窗口场景

```bash
./build/BDSS --headless \
  --config resources/scenarios/high_capacity_10_windows.json \
  --output high_capacity_10_windows.csv
```

## 3. 运行座位压力场景

```bash
./build/BDSS --headless \
  --config resources/scenarios/low_seat_pressure.json \
  --output low_seat_pressure.csv
```

## 4. 图形界面

安装 Qt6 Widgets 后：

```bash
cmake -S . -B build -DBDSS_BUILD_GUI=ON
cmake --build build --parallel
./build/BDSS --gui --config resources/default_config.json
```

使用建议：

1. 在参数页修改参数。
2. 点击“应用参数”，确认状态变成“内核已同步”。
3. 点击“开始”运行。
4. 若运行中需要修改参数，先暂停，再修改，再点击“应用参数”。
5. 需要重新开始一轮实验时，点击“重置”。

## 5. 测试

```bash
ctest --test-dir build --output-on-failure
```

核心回归测试会验证新增窗口确实进入排队逻辑。

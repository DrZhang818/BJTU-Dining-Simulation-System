# 优化报告

## 1. 已修复的问题

用户反馈：“参数修改后前端显示修改了，但内部逻辑没有跟上变化。我把餐口数增加两个后根本没人去那两个新增窗口排队。”

根因是旧实现容易出现三类不同步：

1. UI 控件中的 `windowCount` 已改变，但已创建的 `SimulationEngine` 仍持有旧 `Config`。
2. 内核中的 `windows_` 向量没有跟随 `windowCount` 扩容，因此路由函数只能在旧窗口集合里选择。
3. 新增窗口的 `category/efficiency/profile` 没有统一归一化，导致 UI 看起来有新窗口，但模型层没有完整窗口对象。

本版处理方式：

- 增加 `Config::normalize()`，自动补齐窗口类型、窗口 profile、学生类型等派生配置。
- 增加 `SimulationEngine::updateConfig(const Config&, bool preserveRuntimeState)`，应用参数时直接同步内核配置。
- 增加 `SimulationEngine::rebuildWindowsForConfig()`，增加餐口时保留旧窗口状态并创建新窗口；减少餐口时把被移除窗口中的学生重新路由到保留窗口。
- GUI 的“应用参数”按钮不再只是刷新前端，而是调用内核同步接口。
- 增加回归测试 `LiveConfigUpdateAddsRoutableWindows`，验证运行中从 4 个窗口增加到 6 个窗口后，第 5、第 6 个窗口会收到学生。

## 2. 核心逻辑优化

### 餐口路由

`chooseWindowIndex()` 现在基于实际 `windows_.size()` 遍历窗口，而不是依赖 UI 或旧配置快照。评分考虑：

- 当前队列长度和服务中学生；
- 窗口效率；
- 学生偏好命中加分；
- 服务人数作为平局打散项。

### 外带逻辑

外带学生会乘以 `packingServiceFactor` 增加服务时间，完成服务后直接离开，不占用座位。

### 座位逻辑

座位分配考虑：

- 与所选窗口的相对位置；
- 群体就餐时相邻座位加分；
- 陌生人邻座惩罚；
- 清洁中的座位不可用。

### 配置兼容性

配置读取支持当前字段，同时兼容部分旧字段，例如：

- `enablePreferences` -> `enableWindowPreferences`
- `enablePacking` -> `enableTakeaway`
- `packingRatio` -> `takeawayRate`
- `queueLengthWeight` -> `queueWeight`
- `groupRatio` -> `groupProbability`
- `windowProximityWeight` -> `nearWindowWeight`

## 3. UI 优化建议与已落地部分

根据视频观察，旧界面的主要问题是参数区密度偏高，用户容易误以为“数值变了就立即生效”。本版已经加入：

- 顶部状态提示：`参数待应用` / `内核已同步`。
- 明确的“应用参数”按钮。
- 应用参数后，状态栏提示“新增餐口已参与排队路由”。
- 实时餐口卡片显示：窗口名称、类别、效率、排队人数、已服务人数。
- 座位矩阵显示：空座、本科生、研究生、教职工、清洁中。
- 趋势曲线显示：排队人数、等座人数、座位利用率。

后续还可以继续增强：

- 为 `windowProfiles` 增加可编辑表格，允许每个窗口设置名称、类别、效率。
- 给每个参数增加 tooltip，解释单位和合理范围。
- 增加“保存当前界面参数为 JSON”和“从 JSON 导入到界面”的对称流程。
- 增加一键对比实验：例如 8 窗口 vs 10 窗口、外带率 0.1 vs 0.3。

## 4. 测试覆盖

当前核心测试包括：

- 默认配置读取；
- 配置归一化；
- 非法配置拒绝；
- 初始增加窗口后新增窗口接收学生；
- 运行中增加窗口后新增窗口接收学生；
- 完整仿真不变量；
- 外带学生不占座；
- CSV 导出包含每个窗口指标。

运行方式：

```bash
cmake -S . -B build -DBDSS_BUILD_GUI=OFF
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## 5. 已知限制

- 运行中减少餐口时，移除窗口中的学生会重新进入保留窗口的队列；这能保证学生不丢失，但会近似处理已经服务了一部分的学生。
- 运行中缩小座位矩阵时，超出新矩阵范围的座位状态无法保留。建议在暂停或重置后调整座位规模。
- 当前仓库没有接入第三方 JSON 库，项目内置了轻量 JSON 解析器，足够配置文件使用；若后续配置结构更复杂，建议改用 `nlohmann/json` 或 Qt 的 `QJsonDocument`。

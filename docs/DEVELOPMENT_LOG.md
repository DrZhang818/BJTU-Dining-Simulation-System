# BDSS 集成阶段开发日志

> 时间范围：2026/6/6 20:36 — 2026/6/7 00:06
>
> 核心目标：将 BDSS 从基础食堂离散事件仿真升级为贴合真实场景的"就餐偏好 + Phase 2 扩展功能"系统

---

## 阶段 1：产品升级计划制定

**用户需求**：做一个详细的计划来指导产品升级，至少要加上就餐人偏好设置等贴合实际的改进。

**交付物**：

| 文档 | 内容 |
|---|---|
| `docs/产品升级计划_阶段1_就餐偏好系统.md` | 阶段 1 详细计划，含 5 项验收标准 |
| 口头规划（对话中） | 确认 3 项核心约束：启用 Qt Charts、可配置画像比例、严格向后兼容 |

**决策要点**：

- 采用**渐进式架构**：v2.0 基础内核不破坏 → v2.1 加偏好层 → Phase 2 加耐心/打包/清洁/结伴/座位偏好
- 严格向后兼容：`enablePreferences=false` 且 `seed=42` 时结果与 v2.0 逐行等价
- 代码集中维护在 `集成阶段/BDSS-Optimized/` 目录，不污染原始代码

---

## 阶段 2：就餐偏好系统（Phase 1 核心功能）

### 2.1 就餐者画像与偏好

**设计**：三类画像（本科生/研究生/教职工），可配置比例和就餐时长倍率；学生按评分选择窗口。

**修改文件**：

| 文件 | 改动 |
|---|---|
| `include/core/Config.h` | 新增 `ProfileType`、`enablePreferences`、ratio/diningFactor 等字段 |
| `src/core/Config.cpp` | `validate()` / `loadFromFile()` / `toString()` 同步更新 |
| `include/core/Student.h` | 新增 `profile_`、`preferredCategory_` 字段 |
| `include/core/Window.h` | 新增 `category_` 字段 |
| `src/core/SimulationEngine.cpp` | 新增 `chooseWindowWithPreference()`、修改 `generateArrivals()` |
| `include/utils/RandomGenerator.h` | 新增 `getWeightedChoice()` |

### 2.2 统计图表 Tab 与 CSV 导出

**设计**：Qt Charts 绘制三条曲线（排队总人数 / 等座人数 / 座位利用率），双 Y 轴。

**修改文件**：

| 文件 | 改动 |
|---|---|
| `CMakeLists.txt` | 依赖 `Qt6::Charts`，增加编译宏 `BDSS_HAS_CHARTS` |
| `include/gui/MainWindow.h` | 新增 `QChartView*`、`QChart*`、`summaryPanel_` |
| `src/gui/MainWindow.cpp` | 实现 `buildChartsTab()`、`updateCharts()`、`exportCsv()` |

### 2.3 GUI 配置页改写

`buildConfigTab()` 增加画像比例、就餐倍率、选窗偏好参数的 SpinBox/CheckBox，布局改为三列。

### 2.4 渲染器支持画像颜色

`src/gui/RenderWidget.cpp` 增加 `profileColor()` 函数，座位矩阵按画像类型着色。

### 2.5 测试用例补充

`tests/test_core.cpp` 新增：

- ProfileType 序列化与解析测试
- 学生画像/偏好字段测试
- 窗口品类测试
- 偏好关闭回归测试（`seed=42` 结果一致）
- 偏好开启冒烟测试
- 品类数量不匹配校验测试

---

## 阶段 3：GUI Bug 修复

### Bug 1：已离开学生始终为 0，已完成学生为 0

**根因**：`StatisticsLogger::record()` 中 `summary_.finishedStudents` 用 `std::max` 覆盖式更新，而 GUI 路径未调用 `finalize()`。

**修复**：

1. 在 `SimulationEngine` 中新增 `int getFinishedStudentCount() const noexcept`，直接返回 `finishedStudents_.size()`
2. GUI `refreshLiveStats()` 改用 `engine_->getFinishedStudentCount()` 直接获取
3. `SimulationEngine::tick()` 末尾加入 `finalize()` 调用（`isFinished()` 时调用一次）
4. 增加 `bool finalized_` 防止重复调用

### Bug 2：等座人数 900 秒后下降但窗口人数始终为 0

**根因**：同上，GUI 显示的是 `StatisticsLogger` 的数据而非引擎实时数据。

**修复**：同 Bug 1 修复一并解决。

### Bug 3：修改参数后 GUI 崩溃（windowCategories 不匹配）

**根因**：`readConfigFromForm()` 未同步调整 `windowCategories` 大小。用户把 `windowCount` 改为非 10 后，`Config::validate()` 抛出异常。

**修复**：`readConfigFromForm()` 中根据 `windowCount` 自动 `resize` `windowCategories`。

---

## 阶段 4：Phase 2 四大功能

### 4.1 排队耐心模型

| 文件 | 改动 |
|---|---|
| `Config.h` | 新增 `enablePatience`、`patienceBase`、`patienceStddev`、`patienceAllowSwitch` |
| `Student.h` | 新增 `patience_`/`patienceLeft_` 字段（后简化为只保留 `patienceLeft_`）、`setPatience()`、`tickPatience()`、`isImpatient()` |
| `Window.h/.cpp` | 队列从 `std::queue` 改为 `std::deque`；新增 `collectImpatient()`（遍历队列、递减耐心、返回放弃学生）；`reEnqueue()` 直接入队不重置状态 |
| `SimulationEngine.cpp` | `tick()` 中调用 `collectImpatient()`，支持换队（选最短的其他窗口）或放弃（`droppedStudents_++`） |

### 4.2 打包/堂食模式

| 文件 | 改动 |
|---|---|
| `Config.h` | 新增 `enablePacking`、`packingRatio`、`packingServiceFactor` |
| `Student.h` | 新增 `isTakeaway_` 字段 + getter/setter |
| `SimulationEngine.cpp` | `generateArrivals()` 中按 `packingRatio` 随机分配打包学生；`tick()` 中打包学生完成打饭后立即离开（跳过 WaitingForSeat/Dining） |

### 4.3 座位翻台/清洁机制

| 文件 | 改动 |
|---|---|
| `Config.h` | 新增 `enableCleaning`、`cleaningTime` |
| `Canteen.h` | `Seat` 结构体新增 `cleaningLeft` 字段（清洁倒计时）；析构函数新增 `cleaningTime` 参数 |
| `Canteen.cpp` | `tick()` 中清洁倒计时递减；学生离座后设 `startCleaning` |

### 4.4 结伴就餐 + 相邻座位分配

| 文件 | 改动 |
|---|---|
| `Config.h` | 新增 `enableGroupDining`、`groupRatio`、`groupSizeMin/Max` |
| `Student.h` | 新增 `groupId_` 字段 |
| `Canteen.h/.cpp` | 新增 `tryGroupSeat()`：先尝试同行相邻列，搜索不到则降级为个人就座 |
| `SimulationEngine.cpp` | `generateArrivals()` 中按比例生成组；`tick()` 中等座分配先处理结伴组 |

---

## 阶段 5：代码整合与简化

**目标**：合并新旧代码目录，清理冗余代码。

### 5.1 根目录同步

```bash
# 将 BDSS-Optimized 构建产物同步到根目录
cp 集成阶段/BDSS-Optimized/build/BDSS build/BDSS
cp 集成阶段/BDSS-Optimized/include/core/*.h include/core/
```

### 5.2 代码精简（~80 行 + 1 个字段）

| 优化项 | 说明 |
|---|---|
| Getter/setter 内联到头文件 | Student 的 14 个、Window 的 6 个、Canteen 的 4 个、SimulationEngine 的 6 个 getter 写入 `.h`，删掉 `.cpp` 中实现 |
| 删除冗余 `patience_` 字段 | `patience_` 和 `patienceLeft_` 语义重复，只保留后者 |
| `generateArrivals()` 拆子函数 | 提取 `createStudentWithPreferences()` 和 `assignDiningGroups()`，主函数从 ~100 行 6 层嵌套减至 ~25 行 3 层 |
| JSON 解析精简 | 合并 `readNumber`/`readPositive` 为同一模板 |
| RenderWidget 座位渲染修复 | 删除 `(r*cols+c) < occupiedSeats` 假渲染，改用 `Canteen::isSeatOccupied()` |
| Canteen 新增 `isSeatOccupied()` | 供 GUI 渲染直接查询真实座位状态 |

---

## 阶段 6：座位偏好选择系统

**设计**：三项加权评分选座替代贪心扫描。

### 评分公式

```
score(seat) = 1.5 * dist(seat, 打饭窗口)
            + 2.0 * dist(seat, 已就座组员质心)
            + 3.0 * adjacentStrangers(seat)
```

| 偏好 | 权重 | 说明 |
|---|---|---|
| 窗口就近 | 1.5 | 曼哈顿距离，窗口虚拟位于矩阵上方（row=-1），列位置由窗口编号映射 |
| 结伴邻桌 | 2.0 | 到已就座组员行列均值的距离；降级个人就座时用到 |
| 陌生人距离感 | 3.0 | 周围 8 格中被非同组成员占用的数量（0~8），默认权重最高 |

### 修改文件

| 文件 | 改动 |
|---|---|
| `Config.h` | 新增 `enableSeatPreference`、`windowProximityWeight`、`groupProximityWeight`、`isolationWeight` |
| `Student.h` | 新增 `servedByWindowId_`（记录打饭窗口编号，用于计算就近距离） |
| `Canteen.h/.cpp` | 构造接受 3 个权重；`findEmptySeat()` → `findBestSeat()` 评分算法；`trySeat`/`tryGroupSeat` 增加 `windowVirtualCol` 参数 |
| `SimulationEngine.cpp` | `tick()` 中记录 `servedByWindowId`，计算窗口虚拟列传入 Canteen |
| `MainWindow.h/.cpp` | 配置页新增座位偏好分组（1 个 CheckBox + 3 个 SpinBox） |

---

## 阶段 7：高峰期多段配置

**用户需求**：最高峰 12:00-12:15 之后 5 分钟间隔，12:20-12:40 第二小高峰。

### 架构变更

```cpp
// 旧：三个独立字段
int rushHourStart = 600;
int rushHourEnd = 2700;
double rushHourMultiplier = 2.0;

// 新：多段结构
struct RushPeak { int start; int end; double multiplier; };
std::vector<RushPeak> rushPeaks = {
    {3600, 4500, 2.5},   // 12:00-12:15 最高峰
    {4800, 6000, 1.8}    // 12:20-12:40 第二小高峰
};
```

### GUI 优化：QSpinBox → QTimeEdit

高峰时间控件从"输入秒数"改为 **HH:MM 时间选择器**，基准 11:00 = 仿真 0 秒。

### 修改文件

| 文件 | 改动 |
|---|---|
| `Config.h` | 替换 3 个旧字段为 `vector<RushPeak>` 结构 |
| `Config.cpp` | `loadFromFile` 解析 JSON array；`validate` 遍历每段；`arrivalRatePerSecond` 遍历叠加 |
| `MainWindow.h` | 旧 3 个 SpinBox → 新 6 个 QTimeEdit/DoubleSpinBox |
| `MainWindow.cpp` | 新增 `timeToSeconds()`/`secondsToTime()` 工具函数；高峰面板改为两个 HH:MM 时段 |
| `default_config.json` | 同步更新 JSON schema |

---

## 阶段 8：参数调优与验证

### 默认参数最终值

| 参数 | 值 | 说明 |
|---|---|---|
| `windowCount` | 8 | 中等食堂规模 |
| `tableRows × tableCols` | 10 × 10 | 100 个座位 |
| `totalSimulationTime` | 6600s | 11:00—12:50 共 110 分钟 |
| `arrivalRate` | 6.0 人/分钟 | 基础人流量 |
| `avgServiceTime` | 25s | 含打包等待影响 |
| `avgDiningTime` | 900s (15min) | 实际就餐时长 |
| `rushPeaks[0]` | 12:00-12:15 × 2.5 | 最高峰 |
| `rushPeaks[1]` | 12:20-12:40 × 1.8 | 第二小高峰 |
| `patienceBase` | 150s | 排队耐心 |
| `packingRatio` | 30% | 打包比例 |
| `cleaningTime` | 10s | 清洁时长 |
| `groupRatio` | 20% | 结伴比例 |
| `seatPreference` | 三项权重 1.5/2.0/3.0 | 座位偏好 |

### 最终仿真结果

```
完成学生:     869 人
最大排队长度:  14 人
最大等座人数:  31 人
平均排队等待:  11.6 秒
平均等座等待:  45.5 秒
平均座位利用率: 62.6%
```

### 测试覆盖率（12 项全部通过）

1. 学生状态切换与时间计算
2. 窗口排队和服务完成
3. 餐桌分配、就餐推进和座位释放
4. 配置解析和非法配置校验
5. 随机数范围和带权选择
6. 完整仿真流程（含安全限制检查）
7. CSV 导出
8. 画像类型序列化/解析
9. 学生画像/偏好字段
10. 窗口品类
11. 偏好关闭回归测试
12. 偏好开启冒烟 + 品类校验

---

## 文件变更总览

### 核心库（BDSSCore）

| 文件 | 行数变化 | 说明 |
|---|---|---|
| `include/core/Config.h` | +40 | ProfileType、RushPeak、Phase 2 全字段 |
| `src/core/Config.cpp` | +60 | validate/loadFromFile/toString 全字段同步 |
| `include/core/Student.h` | +15 | 画像、耐心、打包、结伴、窗口编号字段（内联 getter） |
| `src/core/Student.cpp` | −80 | getter 移到头文件，删除冗余函数 |
| `include/core/Window.h` | +8 | 品类字段、内联 getter、deque |
| `src/core/Window.cpp` | −15 | 内联后删除 |
| `include/core/Canteen.h` | +10 | Seat 清洁字段、评分权重参数 |
| `src/core/Canteen.cpp` | +40 | findBestSeat 评分算法、countAdjacentStrangers |
| `include/core/SimulationEngine.h` | +5 | 新方法声明 |
| `src/core/SimulationEngine.cpp` | +20 | 偏好创建、分组、座位偏好传参 |
| `include/utils/RandomGenerator.h` | +1 | getWeightedChoice |
| `src/utils/RandomGenerator.cpp` | +25 | 带权选择实现 |
| `include/utils/StatisticsLogger.h` | — | 无变化 |
| `src/utils/StatisticsLogger.cpp` | — | 无变化 |

### GUI

| 文件 | 说明 |
|---|---|
| `include/gui/MainWindow.h` | +Phase 2 控件指针（~20 个） |
| `src/gui/MainWindow.cpp` | 配置页三列→四列；高峰 QTimeEdit；图表 Tab；座位偏好分组 |
| `include/gui/RenderWidget.h` | 无变化 |
| `src/gui/RenderWidget.cpp` | 座位渲染改用 isSeatOccupied；增加画像颜色 |

### 构建

| 文件 | 说明 |
|---|---|
| `CMakeLists.txt` | 全局 AUTOMOC，Qt6::Charts 依赖 |

### 测试

| 文件 | 说明 |
|---|---|
| `tests/test_core.cpp` | 13 个测试用例（新增 6 个） |

### 文档

| 文件 | 说明 |
|---|---|
| `docs/DEVELOPMENT_LOG.md` | 本文档 |
| `集成阶段/BDSS-Optimized/docs/OPTIMIZATION_SUMMARY.md` | 阶段 1/2 优化说明同步更新 |
| `集成阶段/BDSS-Optimized/docs/RUN_GUIDE.md` | 快速运行指南 |
| `docs/产品升级计划_阶段1_就餐偏好系统.md` | 验收清单全部标记为已完成 |
| `docs/软件综合实训_24281082_王政_集成阶段实训报告.md` | 完成状态同步 |

---

## 技术债务 / 待改进

- [ ] 座位偏好评分中的 `adjacentStrangers()` 当前扫描 8 个方向，复杂度与空位数量 O(rows × cols × 8)，在大规模矩阵下可预计算邻座占用缓存
- [ ] `RushPeak` JSON array 解析使用正则，在大 JSON 文件下性能可改用轻量 JSON 库
- [ ] 结伴就餐降级个人就座时，可进一步优化组员质心实时更新

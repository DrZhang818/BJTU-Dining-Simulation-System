# 模型假设

## 时间单位

仿真内部使用秒。`totalSimulationTime`、`avgServiceTime`、`avgDiningTime`、`cleaningTime`、`avgPatienceTime` 都以秒为单位。

`rushPeaks.start` 和 `rushPeaks.end` 表示“仿真开始后的秒数”。例如：

- `900`：第 15 分钟；
- `2400`：第 40 分钟。

## 学生到达

每秒生成学生数服从泊松分布。基础到达率为 `arrivalRate / 60`，其中 `arrivalRate` 单位为人/分钟。

如果启用 `RushPeaks`，当前仿真时刻落在高峰段内时，到达率乘以该高峰段的 `multiplier`。多个高峰段重叠时会连续相乘，因此配置时应尽量避免重叠。

## 服务窗口

窗口效率越高，同样基础服务时间下实际服务越快。窗口路由评分由队列长度、窗口效率和偏好命中共同决定。

新增窗口时，`Config::normalize()` 会自动补齐窗口 profile；`SimulationEngine::updateConfig()` 会把新增窗口加入内核路由集合。

## 学生类型

默认学生类型包括本科生、研究生和教职工。不同类型可以设置：

- 出现比例；
- 就餐时间系数；
- 耐心系数。

## 外带

外带学生由 `takeawayRate` 决定。外带会增加打包服务时间，倍率为 `packingServiceFactor`。外带学生完成服务后直接离开，不进入找座或就餐阶段。

## 耐心与换队

如果启用耐心机制，排队等待超过耐心阈值的学生会触发一次处理：

- 以 `queueSwitchProbability` 的概率重新选择窗口并换队；
- 否则离队，计入 dropped students。

## 座位与清洁

非外带学生服务结束后进入等座队列。座位选择考虑靠近服务窗口、同组相邻和陌生人间隔。学生吃完后，如果启用清洁机制，座位进入清洁状态，清洁结束后重新可用。

## 随机性与复现

`randomSeed` 控制随机数生成。相同配置和相同种子下，命令行仿真结果应可复现。

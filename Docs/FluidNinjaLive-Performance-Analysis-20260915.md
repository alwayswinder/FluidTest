# FluidNinjaLive 性能优化复测报告

## 1. 结论摘要

本轮使用优化后重新录制的两份 Unreal Trace，对 FluidNinjaLive 水体交互插件进行开启/关闭 A/B 对比，并与 2026-09-14 的优化前基线进行比较。

**结论：本次优化有效，但降本幅度有限。** 去除 Bookmark 首尾各 1 秒后的稳态端到端成本由 **2.099 ms/帧** 降至 **1.824 ms/帧**，平均成本改善 **13.1%**；P95 增量改善 **10.4%**。但是 P50 只改善 **2.5%**，P99 增量由 2.119 ms 上升至 2.494 ms，尾延迟恶化 **17.7%**。

- 优化后插件开启：平均 **17.770 ms/帧**，约 **56.28 FPS**。
- 优化后插件关闭：平均 **15.945 ms/帧**，约 **62.72 FPS**。
- 插件稳态增量：**+1.824 ms/帧**，约损失 **6.44 FPS**。
- 插件增量占 60 FPS 的 16.67 ms 帧预算约 **10.9%**。
- `CanvasDrawTiles` 和 `NinjaLiveComponent` 的单帧成本均有改善。
- 每帧约 7 次 Canvas/RenderTarget 更新的调用结构没有改变，仍是下一轮优化的首要目标。
- 优化后开启版出现少量 20–26 ms 的 GPU-bound 慢帧，短样本下尚不能判断是否稳定复现，需要延长并重复采样。

## 2. 测试样本与识别

| 项目 | 插件开启 | 插件关闭 |
|---|---|---|
| Trace | `20260915_155722.utrace` | `20260915_155748.utrace` |
| 文件大小 | 206,035,334 bytes | 207,713,742 bytes |
| Bookmark 区间 | 85.1166–91.0643 s | 111.583–116.700 s |
| Bookmark 时长 | 5.948 s | 5.117 s |

开启版通过以下仅在该 Trace 中出现的事件识别：

- `NinjaLiveComponent`
- `CanvasDrawTiles`
- `DrawMaterialToRenderTarget`
- `NiagaraGpuComputeDispatch`

两份 Trace 均成功采集 CPU、GPU、Frame、Bookmark、RDG 和 RHICommands 通道。

## 3. 测量方法

1. 使用进入和离开测试区间的 Bookmark 定义 A/B 测量窗口。
2. 只统计完整落在 Bookmark 区间内的 Game Frame。
3. 同时计算完整窗口和去除首尾各 1 秒后的稳态结果。
4. 使用插件独有的 CPU/GPU scope 下钻主要成本。
5. CPU inclusive scope 存在父子嵌套和跨线程并行，GPU 动态 RDG scope 也存在同名嵌套，因此不将 scope 时间简单相加；插件端到端成本以同轮“开启减关闭”的帧时间差为准。

## 4. 优化后 A/B 帧时间

### 4.1 完整 Bookmark 窗口

| 指标 | 插件开启 | 插件关闭 | 开启增量 |
|---|---:|---:|---:|
| 平均帧时间 | 17.903 ms | 15.778 ms | **+2.126 ms** |
| P50 | 17.781 ms | 15.736 ms | **+2.046 ms** |
| P95 | 19.242 ms | 16.949 ms | **+2.293 ms** |
| P99 | 20.316 ms | 17.862 ms | **+2.455 ms** |
| 推算平均 FPS | 55.86 | 63.38 | **-7.53 FPS** |

### 4.2 稳态结果

去除 Bookmark 首尾各 1 秒后：

| 指标 | 插件开启 | 插件关闭 | 开启增量 |
|---|---:|---:|---:|
| 平均帧时间 | 17.770 ms | 15.945 ms | **+1.824 ms** |
| P50 | 17.910 ms | 15.901 ms | **+2.009 ms** |
| P95 | 19.190 ms | 17.175 ms | **+2.015 ms** |
| P99 | 20.405 ms | 17.912 ms | **+2.494 ms** |
| 推算平均 FPS | 56.28 | 62.72 | **-6.44 FPS** |

从平均值和 P50 看，插件仍持续增加约 1.8–2.0 ms 的典型帧成本。P99 增量高于平均值，说明优化后样本中仍存在需要关注的帧节奏问题。

## 5. 与优化前基线对比

优化前基线来自：

- 插件开启：`20260914_111626.utrace`
- 插件关闭：`20260914_111722.utrace`

采用两轮各自的“插件开启减插件关闭”稳态增量比较，可避免不同录制轮次基础场景负载变化造成误判。

| 指标 | 优化前增量 | 优化后增量 | 变化 |
|---|---:|---:|---:|
| 平均帧时间 | 2.099 ms | 1.824 ms | **改善 13.1%** |
| P50 | 2.059 ms | 2.009 ms | **改善 2.5%** |
| P95 | 2.250 ms | 2.015 ms | **改善 10.4%** |
| P99 | 2.119 ms | 2.494 ms | **恶化 17.7%** |

这说明本轮优化主要改善了平均工作量和较慢的常规帧，但典型帧改善很小，并且没有改善尾部慢帧。不能直接用两轮插件开启时的绝对 FPS 判断优化收益，因为新一轮插件关闭基线本身比旧一轮更慢；同轮 A/B 增量才是可靠指标。

## 6. 插件 Scope 新旧变化

下表为插件开启时的单帧或单次更新成本。CPU scope 可能并行或嵌套，不应彼此累加为端到端帧时间。

| Scope | 优化前 | 优化后 | 变化 |
|---|---:|---:|---:|
| `CanvasDrawTiles` 平均/帧 | 0.940 ms | 0.872 ms | **改善 7.2%** |
| `CanvasDrawTiles` P95/帧 | 1.441 ms | 1.153 ms | **改善 20.0%** |
| `NinjaLiveComponent` 平均/帧 | 0.379 ms | 0.333 ms | **改善 12.2%** |
| `NinjaLiveComponent` P95/帧 | 0.498 ms | 0.446 ms | **改善 10.4%** |
| Niagara CPU dispatch | 0.227 ms | 0.270 ms | **恶化 18.7%** |
| Niagara indirect CPU | 0.070 ms | 0.065 ms | 改善 7.9% |
| Niagara GPU | 0.385 ms/更新 | 0.363 ms/更新 | 改善 5.8% |
| GPU `DrawMaterialToRenderTarget` 原始 inclusive | 2.336 ms | 2.268 ms | 改善 2.9% |

优化后主要 scope 明细：

- `CanvasDrawTiles`：平均 **0.872 ms/帧**，P95 **1.153 ms/帧**。
- `NinjaLiveComponent`：平均 **0.333 ms/帧**，P95 **0.446 ms/帧**。
- `DrawMaterialToRenderTarget` CPU：平均 **0.078 ms/帧**。
- Niagara CPU dispatch：平均 **0.270 ms/帧**，P95 **0.420 ms/帧**。
- Niagara indirect CPU：平均 **0.065 ms/帧**。
- Niagara GPU：约 **0.363 ms/更新**。
- GPU `DrawMaterialToRenderTarget` 动态 scope：原始 inclusive 约 **2.268 ms/活动帧**。

GPU `DrawMaterialToRenderTarget` 数据包含动态 RDG metadata、同名嵌套和重复包裹，2.268 ms 不是可直接相加的墙钟 GPU 增量。端到端结果仍应采用第 4 节的 A/B 帧时间差。

## 7. 调用频率判断

优化前后每帧调用数量基本不变：

| 工作项 | 优化后每帧调用量 |
|---|---:|
| `CanvasDrawTiles` | 约 7 次 |
| `DrawMaterialToRenderTarget` | 约 7 次 |
| Niagara CPU dispatch | 约 5 次 |
| Niagara indirect update | 约 2 次 |

因此，本轮收益主要来自降低单次 pass/update 的成本，而不是减少 pass 数量。`CanvasDrawTiles` 的 P95 改善 20% 是积极信号，但每帧约 7 次 Canvas/RenderTarget 往返仍是最显著的结构性成本。

## 8. 慢帧与异常分析

### 8.1 双方最大帧

- 插件开启最大帧：**68.669 ms**；其中 `Slate::Tick` 约 63.069 ms，并包含两次 `FlushRenderingCommands`。
- 插件关闭最大帧：**57.902 ms**；其中 `Slate::Tick` 约 54.640 ms，并包含两次 `FlushRenderingCommands`。

两边均出现结构相同的编辑器 Slate/同步刷新尖峰，不能归因于插件。

### 8.2 开启版稳态慢帧

优化后开启版另有 3 个值得关注的稳态慢帧：

- 25.759 ms
- 21.765 ms
- 20.532 ms

这些帧的主要特征是：

- `GPUBound_WaitingForGPUForOcclusionQueries` 约 10–12 ms。
- RenderThread 在 `InitViews` / `VisibilityCommands` 等阶段等待。
- 单个插件 CPU scope 均低于约 0.5 ms，没有发现某个插件 CPU 函数突然放大的证据。

因此不能把这些慢帧归因于单个插件 CPU 函数，但它们只在本轮开启样本中出现，说明插件增加的 GPU 压力可能使部分帧跨过 20 ms 阈值。当前有效稳态窗口只有约 3–4 秒，暂时应视为尾延迟风险，而不是已经确认的稳定回退。

## 9. 下一轮优化优先级

### P0：减少 Canvas/RenderTarget pass 数量

1. 仅在输入或模拟状态变脏时更新，不变的速度场、密度场和交互数据直接复用。
2. 玩家或交互体不在有效浅滩范围内时停止更新。
3. 将 7 个更新 pass 合并，减少 RenderTarget 切换和 Canvas flush。
4. 将部分 60 Hz 更新改为固定 30 Hz、隔帧更新或按距离/可见性自适应更新。
5. 如果现有 Canvas 路径难以继续压缩，评估合并为单一 RDG/Compute 流程。

### P1：回查 Niagara CPU dispatch 回退

Niagara CPU dispatch 从 0.227 ms/帧上升到 0.270 ms/帧，恶化约 18.7%。建议核查本次优化是否：

- 增加了 Niagara 参数上传、资源转换或同步点；
- 改变了 dispatch 批次或线程分布；
- 将原本分散的工作集中到 Bookmark 稳态窗口；
- 使 GPU/RenderThread 压力增加，从而放大 CPU 等待时间。

### P2：控制尾延迟

1. 检查开启插件后 GPU 利用率和遮挡查询等待是否更容易接近饱和。
2. 避免在同一帧集中提交全部 RenderTarget 更新和 Niagara dispatch。
3. 在可接受的交互延迟范围内错峰或分帧执行更新。

## 10. 验收建议

当前插件稳态平均增量为 1.824 ms，已经比优化前更好，但仍高于建议目标。推荐下一轮验收标准：

- 平均端到端增量不超过 **1.0–1.2 ms/帧**。
- P95 增量不超过 **1.5 ms/帧**。
- 开启插件后的场景 P95 保持在 **16.67 ms** 以下，或符合项目实际目标帧率预算。
- 每组录制至少 **10–15 秒**有效 Bookmark 区间，并重复 **3 次**。
- 保持相同视角、分辨率、相机路径、角色输入和编辑器/Standalone 模式。
- 将三次同轮 A/B 的增量取中位数，并单独统计超过 20 ms、25 ms 和 33.3 ms 的帧数。

## 11. 最终判断

本轮优化已经产生可量化收益：平均插件成本下降约 **0.275 ms/帧**，平均增量改善 **13.1%**，`CanvasDrawTiles` P95 改善 **20.0%**，`NinjaLiveComponent` 平均成本改善 **12.2%**。但调用频率没有下降，P50 几乎不变，P99 反而恶化，说明当前实现仍受高频 RenderTarget/Canvas 更新和 GPU 帧节奏约束。

下一轮最有价值的方向不是继续微调单次调用，而是让“每帧约 7 次更新”变少：按 dirty 状态跳过、降低更新频率、合并 pass，并同步回查 Niagara CPU dispatch 的回退。

## 12. RDG `RT_Output` 试迁移验证

已为 `RT_Output` 建立第一条可独立切换的 RDG Canvas raster 管线，默认仍使用旧 Canvas 路径。验证模式会让旧路径与 RDG 路径在同一采样帧分别写入主 RT 和临时对照 RT，再通过 GPU Compute 统计 RGBA 每通道最大绝对误差和超阈值像素数；结果通过异步 readback 返回，不同步阻塞 Game Thread。

控制台变量：

```text
FluidTest.NinjaLive.OutputRenderPath 0
FluidTest.NinjaLive.OutputRenderPath 1
FluidTest.NinjaLive.OutputRDGValidation 1
FluidTest.NinjaLive.OutputRDGValidationInterval 30
```

- `OutputRenderPath=0`：旧 Canvas 主路径。
- `OutputRenderPath=1`：RDG 主路径。
- `OutputRDGValidation=1`：开启旧/新 GPU 差分。
- `OutputRDGValidationInterval`：差分采样间隔，最小为 1 帧。

在 D3D12、SM6、`/Game/_MyTest/M_Start`、2048×2048 输出下分别验证“RDG 主路径 + Legacy 对照”和“Legacy 主路径 + RDG 对照”，多帧结果一致：

```text
max=(0, 0, 0, 0)
exceeded=0/4194304
tolerance=0.001
```

异步 readback 队列限制为最多 4 个并发请求；组件 Tick 会继续回收已经提交的结果，模块退出时会等待 GPU 完成本轮工作并释放剩余 readback。验证关闭后，验证专用的输出 RT 和对照 RT 会释放引用。该结果证明 `RT_Output` 在当前材质、格式和测试场景中具备像素级等价性，但不代表其余模拟 pass 已迁移，也不代表 pass 数量已经减少。

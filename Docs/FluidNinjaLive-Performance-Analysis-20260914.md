# FluidNinjaLive 水体交互插件性能分析报告

## 1. 结论摘要

本次使用两份 Unreal Trace 对 FluidNinjaLive 水体交互插件进行开启/关闭 A/B 对比。以 Bookmark 标记的水体交互区间为测量窗口，并排除窗口首尾各 1 秒及双方共同存在的编辑器刷新尖峰后，插件带来的稳定帧时间增量约为 **2.10 ms/帧**。

- 插件开启：平均 **17.219 ms/帧**，约 **58.1 FPS**。
- 插件关闭：平均 **15.120 ms/帧**，约 **66.1 FPS**。
- 稳态增量：**+2.099 ms/帧**，帧时间增加约 **13.9%**，帧率下降约 **8.0 FPS**。
- 该增量占 60 FPS 的 16.67 ms 帧预算约 **12.6%**，使当前场景从 60 FPS 以上下降到 60 FPS 以下。

主要成本集中在 RenderTarget/Canvas 更新、`NinjaLiveComponent` 的 GameThread Tick，以及 Niagara GPU 模拟。两份样本中的约 60 ms 尖峰均由编辑器 Slate 和 `FlushRenderingCommands` 引起，不属于插件特有尖峰。

## 2. 测试样本

| 项目 | 插件开启 | 插件关闭 |
|---|---|---|
| Trace | `20260914_111626.utrace` | `20260914_111722.utrace` |
| 文件大小 | 280,939,557 bytes | 252,432,890 bytes |
| Trace 总时长 | 457.017 s | 511.573 s |
| Bookmark 开始 | 442.088 s | 497.829 s |
| Bookmark 结束 | 451.432 s | 505.626 s |
| Bookmark 窗口 | 9.344 s | 7.797 s |
| 窗口内完整帧 | 540 | 511 |

开启版通过以下仅在该 Trace 中出现的事件自动识别：

- `NinjaLiveComponent`
- `CanvasDrawTiles`
- `DrawMaterialToRenderTarget`
- `NiagaraGpuComputeDispatch`
- `NiagaraUpdateDrawIndirectBuffers`

两份 Trace 均成功采集 CPU、GPU、Frame、Bookmark、RDG 和 RHICommands 通道。

## 3. 测量方法

1. 使用 `ShallowWater_Enter` 与 `ShallowWater_Exit` Bookmark 定义交互测量窗口。
2. 只统计完整落在窗口内的 Game Frame，避免边界帧被截断。
3. 同时报告整个 Bookmark 窗口和“首尾各去除 1 秒”的稳态结果。
4. 使用插件独有计时器定位 GameThread、RenderThread、RHI/Worker 和 GPU 工作。
5. CPU inclusive scope 可能存在父子嵌套或跨线程并行，因此不直接相加为墙钟帧时间；最终以 A/B 帧时间差作为端到端成本。

## 4. 帧时间对比

### 4.1 完整 Bookmark 窗口

| 指标 | 插件开启 | 插件关闭 | 开启增量 |
|---|---:|---:|---:|
| 平均帧时间 | 17.258 ms | 15.213 ms | **+2.046 ms** |
| P50 | 17.212 ms | 15.186 ms | **+2.027 ms** |
| P95 | 18.473 ms | 16.281 ms | **+2.191 ms** |
| P99 | 18.872 ms | 16.631 ms | **+2.241 ms** |
| 最大帧时间 | 63.375 ms | 56.616 ms | +6.759 ms |
| 超过 33.3 ms 的帧 | 1 | 1 | 0 |
| 推算平均 FPS | 57.9 | 65.7 | **-7.8 FPS** |

### 4.2 稳态结果

排除 Bookmark 首尾各 1 秒后：

| 指标 | 插件开启 | 插件关闭 | 开启增量 |
|---|---:|---:|---:|
| 样本帧数 | 425 | 382 | — |
| 平均帧时间 | 17.219 ms | 15.120 ms | **+2.099 ms** |
| P50 | 17.193 ms | 15.134 ms | **+2.059 ms** |
| P95 | 18.438 ms | 16.188 ms | **+2.250 ms** |
| P99 | 18.775 ms | 16.656 ms | **+2.119 ms** |
| 最大帧时间 | 19.551 ms | 17.082 ms | +2.469 ms |
| 推算平均 FPS | 58.1 | 66.1 | **-8.0 FPS** |

平均值、P50、P95 和 P99 的增量均稳定在约 2.1 ms，说明这不是少量离群帧造成的统计偏差，而是插件持续运行产生的稳定成本。

## 5. CPU 成本分解

下表均限定在插件开启版的 Bookmark 窗口内。“每帧工作量”是相同 scope 在一帧内的 inclusive 时间合计；跨线程工作可能并行执行。

| Scope | 主要线程 | 每帧调用 | 平均工作量/帧 | P50/帧 | P95/帧 | 最大值/帧 |
|---|---|---:|---:|---:|---:|---:|
| `CanvasDrawTiles` | RenderThread | 约 7 次 | **0.940 ms** | 0.869 ms | **1.441 ms** | 3.058 ms |
| `NinjaLiveComponent` | GameThread | 约 1.14 次 | **0.379 ms** | 0.370 ms | **0.498 ms** | 0.838 ms |
| `NiagaraGpuComputeDispatch` | Render/RHI/Worker | 约 5 次 | 0.227 ms | 0.216 ms | 0.340 ms | 0.485 ms |
| `NiagaraUpdateDrawIndirectBuffers` | Render/RHI | 约 2 次 | 0.070 ms | 0.062 ms | 0.103 ms | 0.343 ms |
| `DrawMaterialToRenderTarget` CPU 调用 | GameThread | 约 7 次 | 0.076 ms | 0.074 ms | 0.090 ms | 0.226 ms |

典型开启帧显示：

- `NinjaLiveComponent` 在 GameThread 执行，内部包含水体区域 Blueprint 和部分 `DrawMaterialToRenderTarget` 调用。
- `CanvasDrawTiles` 在 RenderThread 执行，是最明显的 CPU/RenderThread 单项成本。
- Niagara dispatch 分散在 RenderThread、RHIThread 和工作线程，存在并行和嵌套关系。
- 因此 `NinjaLiveComponent` 与其子 scope，以及 Niagara 的父子 scope 不能重复累加。

## 6. GPU 成本分解

### 6.1 Niagara 模拟

插件开启版包含 1,189 次 Niagara GPU 更新，关闭版完全不存在对应事件。

| GPU Scope | 平均 | P50 | P95 | P99 | 最大值 |
|---|---:|---:|---:|---:|---:|
| 主 `NiagaraGpuComputeDispatch` | **0.379 ms** | 0.310 ms | **0.498 ms** | 0.502 ms | 0.587 ms |
| 辅助 `NiagaraGpuComputeDispatch` | 0.0007 ms | 0.0007 ms | 0.0011 ms | 0.0014 ms | 0.0015 ms |
| `NiagaraUpdateDrawIndirectBuffers` | 0.0057 ms | 0.0056 ms | 0.0066 ms | 0.0076 ms | 0.0121 ms |

三项平均合计约 **0.385 ms/次更新**。

### 6.2 RenderTarget 材质绘制

开启版还记录到插件独有的 GPU `DrawMaterialToRenderTarget` 动态 scope：

- 事件数：16,646
- Inclusive 总时间：2,777.115 ms
- 按 1,189 个插件活动帧归一化：**2.336 ms/活动帧**
- 关闭版：0

需要注意，RDG 动态 metadata 为同名 pass 生成了大量一次性 Timer，并且其中可能包含父子嵌套或重复包裹。上述 2.336 ms 是原始 inclusive scope 总和，不能与 Niagara GPU 时间直接相加，也不能视为精确墙钟 GPU 增量。端到端成本仍应采用第 4 节实测的 **+2.10 ms/帧**。

结合 CPU `CanvasDrawTiles`、每帧约 7 次 RenderTarget 更新，以及 GPU `DrawMaterialToRenderTarget` 只在开启版出现，可以确认 RenderTarget 流体更新链路是本次插件成本的首要来源。

## 7. 异常帧分析

### 插件开启版最大帧：63.375 ms

- `Slate::Tick (Time and Widgets)`：59.097 ms
- 第一次 `FlushRenderingCommands`：33.905 ms
- 第二次 `FlushRenderingCommands`：21.970 ms

### 插件关闭版最大帧：56.616 ms

- `Slate::Tick (Time and Widgets)`：53.244 ms
- 第一次 `FlushRenderingCommands`：28.947 ms
- 第二次 `FlushRenderingCommands`：21.183 ms

两边在进入 Bookmark 后约 0.4 秒均出现结构相同、量级接近的尖峰，根因是编辑器 Slate 触发同步刷新并等待 Render/RHI Thread。关闭插件时同样存在，因此不能归因于 FluidNinjaLive。

去除该共同尖峰后，开启版最大稳态帧为 19.551 ms，关闭版为 17.082 ms，插件没有表现出额外的严重偶发卡顿，但会持续增加约 2.1 ms 基础帧时间。

## 8. 优化建议

### P0：减少 RenderTarget/Canvas 更新

当前每帧约执行 7 次 `CanvasDrawTiles` 和 7 次 `DrawMaterialToRenderTarget`。建议优先检查：

1. 是否可以合并材质 pass，减少 RenderTarget 切换和 Canvas flush。
2. 无交互输入、速度场和密度场没有变化时，跳过对应更新。
3. 将 60 Hz 全量模拟改为固定 30 Hz、隔帧更新或按距离自适应更新。
4. 检查模拟 RenderTarget 分辨率和像素格式，避免超出实际交互精度需求。
5. 评估用单一 RDG/Compute 流程代替多次 Canvas/`DrawMaterialToRenderTarget` 往返。

### P1：降低 GameThread 组件成本

1. 仅在玩家或交互物体进入有效范围时启用 `NinjaLiveComponent` Tick。
2. 缓存不变的 Blueprint 参数、材质实例和对象查询结果。
3. 将高频 Blueprint 调度逐步迁移至 C++，但应先优化 RenderTarget 更新，因为其收益更大。

### P2：降低 Niagara 更新成本

1. 降低 Niagara Grid 分辨率、迭代次数或 dispatch 频率。
2. 无新增交互时复用上一帧结果。
3. 检查 indirect-buffer 更新是否能够合并或延后。

## 9. 性能预算建议

关闭插件时场景稳态平均帧时间已经达到 15.120 ms，只剩约 1.55 ms 的 60 FPS 预算余量；插件当前增加约 2.10 ms，因此至少需要降低约 **0.55 ms** 才能刚好回到 60 FPS。

考虑运行波动和内容增长，建议将插件端到端稳态增量控制到：

- 平均值：不超过 **1.0–1.2 ms/帧**
- P95：不超过 **1.5 ms/帧**
- 插件启用后的场景 P95：保持在 **16.67 ms** 以下

## 10. 分析限制

- 两段 Bookmark 窗口长度不同，但帧数量均超过 380，且平均、P50、P95、P99 的增量一致，结论具有较好的稳定性。
- Trace 来自编辑器环境，Slate、窗口刷新和后台编辑器任务仍会引入噪声；最终交付前应在相同分辨率、相同路径的 Standalone 或 Development/Shipping 构建中再录制一次。
- GPU `DrawMaterialToRenderTarget` 使用动态 RDG metadata，当前导出的同名 scope 包含一次性 Timer 和可能的嵌套计时，因此报告保留其原始 inclusive 数据，但不将其直接计入最终墙钟成本。


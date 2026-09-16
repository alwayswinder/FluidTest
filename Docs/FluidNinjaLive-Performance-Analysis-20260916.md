# FluidNinjaLive 第二轮性能优化复测报告

## 1. 结论摘要

本轮分析最新两份 Unreal Trace，并以同轮“插件开启减插件关闭”的方式测量 FluidNinjaLive 水体交互插件净成本。

**结论：本次 Trace 没有证明第二轮优化带来端到端收益。** 排除 Bookmark 首尾各 1 秒，并剔除双方各自一次结构相同的编辑器 Slate/`FlushRenderingCommands` 尖峰后，插件稳态平均增量为 **2.073 ms/帧**。上一轮为 **1.824 ms/帧**，本轮回退约 **13.7%**。

- P50 增量为 **1.984 ms**，比上一轮轻微改善 **1.2%**。
- P95 增量为 **2.296 ms**，比上一轮回退 **13.9%**。
- P99 增量为 **3.149 ms**，比上一轮回退 **26.3%**；但本轮有效样本较短，P99 置信度有限。
- 插件开启时平均 **16.809 ms/帧**，约 **59.49 FPS**。
- 插件关闭时平均 **14.736 ms/帧**，约 **67.86 FPS**。
- `CanvasDrawTiles` 平均成本基本持平，P95 小幅改善。
- `NinjaLiveComponent`、DrawMaterial CPU 和 Niagara indirect CPU 均小幅回退。
- Niagara CPU dispatch 和 Niagara GPU 单次更新略有改善。
- 每帧约 7 次 Canvas/RenderTarget 更新的结构没有变化。

这不等于代码优化一定无效：两轮录制的基础场景负载明显不同，并且本轮关闭版改善幅度大于开启版。只能确认在当前 A/B 样本中，插件净增量没有下降。建议用更长、重复的相同路径样本重新验收。

## 2. 测试样本

| 项目 | 插件开启 | 插件关闭 |
|---|---|---|
| Trace | `20260916_103338.utrace` | `20260916_103409.utrace` |
| 文件大小 | 214,803,260 bytes | 158,427,338 bytes |
| Trace 总时长 | 112.758 s | 143.463 s |
| Bookmark 开始 | 103.117 s | 133.431 s |
| Bookmark 结束 | 109.026 s | 137.857 s |
| Bookmark 时长 | 5.909 s | 4.426 s |

第一份通过 `NinjaLiveComponent`、`CanvasDrawTiles`、`DrawMaterialToRenderTarget` 和 `NiagaraGpuComputeDispatch` 判定为插件开启版；第二份没有这些插件工作，判定为关闭版。

两份 Trace 均启用了 CPU、GPU、Frame、Bookmark、RDG、RHICommands、Niagara、Region 和 Log 通道。

## 3. 测量方法

1. 使用 `ShallowWater_Enter` 和 `ShallowWater_Exit` Bookmark 定义测量区间。
2. 只统计完整落在区间内的 Game Frame。
3. 稳态结果排除 Bookmark 首尾各 1 秒。
4. 双方稳态窗口内各有一次约 60 ms 的共同编辑器尖峰；其结构均为 Slate Tick 驱动两次 `FlushRenderingCommands`，因此各剔除一次。
5. CPU/GPU scope 可能嵌套或跨线程并行，不把 scope 时间直接相加为总帧时间；插件净成本以同轮 A/B 帧时间差为准。

## 4. 优化后 A/B 结果

### 4.1 稳态结果

| 指标 | 插件开启 | 插件关闭 | 插件增量 |
|---|---:|---:|---:|
| 有效帧数 | 228 | 160 | — |
| 平均帧时间 | **16.809 ms** | **14.736 ms** | **+2.073 ms** |
| P50 | 16.871 ms | 14.887 ms | **+1.984 ms** |
| P95 | 18.083 ms | 15.788 ms | **+2.296 ms** |
| P99 | 19.262 ms | 16.113 ms | **+3.149 ms** |
| 最大帧时间 | 20.620 ms | 18.939 ms | +1.681 ms |
| 推算平均 FPS | 59.49 | 67.86 | **-8.37 FPS** |

插件增量占 60 FPS 的 16.67 ms 帧预算约 **12.4%**。开启版平均帧时间已经略高于 60 FPS 预算。

### 4.2 完整 Bookmark 窗口

剔除双方共同类型的编辑器尖峰后：

| 指标 | 插件开启 | 插件关闭 | 插件增量 |
|---|---:|---:|---:|
| 有效帧数 | 347 | 297 | — |
| 平均帧时间 | 16.779 ms | 14.641 ms | **+2.139 ms** |
| P50 | 16.848 ms | 14.666 ms | **+2.181 ms** |
| P95 | 17.926 ms | 15.759 ms | **+2.167 ms** |
| P99 | 19.066 ms | 17.996 ms | +1.070 ms |

完整窗口与稳态窗口的平均增量均约为 2.1 ms，说明“本轮插件净成本没有低于上一轮”不是由单一慢帧造成的。

## 5. 三轮增量趋势

下表均使用各轮同一次测试中的“开启减关闭”结果，避免跨轮绝对场景负载变化造成误判。

| 指标 | 初始基线 09-14 | 第一轮优化 09-15 | 第二轮优化 09-16 |
|---|---:|---:|---:|
| 平均增量 | 2.099 ms | **1.824 ms** | **2.073 ms** |
| P50 增量 | 2.059 ms | 2.009 ms | **1.984 ms** |
| P95 增量 | 2.250 ms | **2.015 ms** | **2.296 ms** |
| P99 增量 | 2.119 ms | 2.494 ms | **3.149 ms** |

第二轮相对第一轮：

| 指标 | 变化 | 判断 |
|---|---:|---|
| 平均增量 | +0.249 ms / **+13.7%** | 回退 |
| P50 增量 | -0.025 ms / **-1.2%** | 基本持平 |
| P95 增量 | +0.281 ms / **+13.9%** | 回退 |
| P99 增量 | +0.655 ms / **+26.3%** | 回退，短样本需复验 |

第二轮相对初始基线，平均增量只改善约 **1.2%**，可以视为基本回到初始水平。典型帧 P50 略好，但 P95/P99 没有改善。

## 6. 关键插件 Scope 对比

下表比较插件开启版的稳态单帧工作量；本轮已排除 64.239 ms 编辑器尖峰帧。

| Scope | 第一轮优化 09-15 | 第二轮优化 09-16 | 变化 |
|---|---:|---:|---:|
| `CanvasDrawTiles` 平均/帧 | 0.872 ms | 0.879 ms | **回退 0.8%** |
| `CanvasDrawTiles` P95/帧 | 1.153 ms | 1.120 ms | 改善 2.9% |
| `NinjaLiveComponent` 平均/帧 | 0.333 ms | 0.355 ms | **回退 6.6%** |
| `NinjaLiveComponent` P95/帧 | 0.446 ms | 0.470 ms | **回退 5.3%** |
| `DrawMaterialToRenderTarget` CPU 平均/帧 | 0.078 ms | 0.082 ms | **回退约 5%** |
| Niagara CPU dispatch 平均/帧 | 0.270 ms | 0.267 ms | 改善约 1% |
| Niagara CPU dispatch P95/帧 | 0.420 ms | 0.400 ms | 改善约 4.8% |
| Niagara indirect CPU 平均/帧 | 0.065 ms | 0.069 ms | **回退约 5.5%** |
| Niagara GPU 平均/更新 | 0.363 ms | 0.350 ms | 改善约 3.7% |
| GPU DrawMaterial 原始 inclusive/活动帧 | 2.268 ms | 2.320 ms | **回退约 2.3%** |

本轮每帧调用频率：

| Scope | 调用数/帧 |
|---|---:|
| `CanvasDrawTiles` | 6.97，约 7 次 |
| `DrawMaterialToRenderTarget` | 7.00 次 |
| Niagara CPU dispatch | 4.98，约 5 次 |
| Niagara indirect CPU | 1.99，约 2 次 |
| `NinjaLiveComponent` | 1.14 次 |

调用数量与上一轮基本一致。这意味着第二轮修改仍没有降低 pass/update 数量；各单项变化多在 1%–7% 范围内，尚未形成端到端收益。

GPU DrawMaterial 使用动态 RDG metadata，同名 scope 包含大量一次性 Timer、父子嵌套及少量 GPU 时间线交错警告。报告中的 2.320 ms 仅用于与上一轮相同口径做方向性比较，不能与其他 GPU scope 相加，也不能视为准确墙钟成本。

## 7. 异常帧分析

### 7.1 双方共同编辑器尖峰

插件开启版最大帧为 **64.239 ms**：

- `Slate::Tick (Time and Widgets)`：60.424 ms
- 两次 `FlushRenderingCommands`：33.241 ms、23.745 ms
- RenderThread/RHIThread 等待链与 GPU 遮挡查询等待同时出现

插件关闭版最大帧为 **59.328 ms**：

- `Slate::Tick (Time and Widgets)`：55.855 ms
- 两次 `FlushRenderingCommands`：31.202 ms、21.201 ms
- 等待链结构与开启版相同

这是双方共同存在的编辑器刷新噪声，不属于插件特有回退。

### 7.2 开启版 20.620 ms 慢帧

开启版剔除编辑器尖峰后的最大帧为 **20.620 ms**。主要等待链为：

- GameThread `GameThreadWaitForTask`：14.532 ms
- RenderThread `FDeferredShadingSceneRenderer_Render`：14.376 ms
- `InitViews` / `VisibilityCommands`：约 10.88 ms
- `GPUBound_WaitingForGPUForOcclusionQueries`：约 10.76 ms

该帧没有单个插件 CPU scope 异常放大，仍表现为 GPU/可见性等待。关闭版也存在同类等待，但本轮开启版 P95/P99 更差，说明插件增加的 GPU 压力仍可能放大尾延迟。

## 8. 对第二轮优化的判断

当前证据支持以下判断：

1. **没有观测到端到端净收益。** 平均、P95 和 P99 增量均比上一轮更差。
2. **Niagara 路径有小幅改善。** CPU dispatch P95 和 GPU 单次更新下降约 4%–5%。
3. **Canvas 路径没有实质改善。** 平均成本基本不变，且仍保持每帧约 7 次更新。
4. **GameThread 组件成本小幅回退。** `NinjaLiveComponent` 平均和 P95 均上升约 5%–7%。
5. **样本仍偏短。** 稳态有效样本只有开启 228 帧、关闭 160 帧，不足以对 1%–7% 的单项变化做高置信判断。

如果本次代码改动主要针对 Niagara，它可能已经有效；但收益被 `NinjaLiveComponent`、RenderTarget 链路和基础 GPU 帧节奏抵消。如果改动目标是 Canvas/RenderTarget 或整体插件成本，则当前结果不达标。

## 9. 下一步建议

### P0：先提高验证置信度

1. 每组 Bookmark 有效区间录制至少 **15 秒**，建议 20–30 秒。
2. 开启/关闭各重复 **3 次**，交替顺序录制，例如 ON/OFF/OFF/ON，降低温度和缓存漂移。
3. 使用固定相机、固定角色轨迹、固定水体交互输入。
4. 尽量使用 Standalone 或 Development 构建，减少 Slate 和编辑器同步刷新。
5. 以三次“开启减关闭”增量的中位数作为验收结果。

### P1：回查本轮新增成本

1. 检查 `NinjaLiveComponent` 本轮新增的分支、参数更新、对象查询和 Blueprint 调度。
2. 检查 DrawMaterial CPU 路径是否增加了资源状态切换、材质参数设置或同步。
3. 检查 Niagara 优化是否将一部分工作转移到 GameThread/RenderThread。

### P2：继续处理结构性成本

当前最大机会仍不是微调单次调用，而是减少每帧约 7 次 Canvas/RenderTarget 更新：

1. 按 dirty/交互状态跳过无变化的更新。
2. 离开有效浅滩交互范围后立即停止模拟更新。
3. 合并材质 pass，减少 RenderTarget 切换和 Canvas flush。
4. 将部分更新降为 30 Hz、隔帧或按距离自适应。
5. 评估由单一 RDG/Compute 流程替代多次 Canvas 往返。

## 10. 验收结论

第二轮优化在 Niagara 单项上有轻微正向变化，但当前 Trace 中插件端到端净成本由 **1.824 ms/帧**回升到 **2.073 ms/帧**，P95 由 **2.015 ms**回升到 **2.296 ms**。因此本轮应判定为：

**单项局部优化可能有效，但整体性能验收未通过，需要更长重复样本确认，并继续降低 RenderTarget/Canvas 更新次数。**


# FluidNinjaLive Standalone 性能复测报告

## 1. 结论摘要

本轮使用 Standalone 环境重新录制的两份 Unreal Trace，对 FluidNinjaLive 水体交互插件进行开启/关闭 A/B 分析。

**结论：Standalone 消除了 Editor/Slate 尖峰，但插件持续成本仍约为 2.2 ms/帧。** 去除 Bookmark 首尾各 1 秒后的稳态增量为：

- 平均帧时间：**+2.164 ms**
- P50：**+2.156 ms**
- P95：**+2.255 ms**
- P99：**+2.252 ms**

四项增量高度一致，且两组样本均没有超过 20 ms 的帧，说明本次测到的是稳定、持续的插件开销，而不是少量编辑器异常帧造成的统计偏差。

- 插件开启：平均 **12.454 ms/帧**，约 **80.29 FPS**。
- 插件关闭：平均 **10.290 ms/帧**，约 **97.18 FPS**。
- 平均帧时间增加约 **21.0%**，推算平均帧率下降约 **16.89 FPS**。
- 插件增量占 60 FPS 的 16.67 ms 帧预算约 **13.0%**。
- 每帧仍执行 **7 次** `CanvasDrawTiles` 和 `DrawMaterialToRenderTarget`。
- Standalone 中插件 CPU scope 明显低于 Editor，但端到端增量仍约 2.2 ms，说明主要瓶颈仍在 RenderTarget/GPU 工作和跨线程等待链。

## 2. 测试样本

| 项目 | 插件开启 | 插件关闭 |
|---|---|---|
| Trace | `20260916_104430_7508F0.utrace` | `20260916_104550_7A8730.utrace` |
| 文件大小 | 100,852,135 bytes | 69,708,472 bytes |
| Trace 总时长 | 61.579 s | 31.615 s |
| Bookmark 开始 | 42.350 s | 19.536 s |
| Bookmark 结束 | 50.439 s | 25.387 s |
| Bookmark 时长 | 8.089 s | 5.851 s |

开启版存在以下插件独有工作：

- `NinjaLiveComponent`
- `NinjaLive_Area_Water_Blueprint_C`
- `CanvasDrawTiles`
- `DrawMaterialToRenderTarget`
- `NiagaraGpuComputeDispatch`
- `NiagaraUpdateDrawIndirectBuffers`

关闭版不存在 `NinjaLiveComponent` 和插件的 RenderTarget/Niagara 更新，因此样本身份明确。

## 3. Trace 通道状态

两份 Trace 均包含：

- CPU
- GPU
- Frame
- Bookmark
- Region
- Log

但本次 Standalone Trace 中以下通道显示为未启用：

- RDG
- RHICommands
- Niagara

GPU provider 中仍能读取 Niagara 和 DrawMaterial GPU timer，但缺少 RDG/RHICommands 通道会限制更细的 pass、资源转换和提交链归因。本报告可以可靠给出端到端 A/B 成本和已有 timer 的统计，但不能完整拆出每个 RDG pass 的墙钟贡献。

## 4. 帧时间 A/B 对比

### 4.1 稳态窗口

排除 Bookmark 首尾各 1 秒：

| 指标 | 插件开启 | 插件关闭 | 插件增量 |
|---|---:|---:|---:|
| 有效帧数 | 487 | 373 | — |
| 平均帧时间 | **12.454 ms** | **10.290 ms** | **+2.164 ms** |
| P50 | 12.295 ms | 10.138 ms | **+2.156 ms** |
| P95 | 14.205 ms | 11.950 ms | **+2.255 ms** |
| P99 | 14.834 ms | 12.582 ms | **+2.252 ms** |
| 最大帧时间 | 15.556 ms | 13.710 ms | +1.846 ms |
| 推算平均 FPS | 80.29 | 97.18 | **-16.89 FPS** |
| 超过 20 ms 的帧 | 0 | 0 | 0 |

平均、P50、P95、P99 的增量仅相差约 0.1 ms，表明插件成本分布非常稳定。

### 4.2 完整 Bookmark 窗口

| 指标 | 插件开启 | 插件关闭 | 插件增量 |
|---|---:|---:|---:|
| 有效帧数 | 644 | 557 | — |
| 平均帧时间 | 12.519 ms | 10.466 ms | **+2.053 ms** |
| P50 | 12.358 ms | 10.331 ms | **+2.026 ms** |
| P95 | 14.101 ms | 12.117 ms | **+1.984 ms** |
| P99 | 14.813 ms | 12.740 ms | **+2.073 ms** |
| 最大帧时间 | 15.556 ms | 13.710 ms | +1.846 ms |

完整窗口与稳态窗口结论一致，插件持续成本约为 2.0–2.3 ms/帧。

## 5. 与同日 Editor 样本对比

Editor 样本来自：

- 开启：`20260916_103338.utrace`
- 关闭：`20260916_103409.utrace`

| 稳态增量 | Editor | Standalone | 判断 |
|---|---:|---:|---|
| 平均 | 2.073 ms | **2.164 ms** | 接近，差约 4.4% |
| P50 | 1.984 ms | **2.156 ms** | Standalone 高 0.172 ms |
| P95 | 2.296 ms | **2.255 ms** | 基本一致 |
| P99 | 3.149 ms | **2.252 ms** | Standalone 尾部明显更干净 |

Standalone 与 Editor 的平均和 P95 增量都落在约 2.1–2.3 ms，互相验证了插件持续成本。Editor 中偏高的 P99 主要受编辑器和较短样本影响；Standalone 证明插件没有额外的严重偶发尖峰，但也证明约 2.2 ms 的基础开销是真实存在的。

由于运行模式和基础场景帧时间不同，不能用 Editor 与 Standalone 的绝对 FPS 判断优化收益。要严格评估代码优化前后变化，需要补录同配置的“优化前 Standalone”基线；当前没有可直接一一对应的 Standalone 旧样本。

## 6. CPU Scope 分解

下表统计插件开启版的稳态窗口。

| Scope | 调用数/帧 | 平均/帧 | P50/帧 | P95/帧 | P99/帧 |
|---|---:|---:|---:|---:|---:|
| `CanvasDrawTiles` | **7.00** | **0.811 ms** | 0.791 ms | **0.973 ms** | 1.313 ms |
| `NinjaLiveComponent` | 1.11 | **0.277 ms** | 0.273 ms | **0.363 ms** | 0.484 ms |
| `DrawMaterialToRenderTarget` CPU | **7.00** | 0.065 ms | 0.061 ms | 0.093 ms | 0.121 ms |
| Niagara CPU dispatch，全部稳态帧归一化 | 3.74 | 0.150 ms | 0.181 ms | 0.243 ms | 0.432 ms |
| Niagara indirect，全部稳态帧归一化 | 1.50 | 0.039 ms | 0.041 ms | 0.092 ms | 0.115 ms |

Niagara 在 487 个稳态帧中的 368 帧处于活动状态。按 Niagara 活动帧归一化：

| Scope | 调用数/活动帧 | 平均/活动帧 | P95/活动帧 |
|---|---:|---:|---:|
| Niagara CPU dispatch | 4.95 | **0.199 ms** | 0.261 ms |
| Niagara indirect CPU | 1.98 | **0.052 ms** | 0.099 ms |

与同日 Editor 样本相比，Standalone 中主要 CPU scope 都更低：

- `CanvasDrawTiles`：0.879 → **0.811 ms/帧**，降低约 7.7%。
- `NinjaLiveComponent`：0.355 → **0.277 ms/帧**，降低约 22%。
- DrawMaterial CPU：0.082 → **0.065 ms/帧**，降低约 21%。
- Niagara CPU dispatch 按活动帧：0.267 → **0.199 ms/帧**，降低约 26%。

但端到端增量没有同步下降，进一步说明当前瓶颈不只是插件 CPU scope，还包括 GPU 执行、RenderThread/RHIThread 等待和 RenderTarget pass 对整帧节奏的影响。

## 7. GPU Scope

### 7.1 Niagara

插件开启版的 GPU Niagara timer：

| GPU Scope | 平均/更新 | P50 | P95 | P99 | 最大值 |
|---|---:|---:|---:|---:|---:|
| 主 `NiagaraGpuComputeDispatch` | **0.320 ms** | 0.305 ms | **0.528 ms** | 0.530 ms | 0.534 ms |
| 辅助 `NiagaraGpuComputeDispatch` | 0.0007 ms | 0.0006 ms | 0.0008 ms | 0.0009 ms | 0.0117 ms |
| `NiagaraUpdateDrawIndirectBuffers` | 0.0047 ms | 0.0051 ms | 0.0063 ms | 0.0077 ms | 0.0143 ms |

主 Niagara GPU 更新平均约 0.320 ms，仍是明确的独占 GPU 成本。

### 7.2 DrawMaterial/RenderTarget

GPU `DrawMaterialToRenderTarget` 使用动态 metadata，产生大量同名、一次性以及可能嵌套的 scope。原始 inclusive 数据为：

- scope 行数：38,681
- inclusive 总时间：5,485.776 ms
- 按 2,011 个 GPU 活动帧归一化：2.728 ms/活动帧

该数据包含同名嵌套，不能与 Niagara GPU 时间相加，也不能直接视为墙钟 GPU 成本。它只能确认 RenderTarget 材质绘制链路规模很大。最终成本仍以第 4 节的端到端 A/B 增量 **2.164 ms/帧**为准。

## 8. 最慢帧分析

### 插件开启：15.556 ms

- `GameThreadWaitForTask`：13.372 ms
- RenderThread `FDeferredShadingSceneRenderer_Render`：12.642 ms
- `InitViews` / `VisibilityCommands`：约 10.35 ms
- `GPUBound_WaitingForGPUForOcclusionQueries`：10.324 ms

### 插件关闭：13.710 ms

- RenderThread `FDeferredShadingSceneRenderer_Render`：11.821 ms
- `GameThreadWaitForTask`：11.491 ms
- `GPUBound_WaitingForGPUForOcclusionQueries`：9.342 ms
- `InitViews` / `VisibilityCommands`：约 9.30 ms

双方瓶颈结构一致，均是 GPU/遮挡查询/可见性等待；开启版等待时间整体高约 1–2 ms，与端到端 A/B 增量一致。没有发现某个插件 CPU 函数在慢帧中异常突增。

## 9. 优化判断

Standalone 数据比 Editor 更适合作为后续基线，原因是：

1. 没有 Slate/`FlushRenderingCommands` 尖峰。
2. 没有超过 20 ms 的异常帧。
3. 平均、P50、P95、P99 增量高度一致。
4. 有效样本分别为 487 帧和 373 帧，统计稳定性优于上一轮短 Editor 稳态窗口。

当前可以确认：

- 插件的真实持续成本约为 **2.1–2.3 ms/帧**。
- 本场景开启插件后仍可保持 60 FPS，但消耗约 13% 的 60 FPS 帧预算。
- 当前没有证据表明整体插件成本已显著低于最初约 2.1 ms 的水平。
- 主要结构性问题仍是每帧 7 次 Canvas/RenderTarget 更新。
- Standalone 降低了 CPU 调度成本，但没有消除 GPU/RenderThread 等待增量。

## 10. 下一步建议

### P0：减少 RenderTarget 更新数量

1. 按 dirty 状态跳过无输入、无变化的更新。
2. 玩家或交互物体不在有效范围时停止模拟。
3. 合并 7 个材质 pass，减少 RenderTarget 切换和 Canvas flush。
4. 将部分更新改为 30 Hz、隔帧或按距离自适应。
5. 评估用合并的 RDG/Compute pass 替代 Canvas 往返。

### P1：继续使用 Standalone 做验收

1. 将本次数据作为新的 Standalone 基线。
2. 下一次优化后使用相同相机、路径、分辨率和输入重录。
3. 开启/关闭各录制至少 15 秒，并重复 3 次。
4. 以三组同轮 A/B 增量的中位数作为最终结果。

### P2：补齐 GPU 诊断通道

下一轮 Standalone 录制应确认实际进程中启用了：

```text
cpu,frame,bookmark,gpu,rdg,rhicommands
```

当前 Trace 中 RDG 和 RHICommands 显示为关闭，导致无法完整拆解 RenderTarget pass、资源转换与提交成本。启动 Standalone 后应再次通过 Unreal Insights 的 Trace 状态确认这些通道确实在目标进程中启用。

## 11. 最终结论

本次 Standalone 测试排除了 Editor 噪声，给出了更可靠的结论：

**FluidNinjaLive 在当前场景中的持续端到端成本约为 2.164 ms/帧，P95 增量约 2.255 ms，没有严重偶发尖峰，但整体成本仍未显著下降。下一轮优化应优先减少每帧 7 次 RenderTarget/Canvas 更新，而不是继续只压缩单次 CPU 调用。**


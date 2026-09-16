# FluidNinjaLive RDG / Legacy / 无模拟三路性能分析

## 1. 结论摘要

本轮分析同一 Standalone 测试环境下的三份 Unreal Trace：优化后 RDG、旧默认 Legacy、关闭 Fluid 模拟。

**当前 RDG 版本是性能回退，不建议替换 Legacy 默认路径。** 去除 Bookmark 首尾各 1 秒后：

- RDG 相对 Legacy 平均帧时间增加 **0.582 ms/帧（+4.25%）**。
- RDG 相对 Legacy 的 P95 增加 **1.116 ms（+7.12%）**，P99 增加 **1.972 ms（+11.71%）**。
- RDG 平均帧率为 **70.03 FPS**，Legacy 为 **73.00 FPS**，下降约 **2.98 FPS**。
- Legacy 相对无模拟的持续成本为 **1.502 ms/帧**；RDG 相对无模拟为 **2.084 ms/帧**。

RDG 的 CPU 优化方向是有效的：它移除了每帧 7 次 `DrawMaterialToRenderTarget` CPU 调用，并降低了 `NinjaLiveComponent` 与 `CanvasDrawTiles` 的平均 CPU 时间。但这些收益没有转化为整帧收益。

Trace 和源码共同表明，当前实现仍在每个 RDG 材质 pass 内创建并刷新 `FCanvas`，同时把 7 个材质 pass 分散在 3 个独立 `FRDGBuilder` 中，并插入多个 `NeverCull` 空 Raster pass 作为纹理访问屏障。也就是说，当前方案是“用 RDG 包装 Canvas”，而不是移除 Canvas；新增的 Graph 构建、Execute、外部纹理注册、资源更新和 pass 边界抵消并超过了 CPU 节省。

## 2. 测试样本与身份确认

| 模式 | Trace | 文件大小 | Bookmark 区间 |
|---|---|---:|---:|
| 优化后 RDG | `20260916_143548_7AFC60.utrace` | 211,936,959 bytes | 35.258–41.898 s |
| 旧默认 Legacy | `20260916_143645_4AC4A0.utrace` | 243,730,044 bytes | 25.638–32.945 s |
| 无 Fluid 模拟 | `20260916_143742_1B7740.utrace` | 231,009,982 bytes | 23.416–29.989 s |

身份识别依据：

- RDG Trace 存在 `FluidTest.NinjaLive.PressurePairPipeline`、`PainterCompositePipeline`、`AdvectionDivergencePipeline`。
- Legacy Trace 存在 GPU `DrawMaterialToRenderTarget`，且 CPU 侧约 7 次/帧。
- 无模拟 Trace 不存在 `NinjaLiveComponent`、`CanvasDrawTiles` 和对应模拟 Niagara dispatch。
- 三份 Trace 均包含 CPU、GPU、Frame、Bookmark、RDG、RHICommands，满足本轮分析要求。

## 3. 稳态帧时间

稳态窗口为 Bookmark 开始后 1 秒至结束前 1 秒，避免进出区域和 Bookmark 边界扰动。

| 指标 | RDG | Legacy | 无模拟 |
|---|---:|---:|---:|
| 样本帧数 | 324 | 385 | 373 |
| 平均帧时间 | **14.280 ms** | **13.698 ms** | **12.197 ms** |
| P50 | 14.026 ms | 13.626 ms | 12.197 ms |
| P95 | **16.782 ms** | 15.665 ms | 14.885 ms |
| P99 | **18.814 ms** | 16.842 ms | 16.222 ms |
| 最大值 | 19.254 ms | 18.760 ms | 18.716 ms |
| 推算平均 FPS | **70.03** | **73.00** | **81.99** |
| 超过 20 ms | 0 | 0 | 0 |

### 3.1 RDG 相对 Legacy

| 指标 | 差值 | 相对变化 |
|---|---:|---:|
| 平均帧时间 | **+0.582 ms** | **+4.25%** |
| P50 | +0.400 ms | +2.94% |
| P95 | **+1.116 ms** | **+7.12%** |
| P99 | **+1.972 ms** | **+11.71%** |
| 平均 FPS | **-2.98 FPS** | -4.08% |

RDG 不仅平均值更慢，尾部延迟的回退还更明显，因此不能把差异解释为少量离群帧。

### 3.2 Fluid 模拟相对关闭

| 指标 | Legacy 增量 | RDG 增量 |
|---|---:|---:|
| 平均帧时间 | **+1.502 ms** | **+2.084 ms** |
| P50 | +1.429 ms | +1.829 ms |
| P95 | +0.781 ms | +1.897 ms |
| P99 | +0.620 ms | +2.592 ms |
| 平均 FPS | -8.99 FPS | -11.96 FPS |

Legacy 的平均 Fluid 成本约占 60 FPS 帧预算的 **9.0%**；RDG 约占 **12.5%**。

完整 Bookmark 窗口平均帧时间分别为 RDG **14.250 ms**、Legacy **13.908 ms**、无模拟 **12.143 ms**，结论方向一致。

## 4. CPU 工作量对比

下表均为稳态窗口按帧归一化结果。

| Scope | RDG | Legacy | RDG 变化 |
|---|---:|---:|---:|
| `NinjaLiveComponent` 平均/帧 | **0.551 ms** | 0.662 ms | **-0.111 ms（-16.8%）** |
| `NinjaLiveComponent` P95 | 0.722 ms | 0.843 ms | -0.121 ms |
| `CanvasDrawTiles` 平均/帧 | **1.952 ms** | 2.166 ms | **-0.214 ms（-9.9%）** |
| `CanvasDrawTiles` P95 | **2.970 ms** | 2.757 ms | **+0.213 ms** |
| `DrawMaterialToRenderTarget` CPU | **0 ms** | 0.152 ms | **-0.152 ms** |
| Niagara CPU dispatch 平均/帧 | 0.346 ms | 0.321 ms | +0.025 ms |
| Niagara dispatch 活动帧平均 | 0.403 ms | 0.390 ms | +0.013 ms |
| Niagara indirect 平均/帧 | 0.109 ms | 0.101 ms | +0.008 ms |

调用频率：

- RDG 与 Legacy 的 `CanvasDrawTiles` 都约为 **7 次/帧**。
- Legacy 的 `DrawMaterialToRenderTarget` 约为 **7 次/帧**；RDG 为 0。
- Niagara dispatch 活动时约 **5 次/帧**，indirect 活动时约 **2 次/帧**。

因此，RDG 确实消除了 Kismet `DrawMaterialToRenderTarget` 的 CPU 包装成本，但没有消除 7 次实际 Canvas 绘制。`CanvasDrawTiles` 的平均值下降，P95 却上升，说明 RDG 路径的尾部稳定性更差。

## 5. GPU Scope 与 CanvasFlush

### 5.1 RDG 独有 Pipeline

| GPU Scope | 原始次数 | Inclusive 总时间 | 单事件平均 | P95 |
|---|---:|---:|---:|---:|
| `PressurePairPipeline` | 2,827 | 1,561.720 ms | **0.552 ms** | **1.132 ms** |
| `PainterCompositePipeline` | 2,827 | 840.703 ms | **0.297 ms** | 0.597 ms |
| `AdvectionDivergencePipeline` | 2,827 | 407.474 ms | **0.144 ms** | 0.288 ms |

三条 scope 的原始平均合计约 0.994 ms/事件组，但不能直接当成每帧新增 0.994 ms：GPU scope 存在嵌套/重复 metadata，inclusive 时间也会互相覆盖。可靠的最终结果仍是整帧 A/B 的 **+0.582 ms/帧**。

其中 `PressurePairPipeline` 是当前最值得优先优化的单条 RDG 管线，平均和 P95 都显著高于另外两条。

### 5.2 CanvasFlush 异常

全 Trace digest 显示：

| 通道 | RDG | Legacy |
|---|---:|---:|
| CPU `CanvasFlush` 次数 | **32,034** | 2,432 |
| CPU `CanvasFlush` inclusive 总时间 | **3,471.04 ms** | 99.38 ms |
| GPU `CanvasFlush` 次数 | **20,229** | 435 |
| GPU `CanvasFlush` inclusive 总时间 | **2,789.88 ms** | 4.09 ms |

这些数字不能直接解释为实际 flush 数量增长 40–50 倍，因为 Legacy 的同类工作大量记录在动态 metadata 的 `DrawMaterialToRenderTarget` 下，两个模式的命名与嵌套方式不同。但它们至少证明 RDG 路径没有摆脱 Canvas flush，且 RDG 最慢帧中单个 `CanvasFlush` 可达到约 **1.04 ms**。

Legacy 的 GPU `DrawMaterialToRenderTarget` 由于动态 metadata 被拆成大量同名聚合行，工具返回的单行 P95 会失真，不应与三条 RDG Pipeline 做直接逐项相加比较。

## 6. 最慢帧下钻

### RDG：frame 963，19.254 ms

- `GameThreadWaitForTask`：13.604 ms
- RenderThread `FDeferredShadingSceneRenderer_Render`：9.102 ms
- `FRDGBuilder::CollectResources`：1.628 ms
- `PainterCompositePipeline`：1.606 ms
- 其中 `CanvasFlush`：1.041 ms
- `PressurePairPipeline`：0.658 ms
- RHIThread 同时存在多段 `WaitForTasks`、`RHI_Translate` 与提交等待

### Legacy：frame 1151，18.760 ms

- `GameThreadWaitForTask`：12.146 ms
- RenderThread `FDeferredShadingSceneRenderer_Render`：6.168 ms
- `FRDGBuilder::CollectResources`：1.872 ms
- `FRDGBuilder::AllocateTransientResources`：1.541 ms
- 多个 `CanvasDrawTiles` 约为 0.84–0.88 ms
- `NinjaLiveComponent`：0.759 ms

### 无模拟：frame 955，18.716 ms

- RenderThread `FDeferredShadingSceneRenderer_Render`：13.596 ms
- `GameThreadWaitForTask`：13.480 ms
- `GPUBound_WaitingForGPUForOcclusionQueries`：3.574 ms
- `InitViews` / `VisibilityCommands`：约 3.50 ms

三份 Trace 的单个最慢帧都混有引擎通用的 GPU、可见性与提交等待，因此不能用最大值单独评价 Fluid。平均值和百分位更可靠；它们均指向 RDG 回退。

## 7. 源码归因

当前 `MyNinjaFluidRenderPipeline.cpp` 的 `MyAddMaterialPass`：

- 在每个 pass 内调用 `FCanvas::Create`。
- 每次绘制一个全屏 `FCanvasTileItem`。
- 每个 pass 调用 `Canvas.Flush_RenderThread(GraphBuilder, false)`。

这与 Trace 中每帧约 7 次 `CanvasDrawTiles` 完全吻合。RDG 三组管线包含：

- Advection + Divergence：2 次 Canvas pass。
- PressureCycle1 + PressureCycle2：2 次 Canvas pass。
- Painter + Painter + Composite：3 次 Canvas pass。

此外：

- Advection/Divergence、Pressure、Painter/Composite 各自创建一个独立 `FRDGBuilder` 并立即 `Execute`，没有形成一个完整 Fluid Step 的统一 Graph。
- 每组执行前会对外部 RenderTarget 调用 `FlushDeferredResourceUpdate`，执行后又调用 `UpdateResourceImmediate(false)`。
- `MyAddTextureReadBarrier` 使用 `ERDGPassFlags::Raster | ERDGPassFlags::NeverCull` 创建空 Raster pass。按当前组合，每个完整模拟步骤最多会插入约 6 个这类人为边界。

这套结构保留了 Canvas 的大部分成本，又额外增加 RDG 管理与边界成本，是本轮回退最可信的根因。

## 8. 优化优先级

### P0：改为真正的统一 RDG Fluid Step

1. 用单个 `FRDGBuilder` 覆盖 Advection、Divergence、Pressure、Painter、Composite 的整个模拟步骤。
2. 不在每个材质 pass 中创建/刷新 `FCanvas`；改为真正的 RDG fullscreen raster pass，或进一步改成 Compute pass。
3. 通过 RDG pass parameters 显式绑定输入纹理 SRV 和输出 RenderTarget，让 RDG 自动建立依赖与资源转换。
4. 删除仅用于“制造屏障”的空 `NeverCull` Raster pass。

### P1：减少资源和提交边界

1. 检查 `FlushDeferredResourceUpdate` 是否只需在资源首次创建/尺寸变化时执行。
2. 避免每组管线结束后都调用 `UpdateResourceImmediate(false)`；尽量在完整 Fluid Step 前后各处理一次。
3. 优先优化 `PressurePairPipeline`，它是三条新管线中成本最高、尾部最重的一条。
4. 增加细粒度 GPU scope：Graph setup、外部纹理注册、每个真实 pass、barrier、Execute，确认 0.58 ms 的具体构成。

### P2：验收方式

1. 修复后继续保留三路：RDG、Legacy、无模拟。
2. 固定分辨率、相机、输入、Fluid RT 尺寸、压力迭代数和 Niagara 设置。
3. 每种模式至少重复 3 次，并交错录制顺序，避免首次运行、温度和后台负载偏差。
4. 以每轮 Bookmark 稳态 A/B 增量的中位数作为最终判定。
5. 验收门槛建议：RDG 相对 Legacy 平均至少降低 **0.3 ms/帧**，且 P95/P99 不回退。

## 9. 最终判定

| 项目 | 判定 |
|---|---|
| CPU 包装成本 | RDG 有改善 |
| Canvas 绘制数量 | 未减少，仍约 7 次/帧 |
| 平均帧时间 | RDG 回退 0.582 ms/帧 |
| P95 / P99 | RDG 明显回退 |
| 是否切换默认路径 | **否，继续使用 Legacy** |
| 下一轮首要目标 | 移除 pass 内 FCanvas，并合并成单个真实 RDG Graph |

## 10. 统一 Graph 结构优化复测

2026-09-16 已完成第一阶段结构修正：Output、Painter、Composite、Advection、Divergence 与全部 Pressure ping-pong 合并到单个 `FRDGBuilder`；外部纹理只注册一次，材质参数集合只统一刷新一次，稳态下不再重复执行无效的 RenderTarget 资源更新。压力迭代使用逐轮 MID 参数快照，保留原有参数时序；材质输入依赖改为不绑定 RenderTarget 的 `SkipRenderPass` 显式 SRV 访问 pass，避免旧空 Raster RenderPass，同时弥补 `FCanvas` 材质读取对 RDG 不可见的问题。验证开关打开时仍回退到原逐段双路径比较流程。

验证结果：

- `FluidTestEditor Win64 Development` 完整编译成功。
- `FluidTest.NinjaLive` 两项自动化测试均通过。
- D3D12/SM6、四个 RDG 开关同时启用、`r.RDG.Debug=1` 连续运行 200 帧，无 RDG 断言、资源访问错误或退出异常。
- RenderOffscreen 1280×720 下交错采集 3 组 Unified / Legacy CSV；每轮 1000 帧，丢弃前 200 帧后取 750 帧。相邻配对的 Unified − Legacy 平均帧时间差分别为 **-0.699 ms、+0.424 ms、-0.059 ms**，中位数为 **-0.059 ms**；P95 差值中位数为 **-0.024 ms**，P99 为 **-0.114 ms**。

这说明三组独立 Graph、重复资源更新与旧空 RenderPass 导致的明显回退已基本消除，但结果仍受运行顺序影响，且没有达到平均至少快 0.3 ms 的验收门槛。`CanvasDrawTiles` 仍约为 7 次/帧；当前实现只是统一了 Graph 与依赖管理，并未完成材质逻辑到 Global Shader / Compute 的迁移，因此默认路径继续保持 Legacy。下一阶段应直接消除每个材质 pass 内的 `FCanvas`，不再继续微调 Canvas 外层包装。

## 11. Material / Compute 双后端迁移

2026-09-16 已新增独立的 `MySimulationBackend` 实例选项，包含 `Material` 与 `Compute` 两种模式，默认保持 `Material`。原有 `MyRenderPipelineMode` 继续只控制 Material 后端内部的 Legacy / RDG 选择，因此现有实现、现有控制台变量和回退路径均被保留，两个选项不会混用。

Compute 后端使用 `FMaterialShader` Compute permutation 直接编译现有 Fluid 材质表达式，继续复用原材质的 Material Function、静态开关、纹理参数和 MID 参数。这样无需手工维护另一份约 600 节点的 HLSL 实现，同时移除了模拟 pass 内的 `FCanvas`。核心 `PainterOffsetFirst → PainterOffsetSecond → CompositeAndGradient → Advection → Divergence → PressureCycle1/2` 运行在同一个 RDG Compute Graph 中；Collision Painter Line/Dot、外部 RT 导出和 Output 也已接入 Compute。目标格式不支持所需 typed UAV load/store，或材质缺少 Compute permutation 时，会自动回退到原 Material raster 绘制。

为支持独立 Graph 中可能由 Output 材质隐式请求的全局场景纹理，Compute pass 在创建 `SceneTexturesStruct` 前初始化 `FRDGSystemTextures`。RenderTarget 创建同时启用 UAV，Compute shader 按原材质 BlendMode 复现 Opaque、Translucent 与 Additive 写入语义，并保持 PostProcess/不透明材质的 Alpha 与 Canvas 路径一致。

验证结果：

- `FluidTestEditor Win64 Development` 完整编译成功，七个实际 Fluid 材质的 Compute permutation 均成功编译。
- 干净编辑器进程中确认 PainterOffsetFirst、PainterOffsetSecond、CompositeAndGradient、Advection、Divergence、PressureCycle1、PressureCycle2、CollisionPainter Line、CollisionPainter Dot 与 Output 实际进入 Compute，未发生 fallback。
- Composite、Advection、Painter 各抽检 262,144 像素，RGBA 最大误差均为 0；2048×2048 的 PressureDivergence 与 PressureDivergenceTemp 全图导出差异为 0。
- Output 在 512×512 下对 Material 与 Compute 做 262,144 像素 raw readback，RGBA 最大误差为 `(0, 0, 0, 0)`，超出 `1e-5` 的像素为 0。
- `FluidTest.NinjaLive.CachedMaterialParameters` 与 `FluidTest.NinjaLive.TempArraySlots` 自动化测试均通过。

当前结论是迁移所需的双后端结构与数值一致性已经建立，但尚未完成 Compute 相对 Material 的正式稳态性能复测。因此默认后端仍为 `Material`；可在实例上把 `MySimulationBackend` 切到 `Compute` 做后续 A/B 性能采集，确认收益后再讨论是否调整默认值。

## 12. Compute / Material / 无效果 Trace 复测

2026-09-16 18:08–18:09 新增三份 Trace，继续使用 `ShallowWater_Enter` / `ShallowWater_Exit` Bookmark，并从区间首尾各裁掉 1 秒。身份不是按文件顺序猜测，而是由 Trace 事件确认：

| 模式 | Trace | 身份依据 | 稳态窗口 | 帧数 |
|---|---|---|---:|---:|
| 优化后 Compute | `20260916_180830_3E6890.utrace` | 存在 `FluidTest.NinjaLive.UnifiedFluidStep`，`CanvasDrawTiles` 为 0 | 42.179–46.187 s | 301 |
| 优化前 Material | `20260916_180910_2A6B60.utrace` | 每帧 7 次 `CanvasDrawTiles`、7 次 `DrawMaterialToRenderTarget` | 19.996–24.454 s | 351 |
| 无效果基线 | `20260916_180945_0D9490.utrace` | 不存在 Fluid / NinjaLive 模拟事件 | 17.725–21.168 s | 292 |

### 12.1 稳态帧时间

| 指标 | Compute | Material | 无效果 |
|---|---:|---:|---:|
| 平均帧时间 | **13.244 ms** | 12.664 ms | 11.706 ms |
| P50 | **13.072 ms** | 12.501 ms | 11.529 ms |
| P95 | **15.000 ms** | 14.628 ms | 13.415 ms |
| P99 | **15.614 ms** | 15.278 ms | 14.497 ms |
| 最大值 | 16.183 ms | 15.844 ms | 15.434 ms |
| 推算平均 FPS | **75.51** | 78.96 | 85.43 |
| 超过 20 ms | 0 | 0 | 0 |

Compute 相对 Material：

| 指标 | 差值 | 相对变化 |
|---|---:|---:|
| 平均帧时间 | **+0.579 ms** | **+4.57%** |
| P50 | +0.571 ms | +4.57% |
| P95 | +0.371 ms | +2.54% |
| P99 | +0.336 ms | +2.20% |
| 平均 FPS | **-3.45 FPS** | -4.37% |

模拟相对无效果基线：

| 指标 | Material 增量 | Compute 增量 |
|---|---:|---:|
| 平均帧时间 | **+0.958 ms** | **+1.538 ms** |
| P50 | +0.973 ms | +1.543 ms |
| P95 | +1.213 ms | +1.585 ms |
| P99 | +0.781 ms | +1.117 ms |
| 平均 FPS | -6.46 FPS | -9.92 FPS |

这一轮 Compute 的平均、P50、P95、P99 全部慢于 Material，方向一致，不能解释为单个离群帧。它也没有达到上一轮建议的“相对 Material 至少降低 0.3 ms/帧”验收线；实际结果反向回退 0.579 ms/帧。

### 12.2 CPU 收益确实存在

下表将每个 scope 在同一帧内的所有实例求和，再按稳态帧数归一化：

| CPU Scope | Compute 平均/帧 | Material 平均/帧 | 变化 |
|---|---:|---:|---:|
| `NinjaLiveComponent` | **0.311 ms** | 0.364 ms | **-0.053 ms（-14.6%）** |
| `FluidSim_MyCoreFluidsimOPs` | **0.164 ms** | 0.202 ms | **-0.038 ms（-18.8%）** |
| `CanvasDrawTiles` | **0 ms** | 1.103 ms | **-1.103 ms** |
| `DrawMaterialToRenderTarget` | **0 ms** | 0.087 ms | **-0.087 ms** |
| `FluidTest.NinjaLive.UnifiedFluidStep` | 0.319 ms | 不存在 | Compute 每帧 3 个记录实例 |

因此 Compute 后端已经完成上一轮最重要的 CPU 目标：7 次/帧 Canvas 绘制及其 Kismet 包装全部消失。`NinjaLiveComponent` 与核心模拟 CPU scope 也分别下降约 15% 和 19%。`UnifiedFluidStep` 与其他 scope 可能嵌套或跨线程，不能与表中其他行直接相加；这里只把它作为 Compute 路径已生效的身份和成本证据。

### 12.3 回退来自 GPU / RenderThread 等待链

| 等待/渲染 Scope 平均/帧 | Compute | Material | 无效果 |
|---|---:|---:|---:|
| `GPUBound_WaitingForGPUForOcclusionQueries_SeeGPUTrack` | **7.745 ms** | 5.571 ms | 6.583 ms |
| `FDeferredShadingSceneRenderer_Render` | **10.594 ms** | 8.481 ms | 9.471 ms |
| `GameThreadWaitForTask` | **10.291 ms** | 9.676 ms | 9.007 ms |

Compute 相对 Material 的 GPU 等待平均增加 **2.175 ms**，RenderThread inclusive 时间增加 **2.113 ms**，最终传导为 GameThread 等待增加 **0.615 ms**，与整帧回退 **0.579 ms** 基本一致。三路最慢稳态帧也都由可见性任务、遮挡查询 GPU 等待和 RHI 提交等待主导，而不是 GameThread 计算热点。

Compute Trace 的 GPU digest 中，`FluidTest.NinjaLive.UnifiedFluidStep` 共 1,810 次，单次平均 **1.641 ms**、P95 **1.643 ms**。该 GPU scope 是全 Trace 聚合而非 Bookmark 窗口值，不能与帧差严格相减，但它与 Compute 相对无效果的整帧增量 **1.538 ms** 数量级一致，是当前最强的 GPU 归因证据。

### 12.4 判定与下一步

本轮结论是：**Compute 后端消除了 Canvas CPU 成本，但 GPU 代价抵消并超过 CPU 收益；当前版本仍不应切为默认后端。** Material 相对无效果的持续成本约 0.96 ms/帧，Compute 约 1.54 ms/帧。

下一步优先级：

1. 在 `UnifiedFluidStep` 内为 Painter、Composite、Advection、Divergence、每轮 Pressure、RT 导出分别增加 GPU scope，直接定位 1.64 ms 的构成。
2. 检查 Compute pass 是否都落在预期队列、是否存在不必要的 UAV barrier、外部 RT 拷贝/导出或串行化；不要再投入 Canvas CPU 外层微调。
3. Shader 预热后按 Compute → Material → 无效果 → Material → Compute 的顺序交错采集，每种至少 3 轮，以相邻配对差值的中位数验收。当前每种只有一份、录制顺序固定，足以判定本次 Trace 没有显示收益，但不足以量化小于约 0.3 ms 的稳定差异。

Trace 质量说明：三份都包含 CPU、GPU、Frame、Bookmark、RDG 与 RHICommands。Material/无效果 Trace 的解析日志有未启用 MemAlloc provider 的 tag 错误，和本轮 CPU/GPU/Frame 结论无关；无效果 Trace 另有最大约 0.2 ms 的 GPU 时间线交错警告，因此没有用其中的单个 GPU 事件做直接归因。

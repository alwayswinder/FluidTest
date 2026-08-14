# FluidNinjaLive 核心复刻分析报告

> 目标：用 C++ 复刻 FluidNinjaLive 插件的核心功能（NinjaLive Actor 与 NinjaLiveComponent），逐步替换蓝图实现。
> 项目：FluidTest（UE 5.7），蓝图目标：`/Game/FluidNinjaLive/NinjaLive.NinjaLive`
> 文档版本：v0.1（初步分析）

---

## 1. 插件本质

**FluidNinjaLive 是一个纯蓝图 + 材质实现的 GPU 2D 流体模拟插件**（Stam 1999 "Stable Fluids" 风格 Navier-Stokes 求解器）。全插件无 C++/Python/HLSL 源码，全部逻辑在：

- `NinjaLive.uasset`（2.3MB）— **主 Actor 蓝图**（继承 `Actor`）
- `NinjaLiveComponent.uasset`（11.9MB）— **核心组件蓝图**（继承 `ActorComponent`，含全部求解逻辑）
- `Core/NinjaLiveFunctions.uasset`（923KB）— 蓝图函数库（继承 `BlueprintFunctionLibrary`）
- `Core/FluidSim/` — 求解 Pass 材质（Advection / Divergence / Pressure / CompositeAndGradient / CollisionPainter）
- `Core/` — 辅助：MemoryPoolManager（RT 池）、PresetManager（预设）、Interface、GUI、Niagara 画笔

## 2. GPU 求解器核心（Stam 风格）

求解在 RenderTarget 上进行，每个 Pass 是一个材质（`DrawMaterialToRenderTarget`）。标准求解步骤：

| 步骤 | 材质 | 作用 |
|---|---|---|
| 1. 输入/画笔 | `M_CollisionPainter` / 画笔材质 | 把密度/速度/碰撞绘制进缓冲区 |
| 2. 平流 | `M_Advection` | 密度与速度沿速度场平流（半拉格朗日） |
| 3. 散度 | `M_Divergence` | 计算速度场散度（不可压缩约束准备） |
| 4. 压力求解 | `M_Pressure`（Solver1 / Solver2 两步交替） | 雅可比迭代求解泊松方程，可配置迭代次数 |
| 5. 梯度修正+合成 | `M_CompositeAndGradient` | 速度 -= ∇p 保证无散度，合成密度输出 |
| 6. 反馈/输出 | 输出材质 / Niagara | 把 RT 结果显示到网格、材质或 Niagara 粒子 |

**关键 RenderTarget（缓冲区）：**
- `RT_VelocityDensity` / `NinjaVelocityDensityBuffer` — 速度+密度（RG 存速度，B 存密度）
- `RT_Pressure` / `NinjaPressureBuffer` — 压力
- `RT_PressureDivergence` / `NinjaPressureDivergenceBuffer` — 压力散度
- `RT_Painter` / `PaintBuffer` — 画笔缓冲区（支持双缓冲 `EnablePainterDoubleBuffering`）
- `RT_Advection`、`RT_Composite` — 中间缓冲区
- 压力求解支持 `HalfResPressureAndDivergenceBuffers`（半分辨率加速）与 `PressureSolver2_KernelReduction`

**材质参数（从 uasset 提取）：**
- `M_Advection`：`velocity`、`density`、`Input`、`Output`、`TexelSizeMult`、`Texture`
- `M_Pressure`：`Divergence`、`DivergenceFeedback`、`Pressure`、`PressureEdgeMasking`、`PressureFeedback`、`TexelSize`、`TexelSizeMult`

## 3. 组件蓝图功能结构（从符号提取）

### 3.1 核心变量（分类）
- **求解**：`FluidSolver1Iterations`、`PressureSolver1_MaxIterations`、`PressureSolver2_MaxIterations`、`PressureSolver2_KernelReduction`、`HalfResPressureAndDivergenceBuffers`、`DisablePressureEdgeMasking`、`MakePressureAvailableForNiagara`
- **渲染目标**：`AcquireRenderTargetsFromPool`、`CreateOrAcquireRenderTargets`、`RenderTargetList`/`RenderTargetListItem`、`ExternalRenderTargets`、`InternalRenderTargetsToExport`、`DrawInternalRenderTargetToExternal`
- **画笔/输入**：`PaintBrush`、`BrushDensityNoiseFreq/Scale`、`BrushVelocityNoiseFreq/Scale`、`BrushSensitivityToVelocity`、`BrushVelocityClamp`、`BrushVelocityPow`、`GlobalBrushScale`、`DampenBrush`、`DampenBrushBelowThisVelocity`、`KillBrushBelowThisVelocity`、`MuteBrush`、`BrushFadeOutTimer`、`FadeTimeOfBrush`、`FadeTimeOfCanvas`
- **密度/速度输入**：`DensityTemplate`、`DENSITYTEXTURE`、`DensityTxtMult/Scale/OffsetX/OffsetY`、`DensityInputNoiseAmp/Freq/Scale/Tile/Speed`、`DensityNoiseAmount`、`LoadDensityInputTexture`、`LoadVelocityInputTexture`、`VelocityTemplate`、`GenerateVelocity`、`RandomizeNoiseOffsets`、`RandomizeDensityTextureOffset`
- **碰撞/追踪**：`CollisionChannels`、`CollisionMask`、`CollisionChannel`、`CustomTraceSourcePosition`、`CUSTOMLINETRACESOURCE`、`TraceMouse`、`TraceOverlap`、`LineTraceByChannel`、`NinjaLIVETraceExclude`、`FluidTrace`（ECC_GameTraceChannel1）、`PreferredTraceChannelName`
- **量化**：`Quantizer`、`QuantizerMode`、`QuantizerStepSize`、`QuantizerValues`、`QuantizerAxisIgnore`、`QuantizerIgnoresThisAxis`
- **淡出/生命周期**：`FadeOutForceDisable`、`FadeOutStopTicking`、`FadeOutTime`、`FadeOutValuePerCycle`、`InactiveBehaviour_Enum`
- **物理/反馈**：`Experimental_PressureFeedback`、`Exp_PressureFeedbackComponent`、`GetPhysicsLinearVelocity`、`MultiObjectVelocity`、`OffsetFromSimAreaMotion`、`OffsetVector`
- **Niagara**：`NiagaraBasedPainter`、`NS_Painter_v2_Dot`、`NS_Painter_v2_Line`、`SetAgeUpdateMode`、`ENiagaraAgeUpdateMode`
- **材质**：`MI_Advection`、`MI_Divergence`、`MI_Pressure_Solver1`、`MI_Pressure_Solver2_Step1/Step2`、`MI_CompositeAndGradient`、`MI_CollisionPainter_Dot/Line/Offset`、`MI_PaintDensityBuffer`、`MI_PaintVelocityBuffer`、`MI_PressureBuffer`、`MI_VelocityBuffer`、`MI_DensityBuffer_*`（UVflip 变体用于移动端）

### 3.2 关键函数（从符号提取）
- `TraceMouse` / `TraceOverlap` — 屏幕→世界射线检测，返回 `HitUV`、`SimHitByMouse`、`TouchValid`
- `PaintDensityBuffer` / `PaintVelocityBuffer` — 用画笔材质绘制输入
- `AcquireRenderTargetsFromPool` / `CreateRenderTarget` / `CreateOrAcquireRenderTargets` — RT 生命周期管理
- `IterationLogic1` / `IterationLogic2` — 压力求解迭代编排
- `DrawMaterialToRenderTargetSwitch` — 按平台/设置选择 Pass 材质
- `SetBrushDensityParams1/2/3`、`SetBrushSizeCoeff` — 画笔参数设置
- `SetTraceMeshProperties`、`SetVariableTextureRenderTarget` — TraceMesh 与输出设置

## 4. 周边子系统

| 资产 | 类型 | 作用 |
|---|---|---|
| `NinjaLive_MemoryPoolManager` | 蓝图 | RenderTarget 池，`AcquireRenderTargetsFromPool` 复用 RT 减少分配 |
| `NinjaLive_PresetManager` | 蓝图 | 预设（DataTable `DT_NinjaLive_*`）读写与套用 |
| `NinjaLive_InterfaceController` | 蓝图 | 多组件统一控制接口 |
| `NinjaLive_Utilities` / `WriteDataTableUtility` | 蓝图 | 工具函数 |
| `NinjaLiveInterface` | 蓝图接口 | 组件间通信接口 |
| `NinjaLiveGUI`（3.7MB） | 蓝图 | 编辑器/运行时 GUI（大量 UI） |
| `NinjaRelativeVelocityOffsetComponent`（417KB） | 蓝图组件 | 相对速度偏移 |
| `NinjaLiveTraceMesh` | 蓝图 | 射线检测可视化网格（半透明面板） |
| `NS_Painter_v2_Dot/Line` | Niagara 系统 | 画笔粒子（Niagara 绘制） |
| 枚举：`UserInput_Enum`、`QuantizerMode`、`QuantizerAxisIgnore`、`SimPrecision_Enum`、`InactiveBehaviour_Enum`、`SingleObjectType_Enum` | 枚举 | 配置选项 |

## 5. C++ 复刻方案

### 5.1 总体策略：由外到内，分层替换

**不一次性替换**，而是分层推进：先建 C++ 骨架承载逻辑，再逐个把蓝图功能迁移为 C++，最后可选把材质 Pass 换成 Compute Shader。

**两种技术路线：**
- **路线 A（推荐先做）**：C++ 组件驱动现有材质 Pass（`DrawMaterialToRenderTarget`），与蓝图版行为一致、可对照验证。材料（材质/RT）直接引用现有 `FluidSim` 资产。
- **路线 B（后续优化）**：用 RDG + Compute Shader（HLSL）重写求解 Pass，去掉材质依赖，性能更好、可做高精度求解。

### 5.2 建议的 C++ 模块结构

```
Source/FluidTest/                      （或独立插件 FluidNinjaCore）
├── Public/
│   ├── FluidSim/
│   │   ├── NinjaFluidSolver.h         // 核心求解器：RT 管理 + Pass 编排（不含 Actor/Component）
│   │   ├── NinjaFluidSettings.h       // 设置结构（对齐蓝图变量）
│   │   ├── NinjaRenderTargetPool.h    // RT 池（复刻 MemoryPoolManager）
│   │   ├── NinjaFluidEnums.h          // 枚举（UserInput/QuantizerMode/SimPrecision...）
│   │   └── NinjaFluidMaterials.h      // 材质引用封装（Pass 材质 + 参数设置）
│   ├── NinjaLiveActor.h               // ANinjaLiveActor（对齐蓝图主 Actor）
│   └── NinjaLiveComponent.h           // UNinjaLiveComponent（对齐蓝图组件）
└── Private/
    ├── FluidSim/
    │   ├── NinjaFluidSolver.cpp       // Advection→Divergence→Pressure→Gradient 编排
    │   ├── NinjaRenderTargetPool.cpp
    │   └── NinjaFluidMaterials.cpp
    ├── NinjaLiveActor.cpp
    └── NinjaLiveComponent.cpp         // Tick 驱动求解、输入/画笔/碰撞逻辑
```

### 5.3 分阶段实施计划

**阶段 0：基础设施（本次之后开始）**
- [ ] 建立 C++ 模块骨架与 `Build.cs` 依赖（`RHI`、`RenderCore`、`Engine`、`Slate`）
- [ ] `NinjaFluidEnums.h`：从蓝图枚举对齐（UserInput_Enum、QuantizerMode 等）
- [ ] `NinjaRenderTargetPool`：RT 池（创建/复用/释放）

**阶段 1：核心求解 Pass（C++ 驱动现有材质）**
- [ ] `NinjaFluidSolver`：封装 Advection / Divergence / Pressure(Solver1+Solver2) / CompositeAndGradient / CollisionPainter 五个 Pass
- [ ] 复刻迭代逻辑：`FluidSolver1Iterations` + `PressureSolver2` 两步交替
- [ ] 支持半分辨率压力缓冲、UVflip 变体（移动端）
- [ ] **验证**：用 C++ 求解器 + 蓝图材质跑通一帧，输出与蓝图版截图对比

**阶段 2：组件与 Actor 骨架**
- [ ] `UNinjaLiveComponent`：Tick 驱动求解；暴露蓝图变量（可编辑属性）
- [ ] `ANinjaLiveActor`：挂载组件 + TraceMesh 平面 + 材质输出
- [ ] **验证**：关卡中放置 C++ Actor，替换蓝图版 NinjaLive，视觉效果一致

**阶段 3：输入与画笔**
- [ ] 鼠标/触摸 → 屏幕射线 → `TraceMouse`/`TraceOverlap` 复刻
- [ ] 画笔：Density/Velocity 绘制（`PaintBuffer` 双缓冲）、噪声参数、刷子大小/衰减
- [ ] 碰撞：`CollisionPainter` 绘制障碍物遮罩
- [ ] **验证**：在 M_Test 关卡里用 C++ 组件画流体，对照蓝图版行为

**阶段 4：预设与外部接口**
- [ ] `NinjaPreset` 数据表读写（DataTable `DT_NinjaLive_*`）
- [ ] 外部 RT 导入导出（`ExternalRenderTargets`）
- [ ] Niagara 输出（`MakePressureAvailableForNiagara`）

**阶段 5：优化（可选，路线 B）**
- [ ] RDG + Compute Shader 重写求解 Pass
- [ ] 多分辨率/自适应精度（对齐 `SimPrecision_Enum`）

### 5.4 核心数据流（C++ 版 Tick 流程）

```
Tick()
 ├─ 1. 收集输入：鼠标/触摸命中 → HitUV；物理速度（GetPhysicsLinearVelocity）
 ├─ 2. 绘制输入：CollisionPainter / PaintDensity / PaintVelocity → PaintBuffer
 ├─ 3. Advection：velocity += f(velocity, density)  （RT_VelocityDensity）
 ├─ 4. Divergence：div = ∇·v → RT_PressureDivergence
 ├─ 5. Pressure Solve：迭代 Solver1 / Solver2（N 次）→ RT_Pressure
 ├─ 6. CompositeAndGradient：v -= ∇p；density 合成 → 输出 RT
 └─ 7. 输出：材质/网格显示；Niagara 读取（可选）
```

## 6. 参考

- Jos Stam, "Stable Fluids", SIGGRAPH 1999 — 本插件求解器的理论基础（半拉格朗日平流 + 雅可比压力迭代）
- 材质 Pass 结构 = Stam 的 GPU 化实现（advection → divergence → pressure → gradient subtraction）
- 蓝图依赖链：`NinjaLive(Actor)` → `NinjaLiveComponent` → `FluidSim/*` 材质 + `NinjaLiveFunctions` + `MemoryPoolManager`

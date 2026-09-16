# FluidNinjaLive 提交审查报告（`48582de..0350522`）

> 审查时间：2026-09-16。审计对象为 `0350522`（工作树干净）。
> 本报告只读审查，未修改任何源码；结论均可按文中 `文件:行号` 复核。

## 1. 审查范围与方法

| 项目 | 内容 |
| --- | --- |
| 提交范围 | `48582de`（不含）→ `0350522`（含），共 9 个提交 |
| 涉及文件 | `Source/FluidTest/*`（17 个）、`Shaders/Private/MyNinjaOutputDiff.usf`、`Config/DefaultEngine.ini`、`FluidTest.uproject`、`Plugins/SpecialAgentPlugin-main/...`、`Docs/*`、`AGENTS.md` |
| 方法 | 逐提交 `git diff/show`；HEAD 全文阅读；与 `48582de` 基线逐函数对比；对引擎侧 API 语义做源码核对（`E:\Unreal\UE_5.7`） |
| 编译证据 | `Binaries/Win64/UnrealEditor-FluidTest.dll` 时间戳 `2026-09-15 18:49:39` 晚于最后一次源码修改 `18:49:26` → HEAD 状态确实编译通过 |
| 未覆盖 | 无编辑器访问：蓝图函数图、关卡实例属性基线未核（见 §10） |

**范围定性**：这 9 个提交并非"全部是性能优化"，实际构成如下。

| 提交 | 性质 |
| --- | --- |
| `1f395f4` 修复流体资源与组件生命周期管理 | 生命周期修复 + 预设/模板加载器重写 |
| `c12e250` 整理流体组件结构 | 结构重构（拆文件、槽位语义、`Transient` 收紧、`EndPlay` 清理） |
| `56bc58e` 修复画笔强度上限被 Transient 丢弃 | 默认值修复 |
| `7a0ed69` 恢复被 Transient 丢弃的蓝图默认值 | 默认值修复 + 建立蓝图契约基线 |
| `75e3587` 优化流体模拟并修复运行时状态恢复 | 性能优化 + 运行时状态修复 |
| `20f450e` 完成首轮性能优化与RDG输出试迁移 | 性能优化 + RDG 阶段一（含 `uproject`/`Build.cs` 变更） |
| `86eb6a7` / `61192e3` / `0350522` | RDG 阶段二/三/四（新增可选路径） |

## 2. 总体结论

1. **未发现阻断级正确性 bug**，默认路径语义未被破坏：4 个 RDG 开关默认 `0`、GPU 验证默认关闭、枚举数值零变化、蓝图契约 §4 的组件 68 项与 Actor 15 项覆盖属性**无一项**被 `Transient` 化或收紧为只读。
2. 重构文档《复核发现》6 条中 **4 条已修、2 条只算半修**（§3）。
3. 本轮性能优化**确有净收益**，但 0915 报告 P0（每帧约 7 次 Canvas/RT 往返）未动（§6）。
4. 风险集中在两处：**（a）状态门闩/反射收紧带来的复位与契约缺口；（b）两条交互占槽路径互不感知**（当前配置不触发）。
5. `FluidTest.uproject` 的 `LoadingPhase: Default → PostConfigInit` 是**必要且正确**的改动，不应回退（§7.1）。

## 3. 重构文档《复核发现》逐条核对

来源：`Docs/FluidNinjaLive-Refactor-20260914.md:60-82`。

| # | 原文结论 | HEAD 实际状态 | 证据 |
| --- | --- | --- | --- |
| 1 | `MyEndOverlapComponent` 只在 `ECC_Pawn` 分支释放槽位 | **已修复** | `MyNinjaLiveActor.cpp:884-911` 已无通道分支；新增 `:860-882 MyReleaseSkeletalSlotsForActor`（按 owner 扫 `MySkeletalMeshTempArrayPairs` → `MyReleaseTempArraySlot`） |
| 2 | `MyRePlay` 只清数组不释放槽位 | **已修复** | `MyNinjaLiveComponent.cpp:195` 调 `MyResetTempArraySlots()`；`Interaction.cpp:243-248` 同时清 flags、数组、映射 |
| 3 | `MyProximityCheck` 整套 reset 导致映射被清、登记无法重建 | **半修** | `MyNinjaLiveActor.cpp:311` 仍是整套 `MyResetTempArraySlots()`；两个后果被别处掩盖（`:900-906` 改为按 Actor 释放、不依赖映射键；`:153-156` 重进时 Reset `MyOverlappingActors`）。残留：离开期间 `MyOverlappingActors`/`MyOverlappingComponents`/`MyOverlap1` 保持旧值 |
| 4 | `RT_Output` 创建条件依赖密度输入 | **已修复** | `MyNinjaLiveComponentRendering.cpp:1340-1349`（条件改为 2ndOutput / Niagara / 输出验证开关）；`MyCoreFluidsimOPs` 侧的验证专用创建与回收见 `:167-200` |
| 5 | `EndPlay` 未复位 tick/DoOnce 门 | **半修** | `MyNinjaLiveComponent.cpp:59-65` 复位 5 门 + 2 标志；仍漏 `MySimSpeedAdjustmentPending`(h:511)、`MyCheckTouchOptionsDoOnceClosed`(h:592)、`MyInitDone`(h:433)、`MyMaterialInstacesDone`(h:437)、`MyDynamicSimPositionInitialized`(h:250) → 见 §4 P2 |
| 6 | 被 `Transient` 丢弃的蓝图默认值 | **已修复** | `h:1265 MyBrushStrengthTemp2 = 1.0`；`h:960 MyFluidSolver1Iterations = 5`（已无 `Transient`）；构造函数 `Component.cpp:50-52` 重建 `MyPosition3_2D`/`MyLastPosition3_2D`（10×Transparent）与 `MyListOfAvailableTempArrays`（`Init(true, 40)`） |

## 4. 建议修复项

### P1 `MyForceTrackObjectsWithNocollisionFlag` 已成为死属性（行为语义丢失）

- 位置：`MyNinjaLiveActor.h:193`（属性仍暴露）、`MyNinjaLiveActor.cpp:884-911`（唯一的读取点被删）。
- 证据：`git show 48582de:Source/FluidTest/MyNinjaLiveActor.cpp` 中 `MyEndOverlapComponent` 首段为
  `if (OtherComp->GetCollisionEnabled() == ECollisionEnabled::NoCollision && MyForceTrackObjectsWithNocollisionFlag) { return; }`，
  现在 HEAD 全工程仅有声明、无任何读取。
- 影响：原语义是"物体失去碰撞但开启强制追踪时不停止追踪"；删除后无碰撞对象在 EndOverlap **一定**被清理。该属性列在
  `Docs/FluidNinjaLive-BP-Contract.md` §4 的 Actor 覆盖项中，说明蓝图很可能把它设为 `true`。（旧注释写"未开启强制追踪标志时不做任何事"，
  与代码里的 `&&` 相反，注释本身是错的，删除时不宜以注释为准。）
- 建议：二选一——恢复守卫并修正注释；或明确废弃该属性（从蓝图移除引用）并在文档记录。**建议恢复**，因为"强制追踪"是无碰撞交互体的唯一入口。

### P2 EndPlay/BeginPlay 复位不对称，门闩会永久卡死

- 位置：`MyNinjaLiveComponent.cpp:75-131`（EndPlay）与 `:55-73`（BeginPlay）；`MyNinjaLiveComponent.cpp:467-499`（置位/清位处）。
- 问题：`EndPlay` 清了 `MyTimerSimSpeedAdjustment`，却没有复位 `MySimSpeedAdjustmentPending`（h:511）→ 同一实例再次 BeginPlay 后
  `else if (!MySimSpeedAdjustmentPending)` 恒为假，延迟 `TexelSizeMult` 更新路径（含 `MySimSpeedAdjustmentLatency` 分支）永久失效。
  同类漏复位：`MyCheckTouchOptionsDoOnceClosed`（h:592，导致 `MySingleInput`/`MyTouch` 不再重算）、
  `MyInitDone`/`MyMaterialInstacesDone`（h:433/437，EndPlay 已清空 MID/RT，但 Tick 会立刻放行 `MyAfterReadyCheck` 在半拆解状态执行）、
  `MyDynamicSimPositionInitialized`（h:250）。
- 建议：抽 `MyResetRuntimeState()`，`BeginPlay` 与 `EndPlay` 共用同一份复位清单。

### P3 持续交互路径与重叠路径的槽位互不感知（"一 SKM 一槽"只在单路径内成立）

- 位置：`Interaction.cpp:1033-1113`（`MyManageContinuousInteractions`，`:1048` 整套 reset、`:1109` 登记）、
  `MyNinjaLiveActor.cpp:740-858`（重叠路径占槽）、`:860-882`（按 owner 释放）。
- 机理：`MyManageContinuousInteractions` 先 `MyResetTempArraySlots()`，会清掉重叠路径已占的**槽位与映射**，但不碰 `MyOverlappingActors`；
  而 `MyProcessOverlapActor` 开头 `MyOverlappingActors.Contains(Actor)` 直接 return（`Actor.cpp:749-752`）→ 这些 Actor 在本轮重叠期内
  再也拿不回槽位。反向：同类排除表只排"其他"同类 Actor（`:167-170`），同 owner 检查排在 Pawn/标签分支**之后**（`:698`/`:713` vs `:719-723`），
  owner 自身 SKM 存在被两条路径各占一槽的可能（40 槽双倍消耗 + 同一骨骼重复采样）。
- 触发条件：`MyContinuousInteractionWithOwnerActor = true`（h:298）。该项目 C++ 默认 `false`，且不在契约 §4 覆盖清单 →
  **当前配置不触发**，属潜伏缺陷；但 `c12e250` 的提交说明把"一个 SkeletalMesh 只占一个槽位"写成已修复，口径偏大。
- 建议：`Acquire` 以 `UPrimitiveComponent` 为主键去重（已占则复用原槽），`Release` 按组件而非 owner；或在文档明确该组合不受支持。

### P4 外部 Niagara 组件的副作用未还原

- 位置：`MyNinjaLiveComponentRendering.cpp:1806-1811`（对**其他 Actor** 的 Niagara 组件 `SetForceSolo(true)` + 改 tick 间隔 + `ReinitializeSystem`）；
  `MyNinjaLiveComponent.cpp:95`（EndPlay 只 `MyNiagaraSystemsToDrive.Reset()`）。
- 影响：流体 Actor 销毁后，外部 Niagara 组件仍保持 solo 与强制 tick 间隔（把状态泄漏到别人的对象上）。
- 建议：EndPlay 对 `MyNiagaraSystemsToDrive` 逐个还原（记录原值或 `SetForceSolo(false)` + `SetComponentTickInterval(0)`）。

### P5 `MyResetTempArrays` 仍是蓝图入口但不释放槽位

- 位置：`Interaction.cpp:198-204`（只 `Reset()` 数组）、`MyNinjaLiveComponent.h:341-342`（仍 `BlueprintCallable`）、
  而 `MyListOfAvailableTempArrays`（h:330）已 `Transient`，蓝图侧无法自行修正。
- 影响：任一蓝图函数图若仍调用它，40 槽会被"占用但内容为空"占死，骨骼交互静默失效且无日志（契约 §3 自述"函数图尚未逐个扫描"）。
- 建议：让 `MyResetTempArrays()` 等价于 `MyResetTempArraySlots()`，或加 `UE_LOG(Warning)` + 弃用；同时补扫两个蓝图的函数图。

### P6 离开激活体积时的三份状态漂移（半修项补齐）

- 位置：`MyNinjaLiveActor.cpp:308-312`。
- 现状：只 `MyResetTempArraySlots()`，`MyOverlappingActors` / `MyOverlappingComponents` / `MyOverlap1` 保持旧值；
  期间 `MyOverlap1`（`:909-910`）会因残留组件保持 `true`。当前由 `MyAfterTickDelay` 门控（`Component.cpp:216-217`）挡住，无可见故障。
- 建议：在离开分支同时清空 `MyOverlappingActors` / `MyOverlappingComponents` 并置 `MyOverlap1 = false`，与进入路径（`:153-156`）对称。

### P7（预存瑕疵，顺手可改）`AxisLocked == 4` 魔数

- 位置：`MyNinjaLiveActor.cpp:585`。`4` 即 `EMyQuantizerAxisIgnore::None`（`MyNinjaFluidEnums.h:74`）。
- 该行在 `48582de` 即存在，非本轮引入；但枚举重排会静默改变行为，建议显式写枚举名。

## 5. 需确认项（行为变更未记录）

| # | 位置 | 变更 | 现状与建议 |
| --- | --- | --- | --- |
| C1 | `MyNinjaLiveFunctions.cpp:255-272`、`:177-191` | 预设加载由 `AssetName.Contains(...)` 首个命中改为**必须唯一精确命中**；模板包路径不含对象名时要求恰好 1 个资产 | `Content` 下仅 `DT_NinjaLive_Default`、`DT_NinjaLive_Coastline_Quiet`，与 `DT_<Filter>_<Preset>` 完全同名 → 当前可用。但 **0 命中无任何告警**，失败后 `PresetMap` 为空、全部参数静默回默认值（只有 `Rendering.cpp:1902` 一条 Display 日志）。建议 0 命中打 Error |
| C2 | `MyNinjaLiveActor.cpp:827-845` | 部分骨骼名匹配新增 `break` 去重 | 旧实现同一骨骼命中多个部分名会重复入列，且每次 `Find(true)` 都取到同一个槽（正是 `c12e250` 修的槽位语义 bug）→ 属修 bug，但三份文档均未记录，建议补记 |
| C3 | `MyNinjaLiveComponent.cpp:1193-1244`、`:185-207`、`:231-243` | `MyCheckLODLevel` 不再 `SetActorTickInterval`（只限组件 tick/自定义 Timer）；`MyRePlay` 额外重建 Actor 侧跟踪并同步 Tick 频率；`MyAfterTickDelay` 重新进入时新增 Painter V2 `Activate(false)` | 均为有意变更，建议补进 `AGENTS.md` 的"运行时修正"段 |
| C4 | `MyNinjaLiveFunctions.cpp:415-452` | `MyTraceOverlap` 的 `PainterV2` 参数已完全未使用（`20f450e` 删除 `!HitValidator && !PainterV2` 分支） | 已核对**完全等价**：无命中时旧路径写入 `HitUV=FLinearColor(0,0,0,1)==FLinearColor::Black`、`TracePosition=ZeroVector`、`HitValid=false`，与提前 return 相同；调用方只看 `HitValid`（`Interaction.cpp:490`）。属安全简化，`UFUNCTION` 签名保留即可 |
| C5 | `MyNinjaLiveComponent.h` 全文件 | 127 个属性在 `c12e250` 由 `BlueprintReadWrite` 收紧为 `BlueprintReadOnly + Transient` | 契约 §4 的 68+15 项覆盖属性**无一**被收紧（已机械核对）；但 (a) 契约 §3 中仅被 Get 的 `MyNinjaLiveTraceExclude`（h:492）被收紧，字面违反"不得收紧访问"；(b) `CompileAllBlueprints 0 error` 是 `c12e250` 时点的结论，`7a0ed69`/`75e3587` 之后未重跑；(c) 契约只扫 CDO/组件模板，**关卡实例的 per-instance 覆盖查不出来**，`Transient` 化后会静默丢值。建议：重跑蓝图全量编译 + `manage_blueprint get_graph_details` 补扫函数图 + 扫 `M_Start`/`M_Test` 组件实例 |

## 6. 性能核查

**确认为净收益（已逐条读码核对）**

- 追踪排除 Actor 数组按 `GFrameCounter` 复用（`Interaction.cpp:435-450`）；消费方 `MyTraceOverlap` 只做 const 透传，缓存安全。
- 附加流体标量改走 `InitializeScalarParameterAndGetIndex` 索引缓存（`Rendering.cpp:37-56`、`:58-107`），索引在 MID 重建时 Reset（`:1354-1355`）。
- Painter V2 标量/数组跳过未变化上传（`:994-1015`、`:957-982`）：旧版每帧 `GetAllScalarParameterInfo`（两次 `TArray` 分配）+ 全量 `SetVariableFloat`；新版列表只建一次、只在变化时下发，且在 `MyInitPainterV2`（`:831-842`）与重新激活（`Component.cpp:238-240`）时正确失效缓存。
- `RT_Output` 创建与密度输入解耦（`:1340-1349`）；外部 RT 导出校验只跑一次（`:1138-1151`）。

**仍未兑现（与 `Docs/FluidNinjaLive-Performance-Analysis-20260915.md` 一致）**

- P0：每帧约 7 次 Canvas/RT 往返、pass 数量未减少；P99 增量恶化 17.7%；Niagara CPU dispatch +18.7% 未回查。
- 可再省：`MySingleTargetMode` 相关路径每帧 2-3 次 `GenerateValueArray` 堆分配（`Interaction.cpp:694-697`、`:705-707`、`:986-987`，旧版同样）；`MyForwardScalarParamsToNiagara` 仍每帧对画笔 MID 全量 `GetScalarParameterValue`（检测变化所必需，可改为先比副本）。

**默认路径开销**：RDG 全部关闭时仅剩若干 cvar 读取与 1 次 `std::atomic` 检查（`MyNinjaFluidRenderPipeline.cpp:1031-1045`），可忽略；验证关闭会释放全部对照 RT/MID（`Rendering.cpp:143-180`）。

## 7. RDG 专项核查（单方复核，见 §10 限制）

### 7.1 `LoadingPhase` 改动是正确的（勿回退）

`IMPLEMENT_GLOBAL_SHADER`（`MyNinjaFluidRenderPipeline.cpp:116`）必须在 `CompileGlobalShaderMap`（`LaunchEngineLoop.cpp:3247`）之前完成注册；`Default` 阶段要到 `LoadStartupModules`（`:4617`）才加载 → 打包版会缺该 global shader。`PostConfigInit` 在 `AppInit` 内（`:6762`）加载，早于 `:3247`，因此 `FluidTest.uproject:10` 的改动是**必要**的。`/Project` 着色器目录映射由启动器按目录存在性添加（`:2511-2518`），与本改动无冲突。

### 7.2 与引擎等价模式同构（已核对引擎源码）

- `FCanvas::Create` + `SetRenderTargetRect` + `Flush_RenderThread(GraphBuilder, false)`（`MyNinjaFluidRenderPipeline.cpp:203-223`）与引擎 `Renderer/Public/ScreenPass.h:736` 的 `AddDrawCanvasPass` 完全同构。
- 外部 RT 重复注册安全：`RenderGraphUtils.h:271` 的 `RegisterExternalTexture` 通过 `FindExternalTexture` 去重，`MyAddMaterialPass` 二次注册同一 RT 只返回同一 `FRDGTextureRef`，不会产生双份状态机。
- canvas + MID 的纹理依赖 RDG 无法追踪，代码用显式 `MyAddTextureReadBarrier`（`:225-242`）与 `AddCopyTexturePass`（`:614-615`）补上，这正是像素级等价能成立的关键。
- 差分 shader 对 `abs()` 后的 uint 位模式取 `InterlockedMax`（`Shaders/Private/MyNinjaOutputDiff.usf`）：非负浮点位的字典序等于数值序，做法正确；结果缓冲用 `BUF_SourceCopy` + `AddEnqueueCopyPass` 异步读回，队列满（上限 8）时**跳过采样**而不阻塞 GameThread。
- 模块卸载时序安全：`UnloadModulesAtShutdown`（`LaunchEngineLoop.cpp:5122`）先于 `RHIExit`（`:5157`），FluidTest 作为 RHI/RenderCore 的依赖方先卸载，此时渲染线程仍存活；`MyShutdown` 的 `!GIsRHIInitialized` 早退（`:1050-1053`）亦正确。

### 7.3 RDG 相关小项

- `FluidTest.Build.cs:13` 的 `"Renderer"` 私有依赖**无任何 include 使用**（`RenderCore`/`RHI` 已足够）；而它会让整个 Renderer 模块在 `PostConfigInit` 阶段被提前加载，建议删除。
- `AGENTS.md` 写"验证 readback 限制为最多 **4** 个并发请求"，代码与 0915 报告是 **8**（`MyNinjaFluidRenderPipeline.cpp:137`），文档需统一。
- 诊断数据建模不一致：Output 用扁平属性（h:167-187），Core/Pressure/Painter 用 `FMyNinjaRDGTextureDiffDiagnostics`（h:39-61、h:190-205）。

## 8. 文档与规范一致性

| 位置 | 问题 |
| --- | --- |
| `Docs/FluidNinjaLive-Refactor-20260914.md:23-26` | 表格 302/198/1483 是 `c12e250` 时点数据；HEAD 实测为 333 个 `UPROPERTY`、199 个 `EditAnywhere`、最大实现文件 2126 行。建议标注时点或重测 |
| 同上 §"审查后追加的清理" | 提到的 `MyBuildTraceExcludeActors` 在 HEAD 不存在，实际函数名为 `MyBuildTraceExcludeList`（`MyNinjaLiveComponentRendering.cpp:1842`） |
| `Docs/FluidNinjaLive-Cpp-Core-Implementation-Index.md` / `Technical-Guide.md` | 均未提及新增的 `MyNinjaFluidRenderPipeline` 模块（只在 `AGENTS.md` 与性能报告中有），建议补条目 |
| `Docs/FluidNinjaLive-Refactor-20260914.md:76-80` 与 `BP-Contract.md` §3/§4 | `MyInteractionVolumeTemplate` 的口径不一致（Refactor 说"仍由蓝图写入"，契约两节都未列） |
| `Source/FluidTest/MyNinjaLiveComponent{,.cpp}`、`Interaction.cpp`、`Rendering.cpp` | 三个 `.cpp` 各复制 33-35 行相同 include；三处 `#include "MyNinjaLiveMemoryPoolManager.h"` 零引用；自身头文件未按 `AGENTS.md` 写 `"FluidTest/…"` |
| `Source/FluidTest/MyNinjaLiveComponentTests.cpp:74-83` | 第二个用例硬编码 `/Game/FluidNinjaLive/Core/FluidSim/...` 资产路径并 `TestNotNull`，资产改名或未 Cook 的环境会直接失败（CI 噪声） |

## 9. 验收建议

1. 重跑 `CompileAllBlueprints -ProjectOnly`，并用 `manage_blueprint get_graph_details` 补扫 `NinjaLive` / `NinjaLive_Area_Water_Blueprint` 的**函数图**（契约 §3 待办）。
2. 扫 `M_Start` / `M_Test` 中组件**实例**（非 CDO）的 `My*` 覆盖，确认 `Transient` 化没有静默丢值。
3. 打开 `MyContinuousInteractionWithOwnerActor` 做一次骨骼交互回归（验证 §4 P3）。
4. 做一次 EndPlay → BeginPlay 复用回归（PIE 内重注册/流关卡），验证 §4 P2 的门闩问题。
5. 打包 + Standalone 启动一次，确认 `PostConfigInit` 与 `/Project` global shader 在非编辑器环境可用。
6. 按 0915 报告 P1 回查 Niagara CPU dispatch 回退；继续推进 P0（按 dirty 状态跳过、降频、合并 pass）。

## 10. 限制与未覆盖

- 无编辑器访问：**蓝图函数图**与**关卡实例属性**未核，这是 §4 P1/P5、§5 C5 的唯一堵点，结论依赖已有文档而非实测。
- 本次未重新编译：编译结论由 `Binaries/Win64/UnrealEditor-FluidTest.dll` 时间戳推得。
- RDG 部分（§7）为**单方复核**：原计划的独立交叉核查在收尾前被中止，未产出报告。建议对以下三点补一次独立复核：
  默认关闭时是否真的零额外开销、验证资源的创建/释放是否严格对称、渲染线程捕获的裸指针与 `FMaterialRenderProxy` 是否可能悬垂。
- 组件与 Actor 部分（§3-§6）有独立交叉核查背书，与本次结论一致。

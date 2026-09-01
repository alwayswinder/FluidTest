# FluidTest 项目说明

> 本文件是项目级记忆文件（DSH 原生命名 `AGENTS.md`），每次会话自动加载为项目上下文。
> 保持精简；详细分析见 `Docs/FluidNinjaLive-Remake-Analysis.md`。

## 项目定位

用 **C++ 复刻 FluidNinjaLive 插件核心**（GPU 2D 流体模拟，Stam 风格 Navier-Stokes 求解器），
把原蓝图实现**小步增量迁移**为 C++，迁移一次测试一次。

- 引擎：**UE 5.7**（`EngineAssociation: "5.7"`），模块 `FluidTest`（Runtime）
- 启用插件：`ModelingToolsEditorMode`、`BlueprintAssist`；另含 MCP 插件（见下）
- 目标蓝图（已在 `Content/_MyTest/Fluid/Bp/`）：`NinjaLive`（Actor）、`NinjaLiveComponent`（组件）

## 目录速览

| 路径 | 内容 |
|---|---|
| `Source/FluidTest/` | 迁移用 C++：`MyNinjaLiveComponent`、`MyNinjaLiveActor`、`MyNinjaFluidEnums` |
| `Content/_MyTest/` | 用户测试区：`M_Start.umap`、`M_Test.umap`、迁移目标蓝图 |
| `Content/FluidNinjaLive/` | 原插件资产：教程关卡、UseCases、Volumetrics 体积云/雾/烟 |
| `Plugins/McpAutomationBridge/` | Unreal MCP（WebSocket bridge_hello + automation_request） |
| `Plugins/SpecialAgentPlugin-main/` | SpecialAgent MCP Server（34 个标准工具，HTTP/SSE + JSON-RPC 2.0） |
| `Docs/` | `FluidNinjaLive-Remake-Analysis.md`（复刻分析报告）+ DSH 启停脚本 |

## 迁移规范（核心约定）

- **命名**：C++ 类 `U`/`A` + `My` + 蓝图名；函数 `My` + 蓝图函数名；**函数参数不加前缀**；
  **类成员变量加 `My` 前缀**（避免与蓝图内同名变量冲突）；Category 用 `FluidSim|*`
- **蓝图 VM 限制**：蓝图调用"返回引用"一律按值拷贝，改数据须用 `BlueprintCallable` 操作函数
  或 `BlueprintReadWrite` 变量
- **编译**：编辑器运行中无法命令行编译（Live Coding），需先关编辑器再 `Build.bat`；
  include 路径基于 `Source` 根，写 `"FluidTest/X.h"`
- **注释**：只写功能说明（一句话即可），不粘贴蓝图节点还原；不确定的暂留大概，随迁移补充
- 已迁移：`ResetTempArrays → MyResetTempArrays`、`GetTempArray → MyGetTempArray`、
  `MyAddToTempArray` / `MyClearTempArray` / `MyAppendToTempArray`、40 个 `MyTempArray0~39`、
  `CompareMapLength → MyCompareMapLength`、`VelocityHandlerForSimArea → MyVelocityHandlerForSimArea`、
  `ParsePresetMapAndSetVariables（复合节点）→ MyParsePresetMapAndSetVariables`、
  `CheckComponentOwner（复合节点）→ MyCheckComponentOwner`、`Enable OWNER Input → MyEnableOwnerInput`、
  `CheckTouchOptions（复合节点）→ MyCheckTouchOptions`（+ `MySingleInput` / `MyTouch`）、
  `ProximityActivation-MasterVars-Quantizer-OutMat（复合节点）→ MyProximityActivationMasterVarsQuantizerOutMat*`、
  `EMyQuantizerMode` 枚举 + `MyQuantizerValues`、
  `CreateOrAcquireRenderTargets（复合节点）→ MyCreateOrAcquireRenderTargets`、
  `LoadVelocityInputTexture（复合节点）→ MyLoadVelocityInputTexture`、
  `LoadDensityInputTexture（复合节点）→ MyLoadDensityInputTexture`、
  `LoadTextures（复合节点）→ MyLoadTextures`、
  `CreateDynamicMaterialInstances（复合节点）→ MyCreateDynamicMaterialInstances`、
  `ManageContinuousInteractions（复合节点）→ MyManageContinuousInteractions`、
  `BrushFadeOutTimer（复合节点）→ MyBrushFadeOutTimer`（+ `MyStopUsingPainterCanvasWhenIdle`）、
  `INIT PAINTER V2（复合节点）→ MyInitPainterV2`、
  `CreateOuputMaterialAndSetItOnTargets_Step01 → MyCreateOutputMaterialAndSetItOnTargetsStep01`、
  `CreateOuputMaterialAndSetItOnTargets_Step02 → MyCreateOutputMaterialAndSetItOnTargetsStep02`、
  `CreateOuputMaterialAndSetItOnTargets_Step03 → MyCreateOutputMaterialAndSetItOnTargetsStep03`、
  `PresetLoader → MyPresetLoader`、
  `AfterCreateRT → MyAfterCreateRT`、
  `LOD-DistaceStepsPrecalc（复合节点）→ MyLODDistaceStepsPrecalc`、
  `LOD（复合节点）→ MyLOD`（+ `MyLODCheckFrequency` / `MyMinSamplingFPS`，对应蓝图变量 `LOD-CheckFrequency` / `MinSamplingFPS`；周期检查体为私有 `MyCheckLODLevel`）、
  `AfterBind 初始化流程 → MyAfterBind`、
  `BeginPlay / CheckReady / RePlay 初始化流程 → BeginPlay / MyCheckReady / MyRePlay`、
  `AfterReadyCheck（自定义事件）→ MyAfterReadyCheck`（+ `MyInputFeedbackInterface`，对应蓝图变量 `InputFeedbackInterface`）、
  `AfterTickDelay（事件图）→ MyAfterTickDelay`、
  `ReceiveTick（Tick 事件）→ TickComponent`（+ `MyUseUnrealNativeEventTick` / `MyLimitUnrealNativeEventTick` / `MyDeltaSeconds`，对应蓝图变量同名项；非原生分支循环回调为私有 `MyCustomTick`）、
  `MuteBrush（复合节点）→ MyMuteBrush`、
  `CameraFacing（复合节点）→ MyCameraFacing`、
  `SingleTargetVelocity（复合节点）→ MySingleTargetVelocity`、
  `CoreFluidsimOPs（复合节点）→ MyCoreFluidsimOPs`、
  `Forward SCALAR params to Niagara（复合节点）→ MyForwardScalarParamsToNiagara`、
  `Draw Internal RenderTarget to External RT（复合节点）→ MyDrawInternalRenderTargetToExternal`、
  `SetAdditionalFluidsimParams（复合节点）→ MySetAdditionalFluidsimParams`、
  `DynamicSimspeedAndWorldOffsetAdjustment（复合节点）→ MyDynamicSimspeedAndWorldOffsetAdjustment`、
  `Set Pos,Velocity, Scale arrays to Painter v2（复合节点）→ MySetPosVelocityScaleArraysToPainterV2`、
  `Clear Pos, Velocity, Scale arrays - Painter v2（复合节点）→ MyClearPosVelocityScaleArraysPainterV2`、
  `Build brush-POSITION array（复合节点）→ MyBuildBrushPositionArray`、
  `BrushSizeCoEff（复合节点）→ MyBrushSizeCoEff`、
  `MultiObjectVelocity（复合节点）→ MyMultiObjectVelocity`、
  `SetBrushDensityParams1（复合节点）→ MySetBrushDensityParams1`、
  `SetBrushDensityParams3（复合节点）→ MySetBrushDensityParams3`、
  `DefineLineTracingSource（复合节点）→ MyDefineLineTracingSource`（+ `MyUseCustomTraceSource` / `MyCustomTraceSourcePosition`）、
  `BrushRnd3（复合节点）→ MyBrushRnd3`（参数 `in` → `InColor`）、
  `OverlapArtifactWorkaround2（复合节点）→ MyOverlapArtifactWorkaround2`（+ `MyTracePositionTemp` / `MyLastTracePositionTemp` / `MyPosition1_3D` / `MyLastPosition1_3D`）、
  `TraceObjects2（复合节点）→ MyTraceObjects2`、`NinjaLiveFunctions.TraceOverlap → MyTraceOverlap`（占位，待补全）、
  `TraceObj2（事件图）→ MyTraceObj2`、
  `MousePassTrue（自定义事件）→ MyMousePassTrue`（+ `MyUserInputBrushScale`，对应蓝图变量 `UserInputBrushScale`）、
  `MousePassFalse（自定义事件）→ MyMousePassFalse`（+ `MyOverlapBasedInteraction`，对应蓝图变量 `OverlapBasedInteraction`）、
  `MultiObjectProcessorCycle_3（事件图）→ MyMultiObjectProcessorCycle3`、
  `ForLoopOverlapping（事件图）→ MyForLoopOverlapping`、
  `NoInteraction（事件图）→ MyNoInteraction`、
  `FluidCoreStep（事件图）→ MyFluidCoreStep`、
  `FinalDealRTAndBrush（事件图）→ MyFinalDealRTAndBrush`、
  `PaintLine（事件图）→ MyPaintLine`、
  `BrushSwitch2（复合节点）→ MyBrushSwitch2`（+ `MyPosition1_3D_Static`）、
  `BrushSwitch1（复合节点）→ MyBrushSwitch1`、
  `BrushRnd2（复合节点）→ MyBrushRnd2`（逻辑同 `MyBrushRnd3`，参数 `in` → `InColor`）、
  `BrushRnd1（复合节点）→ MyBrushRnd1`（逻辑同 `MyBrushRnd3`，参数 `in` → `InColor`）、
  `TraceObjects1（复合节点）→ MyTraceObjects1`、
  `TraceGestures（复合节点）→ MyTraceGestures`（返回 `bool` 表示是否走到 out；+ `MyTraceMouse`）、
  `CalcPos1（复合节点）→ MyCalcPos1`（+ `MyOverlapFilterInclusiveBoneNameExact`）、
  `SingleTargetMode（事件图）→ MySingleTargetMode`（+ `EMySingleObjectType` / `MySingleTargetType_LEGACY` / `MySingleTargetModeSkeletalMeshIndex_LEGACY`）、
  `CalcPos2（复合节点）→ MyCalcPos2`、
  `CheckValidity2（复合节点）→ MyCheckValidity2`

## 常用操作

- 构建：关闭编辑器 → `Build.bat`（或 `Build.bat FluidTestEditor Win64 Development`）
- 操控编辑器：通过 MCP 插件（`manage_level`、`control_actor`、`manage_asset`、`manage_blueprint` 等）

## 技能索引（DSH）

- `blueprint-to-cpp-migration` — 迁移工作的唯一权威规范，所有迁移代码必须遵守
- `mcp-automation-bridge` — 用 McpAutomationBridge 控制编辑器
- `special-agent-mcp` — 用 SpecialAgent MCP 控制编辑器（Python、关卡、截图等）
- `user-preferences` — 用户偏好：中文交流、中文注释/提交信息

## 注意

- 蓝图父类继承的修改在编辑器中进行（用户操作），C++ 只提供父类骨架
- 本文件应保持精简，重大变更后记得同步更新


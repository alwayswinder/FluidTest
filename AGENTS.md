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
  `ProximityActivation-MasterVars-Quantizer-OutMat（复合节点）→ MyProximityActivationMasterVarsQuantizerOutMat*`、
  `EMyQuantizerMode` 枚举 + `MyQuantizerValues`、
  `CreateOrAcquireRenderTargets（复合节点）→ MyCreateOrAcquireRenderTargets`、
  `LoadVelocityInputTexture（复合节点）→ MyLoadVelocityInputTexture`、
  `LoadDensityInputTexture（复合节点）→ MyLoadDensityInputTexture`、
  `LoadTextures（复合节点）→ MyLoadTextures`、
  `CreateDynamicMaterialInstances（复合节点）→ MyCreateDynamicMaterialInstances`、
  `ManageContinuousInteractions（复合节点）→ MyManageContinuousInteractions`、
  `INIT PAINTER V2（复合节点）→ MyInitPainterV2`、
  `CreateOuputMaterialAndSetItOnTargets_Step01 → MyCreateOutputMaterialAndSetItOnTargetsStep01`、
  `CreateOuputMaterialAndSetItOnTargets_Step02 → MyCreateOutputMaterialAndSetItOnTargetsStep02`、
  `CreateOuputMaterialAndSetItOnTargets_Step03 → MyCreateOutputMaterialAndSetItOnTargetsStep03`、
  `PresetLoader → MyPresetLoader`、
  `AfterCreateRT → MyAfterCreateRT`、
  `LOD-DistaceStepsPrecalc（复合节点）→ MyLODDistaceStepsPrecalc`、
  `AfterBind 初始化流程 → MyAfterBind`、
  `BeginPlay / CheckReady / RePlay 初始化流程 → BeginPlay / MyCheckReady / MyRePlay`、
  `AfterTickDelay（事件图）→ MyAfterTickDelay`、
  `MuteBrush（复合节点）→ MyMuteBrush`、
  `CameraFacing（复合节点）→ MyCameraFacing`

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


# FluidNinjaLive 蓝图契约基线（重构安全网）

> 目的：C++ 侧的属性一旦被标成 `Transient`、改名或嵌套进 `USTRUCT`，蓝图 CDO 里保存的设计师设置值就会被
> **静默丢弃**。本文件记录当前蓝图的"契约面"（哪些属性被蓝图覆盖、哪些名字仍被蓝图图引用）以及每次改动后的核对流程。
>
> 数据来源：SpecialAgent MCP（`http://localhost:8767/mcp`）的 `python-execute`，扫描脚本 `Saved/asset_baseline.py`
> （`Saved/` 不进版本库，本文件里的清单才是版本化的基线）。

## 1. 当前基线（2026-09-14 复核）

| 蓝图 | Actor 覆盖项 | 组件模板覆盖项 | 组件模板位置 |
| --- | ---: | ---: | --- |
| `/Game/_MyTest/Fluid/Bp/NinjaLive` | 15 | 69 | 自身 |
| `/Game/_MyTest/Fluid/Bp/NinjaLive_Area_Water_Blueprint` | 17 | 69（继承自 NinjaLive） | 继承 |
| `/Game/_MyTest/Fluid/Bp/NinjaLiveComponent`（组件蓝图 CDO） | — | 68 | — |

- 组件蓝图 `NinjaLiveComponent` 的 EventGraph **为空**（逻辑已全部迁到 C++），没有蓝图侧引用。
- 全工程只有这 3 个蓝图继承自 `AMyNinjaLiveActor` / `UMyNinjaLiveComponent`，UseCases 资产不涉及。

## 2. 已修复：被 `Transient` 丢弃的蓝图默认值（4 个）

| 属性 | 蓝图里的值 | 处理 |
| --- | --- | --- |
| `MyBrushStrengthTemp2` | `1.0` | C++ 默认值改为 `1.0`（原 `0.0` 会让 `Min(Temp2, Temp1)` 恒为 0，画笔强度写进材质后为 0） |
| `MyPosition3_2D` | 10 个透明项 | 构造函数按 `MyTouchSlotCount=10` 重建（原为空数组，传统画笔路径会因索引无效提前返回） |
| `MyLastPosition3_2D` | 10 个透明项 | 同上 |
| `MyListOfAvailableTempArrays` | 40 个 `true` | 构造函数 `Init(true, 40)`（原为空数组，槽位申请会全部失败） |
| `MyFluidSolver1Iterations` | `5` | 去掉 `Transient` 恢复序列化，并把 C++ 默认值对齐为 `5`（LOD 精度参数，运行期由 LOD 检查更新） |

核对结论：修复后蓝图覆盖项从 177 条降到 169 条，减少的正好是这 4 个属性（每个出现在模板与组件蓝图 CDO 两处），
**没有新增覆盖项、没有任何值发生变化**。

## 3. 红线：蓝图图里仍在引用的名字（不得改名/嵌套/收紧访问）

来自 `NinjaLive` 与 `NinjaLive_Area_Water_Blueprint` 的 EventGraph（共 70 个节点）逐引脚核对：

| 名字 | 引用方式 | 所在蓝图 |
| --- | --- | --- |
| `MyMousePressed` | Set ×4 | NinjaLive |
| `MyTimeSinceLastClick` | Set ×4 | NinjaLive |
| `MyTraceChannel` | Get → `MyTraceMouse` 入参 | NinjaLive |
| `MyNinjaLiveTraceExclude` | Get → `MyTraceMouse` 入参 | NinjaLive |
| `MyUserInputBasedInteraction` | Get ×3 | NinjaLive |
| `MyMultipleTouchLookup` | Get ×2 + `Set Boolean (by ref)` | NinjaLive（Actor 侧） |
| `MyForceTraceMeshVerticalPosition` | Get → `Set Actor Location` 的 Z | Area_Water |
| `NinjaLiveComponent` / `MyTraceMesh` | 组件变量 Get | 两个蓝图 |
| `MyTraceMouse` | C++ 函数调用（`UMyNinjaLiveFunctions`）×2 | NinjaLive |

> 待办：这两个蓝图的**函数图（Function Graphs）尚未逐个扫描**；在对属性做改名/嵌套之前，先用
> `manage_blueprint get_graph_details` 把函数图补扫一遍。

## 4. 覆盖属性清单（改名/嵌套前必须逐个确认）

Actor 侧 15 个：

```
MyActivationVolume, MyActivationVolumeSize, MyAutoExcludeLargeOverlappingObjects,
MyForceTrackObjectsWithNocollisionFlag, MyInactiveGrayMaterial, MyInteractionVolume,
MyInteractionVolumeSize, MyOverlapBasedInteraction, MyOverlapFilterInclusiveBoneNameExact,
MyOverlapFilterInclusiveCollisionType, MyOverlapFilterInclusiveObjType, MyRoot,
MyTraceMesh, MyTraceMeshInactiveBehaviour, MyTraceMeshSize
```

组件侧 68 个：

```
MyAdjustPainterV2BrushStrength, MyAdjustPainterV2BrushVeloNoise, MyAdjustPainterV2EdgeMask,
MyAttenuationPower, MyBrushDensityNoiseFreq, MyBrushDensityNoiseScale, MyBrushVelocityClamp,
MyBrushVelocityNoiseFreq, MyBrushVelocityNoiseScale, MyBrushVelocityPow, MyCollisionChannel,
MyCollisionMask, MyContinuousInteractionInclusiveObjType, MyCoreNiagaraSystems, MyCoreSimMaterials,
MyCustomTraceSourcePosition, MyDampenBrushFactor, MyDefaultPreset, MyDensityInput, MyDivergence,
MyExpDivergenceFeedbackComponent, MyExperimentalPressureFeedback, MyExpPressureFeedbackComponent,
MyFlowFeedback, MyForceMaxSamplingFPSToNiagara, MyGlobalBrushScale, MyInactiveGrayMaterial,
MyInputMediaLoopLength, MyInternalRenderTargetsToExport, MyLightDirectionSourceIsRotation_NOT_Pos,
MyLODCheckFrequency, MyLODFarBound, MyLODNearBound, MyLODSteps, MyLWCAvoidNiagaraWarnings,
MyLWCSupport, MyMediaTexture, MyMinSamplingFPS, MyMovementIsLockedOnThisAxis,
MyMovementNotQuantizedToStepsOnAxis, MyNiagaraVariableSetSafetyDelay, MyNullMaterial,
MyOutputMaterials, MyOutputMaterialSelected, MyOverlapBasedInteraction,
MyPainterV2BrushVeloNoiseTexture, MyPauseSimWhenNotVisible, MyPointLightMovementMultiplier,
MyPresetSearchPaths, MyPressureSolver1MaxIterations, MyPressureSolver2KernelReduction,
MyPressureSolver2MaxIterations, MyPrimitiveObjBrushScale, MyPV2_GenerateVelocity,
MyPV2LineDrawingFailCooldownTime, MyPV2StopLineDrawingAboveThisVelocity, MySimAreaClamp,
MySimSpeedAdjustmentLatency, MySingleTargetModeSpeedInfluenceFactor_LEGACY,
MySingleTargetType_LEGACY, MySkeletalMeshBrushScale, MySpeed, MyStopUsingPainterCanvasWhenIdle,
MySupressUE51TextureSmearing, MyTraceChannel, MyTwoSideBlendPow, MyUserInputBrushScale, MyVelocityInput
```

## 5. 每次改动后的核对流程

1. 改动前把基线另存：`Saved/BP_AssetBaseline_before.json`（`asset_baseline.py` 的输出）。
2. 改完 C++ 后编译：编辑器内 `LiveCoding.Compile`（或关编辑器 `Build.bat`）。
3. 重跑 `asset_baseline.py`，与 before 比对三条规则：
   - **不得出现新增覆盖项**；
   - **不得出现值变化**；
   - 允许"不再作为覆盖项"，当且仅当该属性的 C++ 默认值已被改成与蓝图一致（本次的 4 个即是）。
4. 用 MCP `manage_blueprint`（`action=compile`）复核 3 个蓝图仍能编译（`compilerStatus="UpToDate"`）。

## 6. 后续重构的约束

- 第 3、4 节里的名字：改名/嵌套前必须提供值迁移，或先把蓝图侧引用清理掉。
- 纯运行态、既不在第 3 节也不在第 4 节引用的属性，才是可以安全合并/私有的候选。
- 每次属性默认值改动都要走第 5 节流程；`Transient` 是本次四起丢值的共同原因，收紧 specifier 时必须同时检查
  "蓝图是否覆盖了它"。

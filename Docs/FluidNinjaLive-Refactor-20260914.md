# FluidNinjaLive C++ 整理重构记录

## 目标

本轮重构只调整 C++ 结构、反射暴露和运行时资源管理，不改材质参数名、RenderTarget 阶段名、求解顺序、UClass 路径和已有蓝图函数入口。

## 已完成

- 将 `MyNinjaLiveComponent.cpp` 按职责拆为核心调度、交互追踪、渲染资源三个实现文件；组件类型和对象布局保持不变。
- 将 40 个公开可编辑 `MyTempArray0~39` 合并为组件内部固定容器，保留 `MyGetTempArray`、Add、Append、Clear 等兼容入口。
- 统一临时数组槽位语义为 `true=可用、false=占用`，所有状态切换改由 Acquire、Release、Reset API 完成。
- 修复持续交互路径反向解释槽位状态的问题，并保证一个 SkeletalMesh 只占一个槽位。
- 增加组件 `EndPlay`，统一取消 9 类定时任务、销毁 Painter v2 Niagara 实例并释放 RT/MID 缓存。
- 把 RT、MID、位置历史、LOD 派生值、交互缓存等纯运行态改为 `Transient`，并从详情面板编辑项中移除。
- 保留 `MyMousePressed`、`MyTimeSinceLastClick`、`MyInteractionVolumeTemplate` 等仍由现有蓝图写入的瞬态桥接字段。
- 修复 SpecialAgent 在 Commandlet 中创建 Slate 状态栏导致 CI 崩溃的问题；普通编辑器行为不变。
- 添加 `FluidTest.NinjaLive.TempArraySlots` 自动化测试，覆盖申请、容量耗尽、释放、清空和复用。

## 整理效果

| 指标 | 重构前 | 重构后 |
| --- | ---: | ---: |
| 组件头文件反射属性 | 343 | 302 |
| 组件可编辑属性 | 322 | 198 |
| 组件蓝图可写属性 | 328 | 201 |
| 最大组件实现文件 | 3750 行 | 1483 行 |

剩余 198 个可编辑项主要是模拟、材质、预设、输入、LOD 和输出配置。后续不应仅为减少数量而直接打包成新 `USTRUCT`，否则现有蓝图 CDO 中的序列化默认值需要显式迁移。

## 后续代码规范

- 设计配置：允许 `EditAnywhere` 或 `EditDefaultsOnly`；运行中确实需要蓝图修改时才使用 `BlueprintReadWrite`。
- 观测状态：使用 `VisibleInstanceOnly, BlueprintReadOnly, Transient`。
- 蓝图桥接状态：使用 `BlueprintReadWrite, Transient`，不允许在详情面板编辑。
- C++ 内部状态：普通 private 成员；只在包含 UObject 强引用时使用私有 `UPROPERTY(Transient)`。
- 初始化流程只负责组装配置；Tick 不创建 UObject、不查询固定资源、不改变配置语义。
- 新的异步 Timer、动态组件或外部委托必须同时实现 `EndPlay` 对称清理。
- RT 名称、材质参数名和 Niagara User 参数属于跨资产契约，改名必须有资产级验证。

## 审查后追加的清理

在复核本轮重构时追加的小范围整理，不改任何蓝图可见的名字、参数与执行顺序：

- **补齐 4 个被 `Transient` 丢弃的蓝图默认值**（`MyBrushStrengthTemp2`、`MyPosition3_2D`、
  `MyLastPosition3_2D`、`MyListOfAvailableTempArrays`），并把 LOD 迭代次数 `MyFluidSolver1Iterations`
  恢复为可序列化；核对结论见 `Docs/FluidNinjaLive-BP-Contract.md`（蓝图覆盖项 177 → 169，减少的正好是这 4 个）。
- 建立"蓝图契约基线"：`Docs/FluidNinjaLive-BP-Contract.md` + `Saved/asset_baseline.py`，用于后续重构前后自动比对。
- 去重：`MyOverlapArtifactWorkaround2` 与 `MyTraceObjects1` 里的越界静音逻辑合并为
  `MyApplyTraceArtifactBrushMute`（容差作为参数保留各自语义）；`MySetBrushDensityParams1/3` 的
  `EraserSwitch` 写入合并为 `MySetCompositeEraserSwitch`。
- `MyGetTempArray` 改为返回值副本的 `BlueprintPure`（蓝图侧本来就是值拷贝），并新增仅 C++ 使用的
  `MyGetTempArrayRef`；临时数组越界时返回组件自身的空占位数组，去掉原先的函数级 `static` 数组。
- 三个输出材质 Tag 应用、追踪排除列表、平台兼容开关、预设加载分别从
  `MyCreateOutputMaterialAndSetItOnTargetsStep02` 与 `MyAfterCreateRT` 中拆成独立小函数，主流程只剩调用序列。
- `MyBrushRnd1` / `MyBrushRnd2` 直接转发 `MyBrushRnd3`，消除三份完全相同的随机抖动实现。
- 追踪排除列表的收集统一为 `MyBuildTraceExcludeActors`，替换 `MyTraceObjects1` / `MyTraceObjects2` /
  `MyTraceGestures` 中三处重复循环。
- 预设逐键日志降为 `Verbose`，避免每次初始化刷屏。

## 复核发现（尚未修改，需确认后再动）

槽位统一后暴露出三份状态（组件槽位表、`MySkeletalMeshTempArrayPairs`、Actor 的 `MyOverlappingActors`）
仍可能各自漂移，以下几条建议按顺序处理：

1. `MyEndOverlapComponent` 只在 `ECC_Pawn` 分支释放槽位；带追踪标签的骨骼网格或初始重叠 Actor 走
   `MyProcessOverlapActor` 占槽后，若结束重叠的组件不是 Pawn 通道，槽位与映射都不会释放，耗光 40 槽后
   骨骼交互被静默拒绝。建议把"按 Actor 释放骨骼槽位"提取成函数，两条分支都调用。
2. `MyRePlay` 只调用 `MyResetTempArrays`，清空数组却不释放槽位、不清映射，会留下"占用但内容为空"的死槽位。
   建议改调 `MyResetTempArraySlots`。
3. `MyProximityCheck` 离开激活体积时改用了整套 `MyResetTempArraySlots`（重构前只清占用槽位的数组并保留映射），
   映射被清后 `MyEndOverlapComponent` 找不到键，`MyOverlap1` 不再被刷新；同时 `MyOverlappingActors` 从未清空，
   再次进入无法重建登记。
4. `MyCreateOrAcquireRenderTargets` 中 `RT_Output` 的创建条件是"已有 6 个 RT"，即必须有密度输入才会创建输出缓冲，
   与 `MyMake1stOutputAvailableForNiagara` 的直觉不符，需与蓝图复合节点的数组长度判断再核对一次。
5. 组件 `EndPlay` 未复位 `MyCustomTickLoopStarted` / 各 DoOnce 门；若同一实例再次 BeginPlay，自定义 tick 循环不会重启。
6. **（已用资产核对确认，会直接影响运行）** `MyBrushStrengthTemp2` 在本次重构中被改为 `Transient`，但
   `NinjaLiveComponent` 蓝图 CDO 与 `NinjaLive` 蓝图里的组件模板都把该默认值显式设为 `1.0`。`Transient`
   会丢弃这个默认值，运行时回到 C++ 的 `0.0`；而 C++ 只读不写它
   （`MySetBrushDensityParams1/3` 中的 `Min(MyBrushStrengthTemp2, MyBrushStrengthTemp1)`），
   结果是写入画笔材质的 `BrushStrength` 恒为 0。修复二选一：把 C++ 默认值改为 `1.0`，或去掉 `Transient` 恢复可序列化。
   同批被 `Transient` 丢弃的蓝图默认值还有 `MyFluidSolver1Iterations`（5 → 0，仅 LOD1 开启时参与）
   和 `MyListOfAvailableTempArrays`（40×true → 空，依赖 Actor BeginPlay 重建）。

## 资产属性核对方法（可复现）

用 SpecialAgent MCP（`http://localhost:8767/mcp`）的 `python-execute` 读取蓝图默认值：

1. `unreal.load_asset(<蓝图路径>)` → `generated_class()` → `get_default_object()` 得到 Actor 级 CDO；
2. 组件模板不在本蓝图里，而在父蓝图（本 BP 的组件继承自 `NinjaLive`）：
   `SubobjectDataSubsystem.k2_gather_subobject_data_for_blueprint()` +
   `SubobjectDataBlueprintFunctionLibrary.get_object()/get_variable_name()` 取出 `NinjaLiveComponent` 模板；
3. 用 `dir(obj)` 过滤出非 callable 成员，逐个 `get_editor_property()`，并与
   `unreal.get_default_object(<C++ 原生类>)` 对比得到"蓝图覆盖项"；
4. 对比时必须先去掉 `str()` 里的 `(0x....)` 地址，否则所有结构体/对象都会被误判为"不同"。

`NinjaLive_Area_Water_Blueprint` 的实测结果（2026-09 复核）：

| 范围 | Python 可见属性 | 与 C++ 原生默认值不同 |
| --- | ---: | ---: |
| Actor（CDO） | 69 | 17 |
| `NinjaLiveComponent` 模板 | 297 | 74 |

组件 302 个 `UPROPERTY` 中，仅 2 个委托与 5 个纯 `Transient` 内部标记对 Python 不可见。完整导出见
`Saved/BP_PropertyDump.json`。这 74 个覆盖项就是后续做 `USTRUCT` 分组时必须迁移的默认值清单。

## 下一阶段建议

当现有派生蓝图完全不再直接写 `My*` 兼容字段后，再执行资产迁移：把配置收敛为 Simulation、Interaction、Rendering、LOD 四个 `USTRUCT`，通过一次 Editor Utility 或 C++ PostLoad 迁移旧属性值，然后删除兼容字段。运行逻辑可进一步提取为非 UObject 的求解调度器和交互采样器，但应保持 `UMyNinjaLiveComponent` 为唯一对外门面。

## 验证

- `FluidTestEditor Win64 Development`：UHT 与 C++ 构建通过。
- `CompileAllBlueprints -ProjectOnly`：0 errors、0 warnings、0 blueprints failed to load。
- `/Game/_MyTest/M_Start` 无渲染游戏模式加载：成功，无蓝图编译错误。
- `/Game/_MyTest/M_Start` 实际 RHI 短时运行：预设与 38 个参数加载成功，无 FluidSim、材质、渲染或 Niagara 错误。
- `/Game/_MyTest/M_Test` 无渲染游戏模式加载：成功，无蓝图编译错误。
- `FluidTest.NinjaLive.TempArraySlots`：Success。

现有 Niagara 资源仍报告缺少 `ENiagaraGrid2DResolution_MOVEDFORMIGRATION_62` 的加载警告；这是资产依赖问题，不由本轮 C++ 重构引入。

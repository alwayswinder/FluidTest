# FluidNinjaLive C++ 核心代码实现索引

> 本索引服务于增量迁移和问题定位。源文件均位于 `Source/FluidTest`；函数名保留原蓝图名称的 `My` 前缀，便于蓝图与 C++ 双向检索。

## 文件清单

| 文件 | 内容 |
| --- | --- |
| `MyNinjaLiveComponent.h/.cpp` | 核心模拟组件的接口、生命周期、调度与空间状态 |
| `MyNinjaLiveComponentInteraction.cpp` | 输入、物体追踪、画笔与临时数组槽位管理 |
| `MyNinjaLiveComponentRendering.cpp` | RenderTarget、动态材质、Niagara、输出与输入资产 |
| `MyNinjaLiveComponentTests.cpp` | 组件内部状态的开发自动化测试 |
| `MyNinjaLiveActor.h/.cpp` | 蓝图 Actor 父类、激活体积和重叠追踪 |
| `MyNinjaLiveFunctions.h/.cpp` | RenderTarget、资源/预设、相机与射线函数库 |
| `MyNinjaLiveMemoryPoolManager.h/.cpp` | 内存池类型和 Actor 骨架 |
| `MyNinjaFluidEnums.h` | 蓝图枚举数值兼容层 |
| `FluidTest.Build.cs` | 模块依赖（含 Niagara、MediaAssets、AssetRegistry） |

## 组件：初始化与调度

| 函数 | 实现摘要 |
| --- | --- |
| `BeginPlay` / `MyCheckReady` | 用 Timer 等待 TraceMesh；就绪后设置重播委托、主变量并进入绑定流程。 |
| `MyRePlay` | 清空内部 40 槽临时数组，重新读取 Owner 激活配置，重建运行时资源。 |
| `TickComponent` / `MyCustomTick` | 在 Unreal 原生 Tick 与固定周期 Timer Tick 间切换。 |
| `MyAfterTickDelay` | 过滤禁用、接近激活和不可见暂停；维护点击/碰撞计时并一次性停用 Painter v2。 |
| `MyAfterReadyCheck` | LOD、笔刷、朝向、反馈材质参数、Painter 数组和交互分派的每帧入口。 |
| `MyAfterBind` | 初始化顺序的统一入口：光照、通道、输入、LOD、TraceMesh、RT 与后续资源。 |

## 组件：求解、RT 与动态材质

| 函数 | 实现摘要 |
| --- | --- |
| `MyCreateOrAcquireRenderTargets` | 按简单/完整模式、精度、压力半分辨率和输出选项创建 RT Map。 |
| `MyCreateDynamicMaterialInstances` | 依固定 `MyCoreSimMaterials` 下标创建 MID，并绑定 RT、输入纹理和求解参数。 |
| `MyCoreFluidsimOPs` | 调度 Composite、Advection、Divergence、Pressure ping-pong；返回非简单模式与 Painter v2 两条执行标志。 |
| `MyFluidCoreStep` | 串联位置偏移、核心求解、附加材质参数、Raymarch、外部导出和 Niagara 标量同步。 |
| `MySetAdditionalFluidsimParams` | 把预设速度、噪声、边缘、密度和散度值写入 Composite/Gradient/Divergence MID。 |
| `MyCreateOutputMaterialAndSetItOnTargetsStep01/02/03` | 建立主/次/三级输出，应用目标组件，并把 RT 注入外部 Niagara。 |
| `MyDrawInternalRenderTargetToExternal` | 首帧校验外部 RT 数组后，按 `EMyRenderTargetList` 绘制内部阶段。 |
| `MyAfterCreateRT` | MID、持续交互、输出、预设、输入纹理与 Painter v2 的完整后处理。 |

## 组件：量化、LOD 与空间参数

| 函数 | 实现摘要 |
| --- | --- |
| `MyQuantizerValues` | `EMyQuantizerMode` 到内部 `-3/-2/-1/0/正米数` 的映射。 |
| `MyDynamicSimspeedAndWorldOffsetAdjustment` | 首次采样 TraceMesh；计算量化位置、TexelSizeMult 和交互体积同步。 |
| `MyDynamicSimspeedAndWorldOffsetAdjustmentFinal` | 把局部位移、累计偏移和 BrushPuncture 写入相关 MID。 |
| `MyLockMovementOnGivenAxis` / `MyKillFracOnGivenAxis` | 锁定指定轴或基于相机方向移除量化小数部分。 |
| `MyVelocityHandlerForSimArea` | 父级位移转 TraceMesh 局部速度，Y 分量反向。 |
| `MyLODDistaceStepsPrecalc` / `MyLOD` / `MyCheckLODLevel` | 建阈值，按相机距离调整压力迭代、采样 FPS 与 Actor Tick。 |
| `MyFPSPrecisionResolution` | 分辨率下限、采样周期、精度索引及 Painter v2 插值联动。 |

## 组件：输入、画笔、物体追踪

| 范围 | 关键函数 | 实现摘要 |
| --- | --- | --- |
| 输入开关 | `MyCheckTouchOptions`、`MyEnableOwnerInput` | 从 `EMyUserInput` 推导单点/触摸，必要时启用 Owner 输入和鼠标光标。 |
| 鼠标/触摸 | `MyTraceGestures`、`MyMousePassTrue`、`MyMousePassFalse` | 查询 UV；Painter v2 追加数组，传统模式绘制线条；无鼠标时转重叠路径。 |
| 单目标 | `MySingleTargetMode`、`MyCheckValidity2`、`MyCalcPos1/2` | 支持骨骼或 Primitive 的旧模式，始终在末尾推进一次求解。 |
| 多对象 | `MyForLoopOverlapping`、`MyMultiObjectProcessorCycle3`、`MyCalcPos3/5` | 普通组件后处理骨骼/骨骼数组，循环完成后推进求解。 |
| 追踪 | `MyDefineLineTracingSource`、`MyTraceObjects1/2`、`MyTraceObj2` | 使用相机或自定义源向对象追踪，成功后生成点画笔数据。 |
| 笔刷 | `MySetBrushDensityParams1/3`、`MyPaintLine`、`MyFinalDealRTAndBrush` | 写线/点 Painter MID，传统模式直接 Draw 到 `RT_Painter`。 |
| 防伪影 | `MyBrushSwitch1/2`、`MyOverlapArtifactWorkaround2`、`MyTemporarilySwitchOffLineDrawingIFTracerFails` | 边缘/静止断线、越界静音、Painter v2 连线失败冷却。 |

## 组件：Painter v2、预设和输入资产

| 函数 | 实现摘要 |
| --- | --- |
| `MyInitPainterV2` | 选择 Niagara System，动态创建/注册组件，绑定 Painter RT，并安排安全延迟与最终配置。 |
| `MySetPosVelocityScaleArraysToPainterV2` | 上传位置、历史位置、速度与笔刷尺寸数组；条件满足时开启位置插值。 |
| `MyClearPosVelocityScaleArraysPainterV2` | 保存本帧历史后清空当前数组和追踪对象集合。 |
| `MyForwardScalarParamsToNiagara` | 枚举点画笔 MID 标量参数，跳过 `BrushSize` 后同步到 Niagara。 |
| `MyPresetLoader`（函数库）+ `MyParsePresetMapAndSetVariables` | 加载 DataTable 行字符串并赋值给模拟/笔刷参数。 |
| `MyLoadVelocityInputTexture` / `MyLoadDensityInputTexture` | 覆盖纹理优先，失败时清理材质参数；密度失败清空 Painter RT。 |
| `MyAlternativeInputsFedToCompositeDensityInput` | 接入 SceneCapture 或媒体输入；媒体可用 Timer 循环。 |

## Actor：激活与重叠

| 函数 | 实现摘要 |
| --- | --- |
| `BeginPlay` | 初始化 TraceMesh/组件同步；配置交互模板、清空容器，启动初始扫描与委托。 |
| `Tick` / `MyProximityCheck` | 按频率检查激活者是否在 ActivationVolume，切换组件激活状态和不活动视觉。 |
| `MySetInitialVisibility2` / `MyApplyInitialInactiveState` | 灰材质、隐藏和保留最后帧的初始表现。 |
| `MyInitialOverlapCheck` | 等待追踪通道就绪后获取已有重叠，并以对象类型/碰撞映射过滤。 |
| `MyBeginOverlapDetection` / `MyEndOverlapDetection` | 绑定两个组件重叠委托，并处理初始 Actor。 |
| `MyBeginOverlapComponent` | 标签、类型、Pawn、同类 Actor 和尺寸过滤；分发组件或骨骼 Actor。 |
| `MyProcessOverlapActor` | 选择 SkeletalMesh、筛选骨骼，向组件的 40 槽临时数组分配映射。 |
| `MyEndOverlapComponent` | 释放骨骼数组槽位或普通组件，并刷新 `MyOverlap1`。 |
| `MyCollisionTypeFilter1/2`、`MyExcludeLargeObjects`、`MySimContainerCapacityFilter1` | 重叠的对象类型、尺寸与容量前置判断。 |

## 函数库与兼容类型

| 项目 | 实现摘要 |
| --- | --- |
| `MyCreateRenderTarget` | 创建 2D RT 后设置地址模式、过滤和 LOD Group。 |
| `MyTemplateLoader` | 从 DataTable 的 `SourceString*` 字段经 AssetRegistry 找第一个资产。 |
| `MyPresetLoader` | 搜索预设 DataTable，解析行值为两位小数数值 Map。 |
| `MyCameraFacing` | 在 Legacy、LookAt 和 LockY 三种路径设置 SceneComponent 旋转。 |
| `MyTraceMouse` | 由屏幕鼠标/触摸命中重做相机射线，输出碰撞 UV。 |
| `MyTraceOverlap` | 从 Start 向 End 过冲射线，输出 UV、命中位置和有效标志；仍是后续可继续完善的复刻点。 |
| `FMyRenderTargetListItem` | RT 与空闲标志构成的内存池条目。 |
| `AMyNinjaLiveMemoryPoolManager` | 保存池配置和三类 RT 列表，当前不 Tick。 |

## 当前实现状态

核心 Actor/Component 的迁移已覆盖初始化、Tick、RT/MID、求解调度、输入、Painter v2、输出、LOD 和重叠交互。仍需注意两个有意保留的边界：内存池尚未参与 RT 获取；`MyTraceOverlap` 的实现已可用于当前调用路径，但其注释明确标为待与原函数库继续核对/补全。后续迁移应保持每次一小块、编译并在编辑器中验证的节奏。

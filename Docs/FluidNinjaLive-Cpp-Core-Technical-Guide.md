# FluidNinjaLive C++ 核心技术说明

> 依据 `Source/FluidTest` 当前实现整理，覆盖 `AMyNinjaLiveActor`、`UMyNinjaLiveComponent`、函数库和内存池骨架。本文描述的是已落地的 C++ 行为，而不是对原蓝图节点的逐字转录。

## 1. 目标与边界

本模块以动态材质绘制 RenderTarget 的方式实现二维 GPU 流体模拟。求解器的数值计算仍在材质中完成；C++ 的职责是：建立资源、写入材质参数、按正确的顺序驱动各个 RT 阶段，并把鼠标/触摸/物体运动转换为 Painter 输入。

它不是一个脱离资源独立运行的通用流体库。`MyCoreSimMaterials`、输出材质、预设 DataTable、Niagara System、碰撞通道和蓝图派生类共同构成运行配置。C++ 父类提供稳定骨架，蓝图仍可保留尚未迁移的内容。

```text
AMyNinjaLiveActor
  ├─ MyActivationVolume     接近激活
  ├─ MyInteractionVolume / MyTraceMesh    重叠与 UV 追踪面
  └─ UMyNinjaLiveComponent
       ├─ RenderTarget Map + Dynamic MIDs
       ├─ 输入、重叠、Painter v1/v2
       ├─ RT 求解调度与输出绑定
       └─ Niagara / 外部 RT / 材质参数集合
```

## 2. 类型职责

| 类型 | 责任 | 关键状态/接口 |
| --- | --- | --- |
| `AMyNinjaLiveActor` | 世界对象、激活区、交互区、TraceMesh、进入/离开重叠管理 | `BeginPlay`、`Tick`、`MyBeginOverlapComponent`、`MyEndOverlapComponent` |
| `UMyNinjaLiveComponent` | 初始化、资源生命周期、输入和笔刷、流体阶段调度、LOD、Niagara；实现按 Core/Interaction/Rendering 分文件维护 | `MyAfterBind`、`MyAfterReadyCheck`、`MyFluidCoreStep` |
| `UMyNinjaLiveFunctions` | 可复用的 RT 创建、AssetRegistry 预设/模板加载、相机和射线追踪 | `MyCreateRenderTarget`、`MyPresetLoader`、`MyTraceMouse` |
| `AMyNinjaLiveMemoryPoolManager` | 原蓝图内存池的类型骨架 | 精度、分辨率、RGBA/RG/R 池条目；尚未接入组件的 RT 分配路径 |
| `MyNinjaFluidEnums.h` | 蓝图枚举的数值映射 | 输入、量化、锁轴、精度、输出 RT、非激活显示行为 |

所有蓝图可编辑成员遵循 `My` 前缀。函数参数不加该前缀；这使 C++ 父类与蓝图中遗留的同名变量能够并存。

## 3. 生命周期与初始化

### 3.1 Actor 与组件的启动关系

普通模式下 Actor 的 `BeginPlay` 先把 `MyTraceMesh`、输入类型和重叠开关同步到组件；重叠模式还会重置容器、排除其它同类 Actor，并启动初始重叠扫描及两个重叠委托。组件的 `BeginPlay` 同时以 0.2 秒循环检查 TraceMesh 是否可用。

组件发现 `MyTraceMeshComponent` 后停止检查，绑定 `MyComponentRePlayEvent`，然后执行：

```text
MyCheckReady
  → MyProximityActivationMasterVarsQuantizerOutMat
  → MyAfterBind
     → 光照提供者、追踪通道、输入源、LOD、TraceMesh 属性
     → 精度/分辨率/Painter v2 参数
     → 创建 RT
     → MyAfterCreateRT
        → 创建 MID、持续交互、输出材质/目标、预设、输入纹理、Niagara
```

`MyRePlay` 清空临时骨骼数组，然后从 Owner 再同步一次激活设置并复跑 `MyAfterBind`。因而重播会重建 RT、动态材质和 Painter v2 实例；调用方应避免在高频路径触发它。

### 3.2 接近激活

当 `MySimActivatedByPawnProximity` 开启时，Actor 延迟真正初始化，激活体积查询指定的 `MyActivator`（未指定则 0 号玩家 Pawn）。状态改变时写入组件的 `MyPawnInsideActivationBounds`。

离开时按 `EMyInactiveBehaviour` 执行：保留最后帧（把 Composite 绘制到小型预览 RT）、改灰材质或隐藏 TraceMesh。进入时相应恢复输出材质或可见性。组件的 `MyAfterTickDelay` 在不活跃时仅一次性停用 Painter v2 Niagara，恢复活跃时重新武装该开关。

## 4. 每帧驱动和求解顺序

组件支持两种 Tick 来源：

| 模式 | 条件 | 实现 |
| --- | --- | --- |
| Unreal 原生 Tick | `MyUseUnrealNativeEventTick=true` | 首帧可按 `MyLimitUnrealNativeEventTick` 设置组件 Tick 间隔；使用实际 `DeltaTime`。 |
| 自定义循环 | `false` | 首个 Tick 创建周期 Timer，间隔为 `MyTickRateCustom`；回调使用该固定 Delta。 |

两者都会先经过 `MyAfterTickDelay`：禁用、接近激活、不可见暂停和计时条件被统一过滤。允许更新时进入 `MyAfterReadyCheck`：LOD、笔刷静音、相机朝向、Painter 参数、Painter v2 数组清理以及鼠标/重叠输入分派。

### 4.1 主计算路径

`MyFluidCoreStep` 是本帧求解入口。简单 Painter 且未双缓冲时会直接返回；否则其路径如下：

```text
收集 Painter v2 数组
  → MyDynamicSimspeedAndWorldOffsetAdjustment
  → MyCoreFluidsimOPs
      输入材质 → RT_DensityInputMaterial（可选）
      广播 TraceMeshPos
      Painter 双缓冲/Composite
      RT_Output（可选预输出）
      Advection → Divergence → 压力迭代
  → 附加求解参数、Raymarch 光照、外部 RT 导出（非简单模式）
  → 同步 Painter 标量到 Niagara（Painter v2）
```

压力阶段采用 ping-pong：`RT_PressureDivergenceTemp` 由 `MyMIPressureCycle1` 写入，`RT_PressureDivergence` 由 `MyMIPressureCycle2` 写回。迭代数来自 Pressure Solver 配置，LOD 可将 Solver 1 迭代减少到当前等级。各阶段通过 `UKismetRenderingLibrary::DrawMaterialToRenderTarget` 调用 GPU 材质，因此材质参数名是 C++ 与流体算法之间的关键契约。

### 4.2 RT 拓扑

非简单模式会创建下列核心 RT：

| 名称 | 格式/尺寸 | 用途 |
| --- | --- | --- |
| `RT_Composite` | 全分辨率 RGBA 16f/32f | 速度/密度合成缓冲 |
| `RT_Advection` | 全分辨率 RGBA | 平流后的缓冲 |
| `RT_Painter` | 全分辨率 RGBA | 点/线笔刷或 Niagara Painter 的输出 |
| `RT_PressureDivergence` | RG | 散度与压力 ping-pong 缓冲 |
| `RT_PressureDivergenceTemp` | RG | 压力临时缓冲 |
| `RT_DensityInputMaterial` | R8，可选 | 输入材质/SceneCapture 密度输入 |
| `RT_Output` | RGBA，可选，可 2 倍分辨率 | 供第二输出或 Niagara 使用的首输出 |

`MyHalfResPressureAndDivergenceBuffers` 只降低压力/散度尺寸。简单模式仅创建 Painter；开双缓冲时再创建 Composite。`MySimAreaClamp` 控制 RT 地址模式，格式由 `MySimPrecision` 转换后的 `MySimPrecisionIndex` 决定。

## 5. 移动、量化与 LWC

`EMyQuantizerMode` 先由 `MyQuantizerValues` 映射为内部步长：`-3` 不偏移、`-2` 手动偏移、`-1` 自动偏移并修正极端值、`0` 自动连续偏移，正数为米级量化步长。

`MyDynamicSimspeedAndWorldOffsetAdjustment` 首次采样 TraceMesh 世界/父级坐标和小数部分，随后按量化轴或相机方向移除小数分量，并可将 TraceMesh/InteractionVolume 设为绝对位置。最终函数把局部位移写到求解及笔刷 MID 的 `WorldOffsetDeltaX/Y`、累计 `WorldOffsetX/Y` 与 `BrushPuncture`；父级运动还经 `MyVelocityHandlerForSimArea` 加入速度偏移。

量化和 `MyCameraFacingTraceMesh` 不兼容：初始化会把量化回退为自动无步长模式。启用 `MyLWCSupport` 时，位置还会用双精度材质变量 `TraceMeshPosDouble` 和 Niagara Position 变量同步。

## 6. 交互与 Painter

### 6.1 鼠标/触摸

`MyTraceGestures` 通过 `MyTraceMouse` 获取鼠标或允许手指的碰撞 UV。函数库首先判断命中的组件是否正是 TraceMesh，再从相机位置进行一次 Trace 并用 `FindCollisionUV` 得到 0–1 UV。

`MyMousePassTrue` 在 Painter v2 下直接向 `PositionArray`、`VelocityArray`、`BrushSizeArray` 追加数据，避免线状 Painter 覆盖已有物体笔刷；传统路径则更新历史 UV、调用 `MyPaintLine` 并绘制线状笔刷。没有鼠标输入时 `MyMousePassFalse` 依次选择无交互、旧单目标，或多对象循环。

### 6.2 物体和骨骼

普通 Primitive 由 `MyCalcPos3` 读取组件位置，骨骼由 `MyCalcPos5` 读取 Socket 位置。它们都会计算画笔尺寸系数；尺寸可以来自包围盒/缩放，骨骼也可由父骨骼距离决定。`MyTraceObjects2` 用从相机或自定义源到目标位置的超量射线追踪，成功后写点状 Painter 数据；追踪失败时可临时关闭 Painter v2 的轨迹连线。

`MyBrushSwitch1/2` 负责在边缘、静止或新点击时断开传统线条。`MyOverlapArtifactWorkaround2` 检测“物体在动但有效 Trace 位置不动”的越界情形并静音笔刷，避免平面边缘留下伪影。

### 6.3 Painter v2 / Niagara

Painter v2 仅在非旧单目标模式、可用 Niagara System 和有效 Owner 下创建。系统索引 0/1 对应是否连接追踪点。初始化会绑定 `User.PaintbufferOutput/Input`，随后写模拟分辨率、量化、画笔阈值、速度噪声和 TraceMesh 尺寸；可选强制 Solo Tick。

每帧把位置、历史位置、速度、笔刷尺寸数组写给 Niagara Data Interface。启用轨迹连线时，仅在追踪对象集合、TraceMesh 位置、采样 FPS 和命中状态均满足条件时启用 `User.PosInterpol`。这避免对象切换或量化跳跃时错误插值。

## 7. Actor 重叠模型

Actor 的交互体积可由 Box 或 TraceMesh 担任。初始扫描等待组件完成追踪通道自动发现，然后使用 `ComponentOverlapComponents` 和对象类型/碰撞映射过滤候选。

进入回调按顺序处理：排除列表 → 指定组件标签/骨骼网格标签 → 碰撞对象类型 → Pawn 骨骼路径 → 同类 Actor 忽略 → 大物体过滤 → 普通组件列表。Pawn 或指定骨骼对象由 `MyProcessOverlapActor` 处理：优先选有标签的 SkeletalMesh，按精确/部分骨骼名收集，并为每个网格从组件的 40 个临时数组槽位分配一个索引，记录在 `MySkeletalMeshTempArrayPairs`。

离开回调会释放相应槽位、清空数组、删除映射并重算 `MyOverlap1`。容量判断要求至少有一个可用槽位；多个骨骼网格时，剩余槽位必须不少于网格数。

`MyListOfAvailableTempArrays` 统一使用 `true=可用、false=占用`。申请、释放和重置必须通过组件的 `MyAcquireTempArraySlot`、`MyReleaseTempArraySlot`、`MyResetTempArraySlots` 完成；40 个槽位的数据保存在组件内部固定容器中，不再作为 40 个可编辑属性暴露。

## 8. 材质、输出与外部消费者

`MyCreateDynamicMaterialInstances` 固定依赖 `MyCoreSimMaterials` 下标：0/1 为点/线笔刷，2 起为桌面/移动端求解材质。创建后会按阶段绑定 RT、速度/密度/碰撞遮罩输入以及噪声、边缘、压力参数。

输出建立分三步：

1. 创建主、次、三级输出 MID，绑定 Velocity/Density、Pressure、Divergence、Painter 等标准纹理参数；次/三级可使用 `RT_Output`。
2. 将输出材质放到 TraceMesh，以及以 ActorTag/ComponentTag 找到的 Primitive 或 Volumetric Cloud。
3. 将模拟 RT、TraceMesh 位置和尺寸写给指定标签 Actor 的 Niagara 组件。

`MyDrawInternalRenderTargetToExternal` 可把速度密度、散度、压力、Painter 或 Output 材质绘制到用户提供的外部 RT。它首次执行时校验两个数组数量和所有目标有效性；失败会永久关闭本次生命周期的导出 Gate，改配置后应通过重播/重新初始化打开新 Gate。

## 9. 预设、模板与替代输入

`MyPresetLoader` 用 AssetRegistry 在搜索路径内找 `DT_<Filter>_<Preset>` 数据表，读取每行以 `SourceString` 开头的字段并以最多两位小数存为 `TMap<FString,double>`。`MyParsePresetMapAndSetVariables` 将该 Map 复制到速度、反馈、散度、笔刷、密度纹理和噪声参数。

`MyTemplateLoader` 用同一字段定位 `VelocityTemplate` 或 `DensityTemplate` 的资源包。用户覆盖纹理优先于预设；加载失败会解除对应材质纹理，密度失败还会清空 Painter RT。SceneCapture 优先写入 `RT_DensityInputMaterial`，媒体输入则配置 MediaPlayer 并可按定时器循环播放。

## 10. LOD、性能和维护建议

LOD 以 Owner 到玩家相机的距离选择等级。近距离取最高等级和最大压力迭代，远距离取等级 1/单次迭代；中间距离按预计算阈值落段。可独立降低 Pressure Solver 迭代和采样 FPS，后者也会设置 Owner Tick 间隔。

- 先确认所有材质的参数名与 C++ 完全一致；参数名错误通常不会报编译错误，却会造成静默的视觉异常。
- `MyCoreSimMaterials` 的数组下标是实现契约，调整资源顺序前必须同步 `MyCreateDynamicMaterialInstances`。
- 修改 RT 配置、输出目标或 Painter v2 时优先测试 `MyRePlay` 后的结果，因为该流程才覆盖完整重建路径。
- `MyTraceOverlap` 当前是保守实现：无有效命中且非 Painter v2 时直接返回默认输出；后续若复刻原函数库，应以原蓝图分支为准补全。
- 内存池类当前只是兼容类型骨架；组件仍直接创建 RT，尚未复用池条目。

## 11. 编译与验证

项目为 UE 5.7 Runtime 模块，依赖 `Core`、`CoreUObject`、`Engine`、`InputCore`、`EnhancedInput`、`MediaAssets`、`Niagara` 和 `AssetRegistry`。关闭编辑器后使用 `Build.bat FluidTestEditor Win64 Development` 编译；编辑器开启时由 Live Coding 处理变更。

建议最小验证链：放置继承 `AMyNinjaLiveActor` 的蓝图 → 确认组件获得 TraceMesh → 检查 RT/MID 已建立 → 鼠标或重叠物体产生 Painter 输入 → 验证 Output/Niagara/外部 RT 消费者。调试预设时可查看 `MyAfterCreateRT` 输出的 `[FluidSim][Preset]` 日志。

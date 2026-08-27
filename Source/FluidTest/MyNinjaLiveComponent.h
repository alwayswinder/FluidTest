// MyNinjaLiveComponent.h — FluidNinjaLive 的 NinjaLiveComponent 蓝图对应的 C++ 父类（逐步迁移）
// 命名规范：类/函数/变量加 My 前缀；蓝图父类修改在编辑器中进行，C++ 只提供父类骨架。

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInterface.h"
#include "MyNinjaFluidEnums.h"
#include "Engine/SceneCapture2D.h"
#include "TimerManager.h"
#include "MyNinjaLiveComponent.generated.h"

class AMyNinjaLiveActor;
class AMyNinjaLiveMemoryPoolManager;
class AActor;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMaterialParameterCollection;
class UDataTable;
class UFileMediaSource;
class UMediaPlayer;
class UMediaTexture;
class UNiagaraComponent;
class UNiagaraSystem;
class UTexture2D;
class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMyComponentRePlayEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyWorldSpaceOffsetEvent, FVector, TraceMeshPos);

/**
 * NinjaLiveComponent 蓝图组件的 C++ 父类。
 * 已迁移：临时数组（TempArray0~39）、Map/长度变量、数组操作与若干工具函数。
 * 蓝图侧同名函数/变量保留，待逐步迁移后删除。
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup = (FluidSim), meta = (BlueprintSpawnableComponent))
class FLUIDTEST_API UMyNinjaLiveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMyNinjaLiveComponent();

	/** 延迟检查 TraceMesh 是否创建完成的定时器。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Init")
	FTimerHandle MyTimerCheckReady;

	/** TraceMesh 准备完成后可由 Owner 触发的重播事件。 */
	UPROPERTY(BlueprintAssignable, Category = "FluidSim|Init")
	FMyComponentRePlayEvent MyComponentRePlayEvent;

	/** TraceMesh 世界位置变化时广播，对应 CoreFluidsimOPs 的 WorldSpaceOffset 委托。 */
	UPROPERTY(BlueprintAssignable, Category = "FluidSim|Trace")
	FMyWorldSpaceOffsetEvent MyWorldSpaceOffset;

	/** TraceMesh 就绪后完成首次初始化与重播事件绑定。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Init")
	void MyCheckReady();

	/** 重置临时数据并按 Owner 设置重新初始化模拟。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Init")
	void MyRePlay();

	/** 执行延迟 Tick；仅在碰撞计时器实际更新时返回 true。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Simulation")
	bool MyAfterTickDelay(double DeltaSeconds);

	/** 根据鼠标与重叠状态更新当前画笔强度。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials|Brush")
	void MyMuteBrush();

	/** 根据模拟区域尺寸限制世界位移引起的纹理偏移。 */
	UFUNCTION(BlueprintPure, Category = "FluidSim|Simulation")
	FVector MyCorrectExtremes(FVector DeltaPos, FVector Scale, FVector Composite) const;

	/** 根据 TraceMesh 位移更新模拟纹理偏移与相关动态材质参数。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Simulation")
	void MyDynamicSimspeedAndWorldOffsetAdjustmentFinal();

	/** 采样 TraceMesh 与父级位置，处理锁轴后更新模拟区域运动。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Simulation")
	void MyDynamicSimspeedAndWorldOffsetAdjustment();

	/** 将指定轴的位置还原为 TraceMesh 初始世界坐标。 */
	UFUNCTION(BlueprintPure, Category = "FluidSim|Trace")
	FVector MyLockMovementOnGivenAxis(FVector Pos, EMyQuantizerAxisIgnore LockThisAxis) const;

	/** 按量化锁轴或相机朝向屏蔽两组位置分量。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Trace")
	void MyKillFracOnGivenAxis(FVector Frac, FVector FracInit, EMyQuantizerAxisIgnore QuantizerIgnoresThisAxis,
		FVector& FracOut, FVector& FracInitOut) const;

	// ------------------------------------------------------------------
	// 临时 Name 数组 TempArray0~39（蓝图类型 TArray<FName>）
	// ------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray4;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray6;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray7;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray8;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray9;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray11;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray12;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray13;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray14;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray15;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray16;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray17;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray18;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray19;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray20;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray21;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray22;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray23;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray24;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray25;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray26;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray27;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray28;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray29;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray30;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray31;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray32;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray33;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray34;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray35;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray36;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray37;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray38;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray39;

	/** RenderTarget 映射表（string → RT） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|RT")
	TMap<FString, TObjectPtr<UTextureRenderTarget2D>> MyRenderTargetsMap;

	/** RenderTarget 名称列表；保留给蓝图按索引访问创建结果。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|RT")
	TArray<FString> MyRenderTargetsList;

	/** 模拟区域边界采样是否使用 Clamp 地址模式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|RT")
	bool MySimAreaClamp = false;

	/** 强制输出缓冲使用 8 位格式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|RT")
	bool MyForce8bitOutputBuffer = false;

	/** 简单 Painter 模式下强制 Painter 缓冲使用 8 位格式。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|RT")
	bool MyForce8bitSimplePainterBuffers = false;

	/** 输出缓冲是否使用两倍模拟分辨率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|RT")
	bool MyForce2xResolutionOutputBuffer = false;

	/** 是否额外创建第二个输出缓冲。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|RT")
	bool MyMake1stOutputAvailableFor2ndOutput = false;

	/** 是否让第一个输出缓冲可供 Niagara 使用。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|RT")
	bool MyMake1stOutputAvailableForNiagara = false;

	/** Map 长度临时值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	int32 MyMapLengthTmp = 0;

	// ------------------------------------------------------------------
	// VelocityHandlerForSimArea 相关（模拟区域运动速度）
	// ------------------------------------------------------------------
	/** 模拟区域运动速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	double MyVeloFromSimAreaMotion = 0.0;

	/** 模拟区域运动对画笔穿透强度的影响。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	double MySimAreaMotionEffectsBrushPuncture = 0.0;

	/** TraceMesh 组件引用 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	TObjectPtr<UStaticMeshComponent> MyTraceMeshComponent = nullptr;

	/** TraceMesh 父级当前位置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	FVector MyTraceMeshParentPos = FVector::ZeroVector;

	/** TraceMesh 父级上一帧位置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	FVector MyTraceMeshParentLastPos = FVector::ZeroVector;

	/** TraceMesh 的初始世界位置，用于锁定量化移动轴。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	FVector MyTraceMeshPosInitialWorld = FVector::ZeroVector;

	/** TraceMesh 上一帧世界位置，用于计算本帧模拟区域位移。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	FVector MyTraceMeshLastPos = FVector::ZeroVector;

	/** 本帧 TraceMesh 的世界空间位移。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	FVector MyTraceMeshDeltaPos = FVector::ZeroVector;

	/** 首次采样的 TraceMesh 相对父级位置与小数部分。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	FVector MyTraceMeshPosInitialLocal = FVector::ZeroVector;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	FVector MyTraceMeshPosInitialFractionalPart = FVector::ZeroVector;

	/** 是否已经完成动态模拟区域位置的首次采样。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	bool MyDynamicSimPositionInitialized = false;

	/** 量化时保持连续移动的小数分量所忽略的轴。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Quantizer")
	EMyQuantizerAxisIgnore MyMovementNotQuantizedToStepsOnAxis = EMyQuantizerAxisIgnore::None;

	/** 首次采样时可将 TraceMesh 固定到指定的世界 Z 坐标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	bool MyForceTraceMeshToCustomVerticalPos = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace", meta = (EditCondition = "MyForceTraceMeshToCustomVerticalPos"))
	double MyForceTraceMeshVerticalPosition = 0.0;

	/** 非单目标模式下写入 TexelSizeMult 前的蓝图延迟。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	double MySimSpeedAdjustmentLatency = 0.0;

	/** 旧版单目标模式对速度值的影响系数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	double MySingleTargetModeSpeedInfluenceFactor_LEGACY = 0.0;

	// ------------------------------------------------------------------
	// Enable OWNER Input 相关（用户输入方式）
	// ------------------------------------------------------------------
	/** 用户输入方式（UserInput_Enum，默认无输入） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Input")
	EMyUserInput MyUserInputBasedInteraction = EMyUserInput::None;

	/** 是否显示鼠标光标 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Input")
	bool MyShowMouseCursor = true;

	// ------------------------------------------------------------------
	// ManageContinuousInteractions 相关（Owner 组件与骨骼筛选）
	// ------------------------------------------------------------------
	/** 是否持续使用 Owner 上满足筛选条件的组件进行交互。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	bool MyContinuousInteractionWithOwnerActor = false;

	/** 仅接受这些组件名称；为空时接受全部候选组件。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<FName> MyContinuousInteractionComponentNamesExact;

	/** 仅接受这些 SkeletalMesh 骨骼名称；为空时不分配骨骼临时数组。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<FName> MyContinuousInteractionBoneNamesExact;

	/** 可参与持续交互的对象类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<TEnumAsByte<EObjectTypeQuery>> MyContinuousInteractionInclusiveObjType;

	/** 已筛选出的 Owner Primitive 组件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FluidSim|Interaction")
	TArray<TObjectPtr<UPrimitiveComponent>> MyOverlappingComponents;

	/** Owner 上的 SkeletalMesh 组件缓存。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FluidSim|Interaction")
	TArray<TObjectPtr<USkeletalMeshComponent>> MyContinuousInteractionSkeletalComponent;

	/** 当前 SkeletalMesh 已匹配且尚未分配的骨骼名称。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FluidSim|Interaction")
	TArray<FName> MyContinuousInteractionBoneNamesExactTemp;

	/** 骨骼筛选的工作副本。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FluidSim|Interaction")
	TArray<FName> MyContinuousInteractionBoneNamesExactTemp2;

	/** 临时数组槽位是否已被 SkeletalMesh 骨骼交互占用。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<bool> MyListOfAvailableTempArrays;

	/** SkeletalMesh 到其骨骼临时数组索引的映射。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FluidSim|Interaction")
	TMap<int32, TObjectPtr<UPrimitiveComponent>> MySkeletalMeshTempArrayPairs;

	/** 清空全部临时数组 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Temp")
	void MyResetTempArrays();

	/** 按索引（0~39）返回对应临时数组引用 */
	UFUNCTION(BlueprintPure, Category = "FluidSim|Temp")
	TArray<FName>& MyGetTempArray(int32 Index);

	/** 向指定临时数组添加一个元素 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Temp")
	void MyAddToTempArray(int32 ArrayIndex, FName Item);

	/** 清空指定临时数组 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Temp")
	void MyClearTempArray(int32 ArrayIndex);

	/** 把另一数组追加到指定临时数组末尾 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Temp")
	void MyAppendToTempArray(int32 ArrayIndex, const TArray<FName>& Items);

	/** 刷新 Owner 持续交互组件、骨骼筛选结果与临时数组映射。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Interaction")
	void MyManageContinuousInteractions();

	/** 解析预设参数映射并写入模拟与画笔参数。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Preset")
	void MyParsePresetMapAndSetVariables(const TMap<FString, double>& PresetMap);

	/** 比较 Map 长度：输出 MapLength、Added=(LastIndex+1)-FirstIndex、Equal=(Added+MapLengthTmp)==MapLength */
	UFUNCTION(BlueprintPure, Category = "FluidSim|Temp")
	void MyCompareMapLength(int32 FirstIndex, int32 LastIndex, int32& MapLength, bool& Equal, int32& Added) const;

	/** 处理模拟区域速度：输出 X/Y/Z 分量（Y 取反），CoEff 默认 -0.01 */
	UFUNCTION(BlueprintPure, Category = "FluidSim|Velocity")
	void MyVelocityHandlerForSimArea(double CoEff, double& X, double& Y, double& Z) const;

	/** 检查 Owner 是否为 NinjaLive 类；返回 true 并输出 AsNinjaLive 强转引用 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Input")
	bool MyCheckComponentOwner(AMyNinjaLiveActor*& AsNinjaLive) const;

	/** 启用 Owner 的输入（输入方式非无输入且 Owner 是 NinjaLive 类时） */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Input")
	void MyEnableOwnerInput();

	/** 根据量化模式返回对应的步长值（米），用于纹理偏移量化 */
	UFUNCTION(BlueprintPure, Category = "FluidSim|Quantizer")
	int32 MyQuantizerValues(EMyQuantizerMode InQuantizerMode) const;

	// ------------------------------------------------------------------
	// ProximityActivation-MasterVars-Quantizer-OutMat 复合节点相关变量
	// ------------------------------------------------------------------
	/** 是否已由 Pawn 靠近激活（运行时从 Owner 同步，蓝图 NONPUBLICLiveActivation） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	bool MyComponentActivatedByPawnProximity = false;

	/** 组件是否被禁用（运行时从 Owner 同步，蓝图 Live Activation） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	bool MyDisableComponent = false;

	/** 是否抑制 BeginPlay 初始化 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|System")
	bool MyBeginPlaySupressed = false;

	/** 初始化是否完成 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|System")
	bool MyInitDone = false;

	/** 材质实例是否已创建完成 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|System")
	bool MyMaterialInstacesDone = false;

	/** 阻止本帧延迟 Tick 的执行。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	bool MyTickBlocker = false;

	/** Pawn 当前是否处在模拟激活范围内。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	bool MyPawnInsideActivationBounds = false;

	/** Owner 不可见时暂停模拟与 Painter v2。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	bool MyPauseSimWhenNotVisible = false;

	/** 判定 Owner 最近可见时使用的时间容差。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation", meta = (ClampMin = "0.0"))
	float MyWaitBeforePause = 0.2f;

	/** 距离上一次点击与碰撞的累计时间。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	double MyTimeSinceLastClick = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	double MyTimeSinceLastCollision = 0.0;

	/** AfterTickDelay 中与蓝图 DoOnce_2 对应的停用门状态。 */
	UPROPERTY(Transient)
	bool MyAfterTickDelayDeactivateDoOnceClosed = false;

	/** AfterTickDelay 中与蓝图 DoOnce_3 对应的重新武装门状态。 */
	UPROPERTY(Transient)
	bool MyAfterTickDelayRearmDoOnceClosed = false;

	/** 预设名过滤条件 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|System")
	FName MyPresetNameFilterCriteria = NAME_None;

	/** 首选预设数据表；有效时可强制加载它。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	TObjectPtr<UDataTable> MyDefaultPreset = nullptr;

	/** 是否优先加载首选预设。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	bool MyForceAutoLoadPreset = false;

	/** 当前预设名称与查找路径。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	FString MyActualPreset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	TArray<FName> MyPresetSearchPaths;

	/** 当前预设的数值映射。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	TMap<FString, double> MyPresetMap;

	/** 追踪时排除的 NinjaLive 接口 Actor。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	TArray<TObjectPtr<AActor>> MyNinjaLiveTraceExclude;

	/** 是否关闭 UE 5.1 TSR 的纹理闪烁抑制周期。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MySupressUE51TextureSmearing = false;

	/** UE5 早期访问版本标志（早于 UE 5.4 的版本为 false，用于修复物理速度 bug） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|System")
	bool MyUE5EAFLAG = false;

	/** 量化步长（米），由 MyQuantizerValues 计算 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Quantizer")
	int32 MyQuantizerStepSize = 0;

	/** 延迟写入 TexelSizeMult 的定时器。 */
	FTimerHandle MyTimerSimSpeedAdjustment;

	/** 对应蓝图 Delay 的未完成 latent action 标记。 */
	bool MySimSpeedAdjustmentPending = false;

	/** 是否启用画笔双缓冲（PaintBuffer WorldSpace 偏移或 Painter v2 时需要） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Quantizer")
	bool MyEnablePainterDoubleBuffering = false;

	/** TraceMesh 是否在世界空间移动（量化模式） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Quantizer")
	EMyQuantizerMode MyTraceMeshMovingInWorldSpace = EMyQuantizerMode::NoQuantizerTextureOffsetAutomatic;

	/** 输出材质数组 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TArray<TObjectPtr<UMaterialInterface>> MyOutputMaterials;

	/** 次级和三级输出材质数组；为空时不创建对应输出 MID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TArray<TObjectPtr<UMaterialInterface>> MySecondaryOutputMaterials;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TArray<TObjectPtr<UMaterialInterface>> MyTertiaryOutputMaterials;

	/** 三组输出材质各自选用的数组索引。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	int32 MyOutputMaterialSelected = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	int32 MySecondaryOutputMaterialSelected = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	int32 MyTertiaryOutputMaterialSelected = 0;

	/** 输出材质使用的参数集合；有效时同步 TraceMesh 的位置和缩放。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TObjectPtr<UMaterialParameterCollection> MySetInternalParamsToMaterialParamCollection = nullptr;

	/** 是否使用 Painter v2 追踪物体（强制双缓冲） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MyUsePAINTER_V2_ToTrackObjects = true;

	/** TraceMesh 是否面向相机（与量化不兼容） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MyCameraFacingTraceMesh = false;

	/** CameraFacing 选择原蓝图中的 LookAt 朝向路径。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MyUseLegacyCameraFacing = false;

	/** CameraFacing 的 LockY 路径，会组合初始旋转偏移。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MyCameraFacingLockYAxis = false;

	/** 按 CameraFacing 复合节点更新 TraceMesh 的相机朝向。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Compatibility")
	void MyCameraFacing();

	/** 旧版单目标模式 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MySingleTargetMode_LEGACY = false;

	/** 旧版单目标模式是否以画笔速度更新模拟速度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MySingleTargetModeSetSimSpeed_LEGACY = false;

	/** SingleTargetVelocity 写入的当前速度长度。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	double MySpeedTemp = 0.0;

	/** SingleTargetVelocity 是否从多触点数组读取当前位置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	bool MyMousePass = false;

	/** 多触点位置数组的当前读取索引。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	int32 MyTouchLookupIndex = 0;

	/** 单触点当前位置与上一帧位置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	FLinearColor MyPosition2_2D = FLinearColor::Black;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	FLinearColor MyLastPosition2_2D = FLinearColor::Black;

	/** 多触点当前位置与上一帧位置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	TArray<FLinearColor> MyPosition3_2D;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	TArray<FLinearColor> MyLastPosition3_2D;

	/** 计算旧版单目标画笔速度并写入线形 Painter 材质。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Velocity")
	void MySingleTargetVelocity();

	// ------------------------------------------------------------------
	// ProximityActivation-MasterVars-Quantizer-OutMat 复合节点
	// 蓝图实现：In1=Owner 激活设置（含 CheckComponentOwner），In2=直接初始化；
	// 内部依次：量化与 CameraFacing 冲突修正、画笔双缓冲、MasterVars 初始化、OutMat 空数组补占位材质
	// ------------------------------------------------------------------
	/** 初始化复合节点主流程（In2 路径：跳过 Owner 激活设置） */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Init")
	void MyProximityActivationMasterVarsQuantizerOutMat();

	/** 从 Owner 读取激活设置后走初始化（In1 路径：含 CheckComponentOwner） */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Init")
	void MyProximityActivationMasterVarsQuantizerOutMatFromOwner();
	// ------------------------------------------------------------------
	// LightDirectionProviderCheck 复合节点
	// 蓝图：EnableRayMarching 开启时检查 LightDirectionProvider 是否有效，
	// 无效则用 Owner 初始化并设置默认太阳参数（手动太阳位置）
	// ------------------------------------------------------------------
	/** 是否启用光线追踪（RayMarching） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Light")
	bool MyEnableRayMarching = false;

	/** 光线方向提供者（默认取 Owner） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Light")
	TObjectPtr<AActor> MyLightDirectionProvider = nullptr;

	/** 光源方向是否来自旋转而非位置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Light")
	bool MyLightDirectionSourceIsRotation_NOT_Pos = false;

	/** 太阳纬度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Light")
	double MySunLatitude = 0.0;

	/** 太阳经度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Light")
	double MySunLongitude = 0.0;

	/** 太阳高度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Light")
	double MySunHeight = 0.0;

	/** 是否强制手动太阳位置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Light")
	bool MyForceManualSunPosition = false;


	/** 检查并初始化光线方向提供者（EnableRayMarching 开启时有效） */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Light")
	void MyLightDirectionProviderCheck();

	// ------------------------------------------------------------------
	// TraceChannelAutoFind 相关（追踪通道自动查找）
	// ------------------------------------------------------------------
	/** 追踪通道（ETraceTypeQuery，默认 TraceTypeQuery1） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	TEnumAsByte<ETraceTypeQuery> MyTraceChannel = TraceTypeQuery1;

	/** 碰撞通道（ECollisionChannel，默认 ECC_WorldStatic） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	TEnumAsByte<ECollisionChannel> MyCollisionChannel = ECC_WorldStatic;

	/** 首选追踪通道名称（默认 "FluidTrace"） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	FString MyPreferredTraceChannelName = TEXT("FluidTrace");

	/** 追踪通道是否已设置完毕 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	bool MyTraceChannelsSet = false;

	/** 自动查找追踪通道：遍历 ETraceTypeQuery 和 ECollisionChannel，匹配 PreferredTraceChannelName 并设置对应通道 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Trace")
	void MyTraceChannelAutoFind();
	// ------------------------------------------------------------------
	// SceneCapCamera-VS-InputMaterials 相关（场景捕捉 vs 输入材质的切换）
	// ------------------------------------------------------------------
	/** 输入材质数组（用于替代场景捕捉） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TArray<TObjectPtr<UMaterialInterface>> MyInputMaterials;

	/** 输入场景捕捉相机（有效时优先使用，替代输入材质） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TObjectPtr<ASceneCapture2D> MyInputSceneCaptureCamera = nullptr;

	/** 是否使用输入材质（由 MySceneCapCameraVSInputMaterials 自动计算） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	bool MyUseInputMaterials = false;

	/** 判断使用场景捕捉还是输入材质：有输入材质且场景捕捉相机无效时使用输入材质 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials")
	void MySceneCapCameraVSInputMaterials();

	// ------------------------------------------------------------------
	// SetTraceMeshProperties 相关（追踪网格碰撞与派生参数）
	// ------------------------------------------------------------------
	/** 追踪网格尺寸系数，由包围盒最大分量换算 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	double MyTraceMeshSizeCoeff = 0.0;

	/** 是否按追踪网格尺寸反向缩放画笔 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	bool MyBrushScaledInverselyByTraceMeshSize = false;

	/** 追踪网格初始化旋转 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	FRotator MyTraceMeshInitialRotation = FRotator::ZeroRotator;

	/** 追踪网格半透明排序优先级 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	int32 MyTraceMeshTranslucentSortPrio = 0;

	/** 追踪网格是否同时承担交互体积功能 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	bool MyTraceMeshIsAlsoInteractionVolume = false;

	/** 配置 TraceMesh 的碰撞、重叠、排序及尺寸参数 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Trace")
	void MySetTraceMeshProperties();

	// ------------------------------------------------------------------
	// FPS-PRECISION-RESOLUTION 复合节点
	// ------------------------------------------------------------------
	/** 流体模拟横向分辨率，低于 8 时由初始化逻辑回退到 256。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	int32 MyResolutionX = 256;

	/** 流体模拟纵向分辨率，低于 8 时由初始化逻辑回退到 256。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	int32 MyResolutionY = 256;

	/** 允许的最高采样帧率，用于计算采样间隔。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	int32 MyMaxSamplingFPS = 60;

	/** 当前实际使用的采样帧率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	int32 MySamplingFPS = 60;

	// ------------------------------------------------------------------
	// LOD-DistaceStepsPrecalc 复合节点
	// ------------------------------------------------------------------
	/** LOD 阈值数量，同时作为当前 LOD 等级的初始值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|LOD")
	int32 MyLODSteps = 1;

	/** LOD 距离阈值的近端和远端边界。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|LOD")
	double MyLODNearBound = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|LOD")
	double MyLODFarBound = 0.0;

	/** 是否分别允许 LOD 降低模拟质量或采样帧率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|LOD")
	bool MyLOD1ReduceSimQuality = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|LOD")
	bool MyLOD2ReduceSamplingFPS = false;

	/** 当前 LOD 等级、相邻阈值间距及预计算出的整数距离阈值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|LOD")
	int32 MyLODLevel = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|LOD")
	double MyLODStepRange = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|LOD")
	TArray<double> MyLODStepsArray;

	/** 预计算 LOD 距离阈值；仅在任一 LOD 降级选项启用时刷新阈值数组。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|LOD")
	void MyLODDistaceStepsPrecalc();

	/** 执行 AfterBind 初始化流程。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Init")
	void MyAfterBind();

	/** 自定义 Tick 间隔，等于最高采样帧率的倒数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	double MyTickRateCustom = 1.0 / 60.0;

	/** 流体求解使用的数值精度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	EMySimPrecision MySimPrecision = EMySimPrecision::Bit16;

	/** 精度枚举转换后的材质选择索引，16 位为 0、32 位为 1。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	int32 MySimPrecisionIndex = 0;

	/** 是否使用半分辨率压力和散度缓冲区。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	bool MyHalfResPressureAndDivergenceBuffers = false;

	/** Pressure Solver 1 在当前 LOD 下的迭代次数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	int32 MyFluidSolver1Iterations = 0;

	/** Pressure Solver 1 的最大迭代次数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	int32 MyPressureSolver1MaxIterations = 0;

	/** Pressure Solver 2 的最大迭代次数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	int32 MyPressureSolver2MaxIterations = 0;

	/** 压力解算器随 LOD 变化的核缩放系数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	double MyPressureSolver2KernelReduction = 0.0;

	/** InputMaterials 的当前材质索引。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	int32 MyInputMaterialSelected = 0;

	/** TraceMesh 在量化模式下锁定移动的轴枚举值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	EMyQuantizerAxisIgnore MyMovementIsLockedOnThisAxis = EMyQuantizerAxisIgnore::X;

	/** 跟随 TraceMesh 的可选交互体积。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	TObjectPtr<UBoxComponent> MyInteractionVolume = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	bool MyInteractionVolumeIsPresent = false;

	/** LWC 关闭时是否跳过 Niagara 的位置参数写入。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	bool MyLWCAvoidNiagaraWarnings = false;

	/** 执行核心流体求解步骤；ThenExec 与 PainterV2Exec 分别对应原复合节点的两个执行出口。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Simulation")
	void MyCoreFluidsimOPs(bool& ThenExec, bool& PainterV2Exec);

	/** 将补充的流体参数写入 Composite、Gradient 与 Divergence 动态材质。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials|Solver")
	void MySetAdditionalFluidsimParams();

	/** 是否连接 Painter v2 追踪点生成轨迹线。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MyPV2_Connect_TrackpointsWithLines = false;

	/** 是否由 Painter v2 根据追踪点位移生成速度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MyPV2_GenerateVelocity = false;

	/** Painter v2 是否启用位置插值，取轨迹线和速度生成需求的逻辑或。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MyPV2_Interpolation = false;

	/** 初始化采样频率、精度索引、分辨率兜底及 Painter v2 联动参数。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Simulation")
	void MyFPSPrecisionResolution();

	/** 创建 Painter v2 的 Niagara 组件，并启动其参数初始化流程。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Niagara")
	void MyInitPainterV2();

	/** 将点画笔材质的标量参数同步到 Painter v2 Niagara，BrushSize 由 Niagara 单独控制。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Niagara")
	void MyForwardScalarParamsToNiagara();

	/** 是否将选定的内部模拟阶段绘制到外部 RenderTarget。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|RenderTarget|Export")
	bool MyDrawInternalRenderTargetToExternalEnabled = false;

	/** 与导出阶段列表按索引一一对应的外部 RenderTarget。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|RenderTarget|Export")
	TArray<TObjectPtr<UTextureRenderTarget2D>> MyExternalRenderTargets;

	/** 要导出的内部模拟阶段；顺序决定写入的外部 RenderTarget。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|RenderTarget|Export")
	TArray<EMyRenderTargetList> MyInternalRenderTargetsToExport;

	/** 将选定内部阶段的材质绘制到按索引匹配的外部 RenderTarget。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|RenderTarget|Export")
	void MyDrawInternalRenderTargetToExternal();

	/** Painter v2 使用的 Niagara 系统；索引 0/1 分别对应不连接/连接追踪点。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	TArray<TObjectPtr<UNiagaraSystem>> MyCoreNiagaraSystems;

	/** Painter v2 使用的 Niagara 组件；蓝图可读写，初始化会按原蓝图逻辑写入新创建的组件。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	TObjectPtr<UNiagaraComponent> MyNiagaraBasedPainter = nullptr;

	/** 原蓝图 PositionArray、LastPositionArray、VelocityArray、BrushSizeArray 的 C++ 映射。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	TArray<FVector2D> MyPositionArray;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	TArray<FVector2D> MyLastPositionArray;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	TArray<FLinearColor> MyVelocityArray;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	TArray<double> MyBrushSizeArray;

	/** Painter v2 的线条、速度和噪声参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	float MyPV2StopLineDrawingAboveThisVelocity = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	float MyAdjustPainterV2BrushStrength = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	float MyAdjustPainterV2BrushVeloNoise = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara", meta = (ClampMin = "0.0"))
	double MyPV2LineDrawingFailCooldownTime = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	TObjectPtr<UTexture> MyPainterV2BrushVeloNoiseTexture = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara", meta = (ClampMin = "0.0"))
	double MyNiagaraVariableSetSafetyDelay = 0.0;

	/** 按当前模拟配置创建或重建所有需要的 RenderTarget。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|RenderTarget")
	void MyCreateOrAcquireRenderTargets();

	/** 核心模拟材质；0、1 为点/线画笔，2~17 为八组桌面端/移动端材质变体。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TArray<TObjectPtr<UMaterialInterface>> MyCoreSimMaterials;

	/** 是否选用为移动端翻转过的核心模拟材质。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	bool MyFlipRenderTargetsForMobile = false;

	/** 画笔及求解器参数，由动态材质实例初始化时写入对应材质参数。 */
	/** 密度画笔噪声强度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MyBrushDensityNoiseScale = 0.0;
	/** 密度画笔噪声频率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MyBrushDensityNoiseFreq = 0.0;
	/** 速度画笔噪声强度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MyBrushVelocityNoiseScale = 0.0;
	/** 速度画笔噪声频率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MyBrushVelocityNoiseFreq = 0.0;
	/** 速度画笔的幂指数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MyBrushVelocityPow = 1.0;
	/** 是否在世界空间采样画笔噪声。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	bool MyBrushNoiseInWorldSpace = false;
	/** 触发画笔阻尼的最低速度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MyDampenBrushBelowThisVelocity = 0.0;
	/** 低速画笔的阻尼系数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MyDampenBrushFactor = 0.0;
	/** 是否允许密度输出为绝对黑色。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	bool MyAllowAbsoluteBlackDensity = false;
	/** Painter V2 边缘遮罩强度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MyAdjustPainterV2EdgeMask = 0.0;
	/** 模拟画笔速度参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MySpeed = 0.0;

	/** 由预设映射驱动的速度、密度和画笔参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloOffsetX = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloOffsetY = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloFromBrushMotion = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyOffsetFromSimAreaMotion = 1.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloStrength = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloRotate = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloAmpNoise = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloDirNoise = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyInputFeedback = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyBrushSize = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyBrushStrength = 0.0;
	/** 当前画笔是否与交互区域重叠。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	bool MyOverlap1 = false;
	/** 当前鼠标按钮是否按下。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	bool MyMousePressed = false;
	/** MuteBrush 计算出的实际画笔强度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MyBrushStrengthTemp1 = 0.001;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyBrushHardness = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyBrushPuncture = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	bool MyEraserMode = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyDensityTxtMult = 1.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyFadeDensityAtSimEdge = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MySimEdgeBouncyness = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloDirNoiseSize = 1.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloDirNoiseSpeed = 1.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyEdgeMaskWidth = 0.25;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyDensityTxtScale = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyDensityTxtOffsetX = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyDensityTxtOffsetY = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyBrushNoise = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloInputTile = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloInputOffsetSpeed = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyDensityInputNoiseAmp = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyDensityInputNoiseOffset = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyDensityInputNoiseTile = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyBrushRnd = 0.0;

	/** 速度场反馈系数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Solver")
	double MyFlowFeedback = 0.0;
	/** 散度求解强度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Solver")
	double MyDivergence = 0.0;
	/** 压力边缘遮罩强度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Solver")
	double MyPressureEdgeMasking = 0.0;
	/** 实验性压力反馈系数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Solver")
	double MyExperimentalPressureFeedback = 0.0;
	/** 实验性压力反馈分量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Solver")
	double MyExpPressureFeedbackComponent = 0.0;
	/** 实验性散度反馈分量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Solver")
	double MyExpDivergenceFeedbackComponent = 0.0;
	/** 第二压力求解器的核索引偏移。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Solver")
	int32 MyExperimentalPSolver2KernelIndexOffset = 0;
	/** 是否使用压力求解器 1；false 时使用默认求解器 2。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Solver")
	bool MyUsePressureSolver1DefaultIs2 = false;

	/** 外部密度输入纹理。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	TObjectPtr<UTexture> MyDensityInput = nullptr;
	/** 外部速度输入纹理。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	TObjectPtr<UTexture> MyVelocityInput = nullptr;
	/** 速度预设所在的数据表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	TObjectPtr<UDataTable> MyLoadedDataTable = nullptr;
	/** 速度预设的相对资源路径根。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	FString MyLoadedDataTablePath;
	/** 有效时优先替代预设速度纹理。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	TObjectPtr<UTexture2D> MyOverwritePresetVelocityInput = nullptr;
	/** 有效时优先替代预设密度纹理。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	TObjectPtr<UTexture2D> MyOverwritePresetDensityInput = nullptr;
	/** 碰撞遮罩纹理。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	TObjectPtr<UTexture> MyCollisionMask = nullptr;
	/** 作为模拟输入的 RenderTarget。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	TObjectPtr<UTexture> MyInputRenderTarget = nullptr;
	/** 作为模拟输入的媒体纹理。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	TObjectPtr<UMediaTexture> MyMediaTexture = nullptr;
	/** 媒体输入对应的播放器对象。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	TObjectPtr<UMediaPlayer> MyInputMediaPlayer = nullptr;
	/** 媒体输入的文件源。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	TObjectPtr<UFileMediaSource> MyInputMediaSource = nullptr;
	/** 媒体输入循环时长（秒）；非正值时不启用循环重播。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input", meta = (ClampMin = "0.0"))
	double MyInputMediaLoopLength = 0.0;
	/** 是否优先使用 InputRenderTarget 作为密度输入。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	bool MyUseRenderTargetAsInput = false;
	/** 是否随机化画笔噪声偏移。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	bool MyRandomizeNoiseOffsets = false;
	/** 是否随机化密度纹理偏移。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	bool MyRandomizeDensityTextureOffset = false;
	/** 当前碰撞遮罩是否为有效的非默认纹理，可由蓝图覆盖用于测试。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	bool MyCollisionMaskIsNonDefault = false;

	/** 按蓝图的默认遮罩名称规则更新碰撞遮罩状态。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials|Input")
	void MyUpdateCollisionMaskIsNonDefault();

	/** 配置场景捕捉或媒体源作为 Composite 的密度输入。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials|Input")
	void MyAlternativeInputsFedToCompositeDensityInput();

	/** 加载预设速度纹理，或优先使用覆盖纹理并同步 Composite 参数。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials|Input")
	void MyLoadVelocityInputTexture();

	/** 加载预设密度纹理，或优先使用覆盖纹理并同步画笔材质参数。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials|Input")
	void MyLoadDensityInputTexture();

	/** 材质实例准备完成后加载对应的输入纹理。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials|Input")
	void MyLoadTextures();

	/** 创建所有模拟所需的动态材质实例，并绑定当前 RenderTarget。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials")
	void MyCreateDynamicMaterialInstances();

	/** 完成 RenderTarget 创建后的材质、预设与 Painter 初始化流程。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Init")
	void MyAfterCreateRT();

	/** 创建主、次、三级输出 MID，并将模拟缓冲绑定到其标准参数。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials")
	void MyCreateOutputMaterialAndSetItOnTargetsStep01();

	/** 将输出动态材质应用到 TraceMesh 与按标签筛选的外部组件。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials")
	void MyCreateOutputMaterialAndSetItOnTargetsStep02();

	/** 将模拟 RenderTarget 与 TraceMesh 参数注入指定 ActorTag 下的 Niagara 组件。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Niagara")
	void MyCreateOutputMaterialAndSetItOnTargetsStep03();

	/** 合成与梯度阶段的动态材质实例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMICompositeAndGradient = nullptr;
	/** 平流阶段的动态材质实例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMIAdvection = nullptr;
	/** 散度阶段的动态材质实例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMIDivergence = nullptr;
	/** 第一压力循环的动态材质实例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMIPressureCycle1 = nullptr;
	/** 第二压力循环的动态材质实例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMIPressureCycle2 = nullptr;
	/** 线状碰撞画笔的动态材质实例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMICollisionPainterLine = nullptr;
	/** 点状碰撞画笔的动态材质实例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMICollisionPainterDot = nullptr;
	/** 碰撞画笔偏移阶段的动态材质实例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMICollisionPainterOffset = nullptr;
	/** 默认空输出使用的动态材质实例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMINull = nullptr;
	/** 主、次、三级输出材质对应的动态材质实例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMIOutput = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMISecondaryOutput = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMITertiaryOutput = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	bool MySecondaryMaterialsPresent = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	bool MyTertiaryMaterialsPresent = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	bool MyMaterialCollectionPresent = false;

	/** 禁用组件时用于 TraceMesh 的灰色材质。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TObjectPtr<UMaterialInterface> MyInactiveGrayMaterial = nullptr;

	/** TraceMesh 隐藏时使用的空材质。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TObjectPtr<UMaterialInterface> MyNullMaterial = nullptr;

	/** 是否让 TraceMesh 使用空材质而非主输出材质。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	bool MyTraceMeshInvisible = false;

	/** 主、次、三级输出材质要应用到的 Actor 标签。None 表示不应用。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Output")
	FName MyApply1stOutMatToActorsWithTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Output")
	FName MyApply2ndOutMatToActorsWithTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Output")
	FName MyApply3rdOutMatToActorsWithTag = NAME_None;

	/** 主、次、三级输出材质要应用到的组件标签。None 表示应用 Actor 的所有 PrimitiveComponent。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Output")
	FName MyApply1stOutMatToComponentsWithTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Output")
	FName MyApply2ndOutMatToComponentsWithTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Output")
	FName MyApply3rdOutMatToComponentsWithTag = NAME_None;

	/** 用于寻找待驱动 Niagara 组件的 Actor 标签。None 时跳过 Niagara 初始化。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	FName MyFeedTaggedActorNiagaraComponent = NAME_None;

	/** 已绑定模拟参数的 Niagara 组件。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	TArray<TObjectPtr<UNiagaraComponent>> MyNiagaraSystemsToDrive;

	/** 是否至少成功绑定一个 Niagara 组件。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	bool MyNiagaraSystemsPresent = false;

	/** 是否将压力与散度 RenderTarget 暴露给 Niagara。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	bool MyMakePressureAvailableForNiagara = false;

	/** 是否强制 Niagara 以 MyMaxSamplingFPS 单独更新并重新初始化。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	bool MyForceMaxSamplingFPSToNiagara = false;

	/** 是否写入大世界坐标位置变量 TraceMeshPosDouble。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	bool MyLWCSupport = false;

	/** 写入 Niagara 与输出材质的 TraceMesh 世界位置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	FVector MyTraceMeshPos = FVector::ZeroVector;
	
	/** 是否使用简单画笔模式；该模式跳过内存池连接。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	bool MySimplePainterMode = false;

	/** 是否使用 RGB 输入材质，连接流程初始化为 false。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	bool MyRGBInputMaterial = false;

private:
	/** 媒体输入循环重播的定时器。 */
	FTimerHandle MyInputMediaLoopTimer;
	FTimerHandle MyLoadTexturesTimer;
	FTimerHandle MyNiagaraPainterV2SafetyTimer;
	FTimerHandle MyNiagaraPainterV2CooldownTimer;

	/** 倒带并重播当前媒体输入。 */
	void MyRestartInputMedia();

protected:
	/** 启动 TraceMesh 就绪检查。 */
	virtual void BeginPlay() override;

	/** 在安全延迟后写入 Painter v2 的输入缓冲。 */
	void MySetPainterV2PaintbufferInput();

	/** 在线条绘制冷却期结束后写入 Painter v2 的其余参数。 */
	void MyFinalizePainterV2Setup();

	/** 对应蓝图两条执行支路共用的 Painter v2 参数设置链。 */
	void MyApplyPainterV2SharedParameters();

	/** 对应 SetTraceMeshProperties 内未连接 Reset 引脚的 DoOnce 状态。 */
	UPROPERTY(Transient)
	bool bMyTraceMeshInitialRotationCaptured = false;

	/** 外部 RT 导出节点首次执行后的数组校验状态。 */
	UPROPERTY(Transient)
	bool bMyExternalRenderTargetExportValidated = false;

	/** 外部 RT 导出节点的 Gate 状态；首次校验失败后保持关闭。 */
	UPROPERTY(Transient)
	bool bMyExternalRenderTargetExportGateOpen = false;

};


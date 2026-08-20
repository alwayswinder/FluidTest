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
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UFileMediaSource;
class UMediaPlayer;
class UMediaTexture;

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

	/** TraceMesh 组件引用 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	TObjectPtr<UStaticMeshComponent> MyTraceMeshComponent = nullptr;

	/** TraceMesh 父级当前位置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	FVector MyTraceMeshParentPos = FVector::ZeroVector;

	/** TraceMesh 父级上一帧位置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	FVector MyTraceMeshParentLastPos = FVector::ZeroVector;

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

	/** 预设名过滤条件 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|System")
	FName MyPresetNameFilterCriteria = NAME_None;

	/** UE5 早期访问版本标志（早于 UE 5.4 的版本为 false，用于修复物理速度 bug） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|System")
	bool MyUE5EAFLAG = false;

	/** 量化步长（米），由 MyQuantizerValues 计算 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Quantizer")
	int32 MyQuantizerStepSize = 0;

	/** 是否启用画笔双缓冲（PaintBuffer WorldSpace 偏移或 Painter v2 时需要） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Quantizer")
	bool MyEnablePainterDoubleBuffering = false;

	/** TraceMesh 是否在世界空间移动（量化模式） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Quantizer")
	EMyQuantizerMode MyTraceMeshMovingInWorldSpace = EMyQuantizerMode::NoQuantizerTextureOffsetAutomatic;

	/** 输出材质数组 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TArray<TObjectPtr<UMaterialInterface>> MyOutputMaterials;

	/** 是否使用 Painter v2 追踪物体（强制双缓冲） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MyUsePAINTER_V2_ToTrackObjects = true;

	/** TraceMesh 是否面向相机（与量化不兼容） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MyCameraFacingTraceMesh = false;

	/** 旧版单目标模式 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MySingleTargetMode_LEGACY = false;

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

	/** 创建所有模拟所需的动态材质实例，并绑定当前 RenderTarget。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials")
	void MyCreateDynamicMaterialInstances();

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
	
	/** 是否使用简单画笔模式；该模式跳过内存池连接。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	bool MySimplePainterMode = false;

	/** 是否使用 RGB 输入材质，连接流程初始化为 false。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	bool MyRGBInputMaterial = false;

private:
	/** 媒体输入循环重播的定时器。 */
	FTimerHandle MyInputMediaLoopTimer;

	/** 倒带并重播当前媒体输入。 */
	void MyRestartInputMedia();

	/** 对应 SetTraceMeshProperties 内未连接 Reset 引脚的 DoOnce 状态。 */
	UPROPERTY(Transient)
	bool bMyTraceMeshInitialRotationCaptured = false;

};


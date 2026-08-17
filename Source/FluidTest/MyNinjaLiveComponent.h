// MyNinjaLiveComponent.h — FluidNinjaLive 的 NinjaLiveComponent 蓝图对应的 C++ 父类（逐步迁移）
// 命名规范：类/函数/变量加 My 前缀；蓝图父类修改在编辑器中进行，C++ 只提供父类骨架。

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInterface.h"
#include "MyNinjaFluidEnums.h"
#include "MyNinjaLiveComponent.generated.h"

class AMyNinjaLiveActor;

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

	/** 是否将调试消息保存到默认日志 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Debug")
	bool MySaveDebugMessagesToDefaultLog = false;

	/** 检查并初始化光线方向提供者（EnableRayMarching 开启时有效） */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Light")
	void MyLightDirectionProviderCheck();
};

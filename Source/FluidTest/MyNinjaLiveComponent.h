// MyNinjaLiveComponent.h — FluidNinjaLive 的 NinjaLiveComponent 蓝图对应的 C++ 父类（逐步迁移）
// 命名规范：类/函数/变量加 My 前缀；蓝图父类修改在编辑器中进行，C++ 只提供父类骨架。

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
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
};

// MyNinjaLiveActor.h — FluidNinjaLive 的 NinjaLive 蓝图对应的 C++ 父类（逐步迁移）
// 数组等数据属于 NinjaLiveComponent（见 UMyNinjaLiveComponent），Actor 通过组件引用访问。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyNinjaFluidEnums.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "MyNinjaLiveActor.generated.h"

class UMyNinjaLiveComponent;

/**
 * NinjaLive 蓝图 Actor 的 C++ 父类。
 */
UCLASS(Blueprintable, BlueprintType)
class FLUIDTEST_API AMyNinjaLiveActor : public AActor
{
	GENERATED_BODY()

public:
	AMyNinjaLiveActor();

	/** 获取 NinjaLiveComponent 组件引用 */
	UFUNCTION(BlueprintPure, Category = "FluidSim|Component")
	UMyNinjaLiveComponent* GetNinjaLiveComponent() const;

	/** 用户输入方式（UserInput_Enum，默认 Mouse single） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Input")
	EMyUserInput MyUserInputBasedInteraction = EMyUserInput::MouseSingle;

	/** 多触点输入中允许参与追踪的手指索引。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Input")
	TArray<bool> MyMultipleTouchLookup;

	/** 是否在 Pawn 靠近时激活模拟（性能优化，关闭则始终运行） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	bool MySimActivatedByPawnProximity = false;

	/** 是否禁用蓝图（关闭模拟） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	bool MyDisableBlueprint = false;

	/** 是否使用 TraceMesh 作为交互体积 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	bool MyUseTraceMeshAsInteractionVolume = false;

	/** 非持续交互时用于定位交互骨骼的包含列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<FName> MyOverlapFilterInclusiveBoneNameExact;

	/** 根 SceneComponent */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FluidSim|Activation")
	TObjectPtr<USceneComponent> MyRoot = nullptr;

	/** 激活体积（BoxCollision，Pawn 进入时激活模拟，挂载在 MyRoot 下） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FluidSim|Activation")
	TObjectPtr<UBoxComponent> MyActivationVolume = nullptr;
	/** 交互体积（BoxCollision，用于交互输入） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FluidSim|Interaction")
	TObjectPtr<UBoxComponent> MyInteractionVolume = nullptr;
	/** 追踪网格（StaticMesh，用于射线检测交互） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FluidSim|Trace")
	TObjectPtr<UStaticMeshComponent> MyTraceMesh = nullptr;
};

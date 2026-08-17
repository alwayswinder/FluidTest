// MyNinjaLiveActor.h — FluidNinjaLive 的 NinjaLive 蓝图对应的 C++ 父类（逐步迁移）
// 数组等数据属于 NinjaLiveComponent（见 UMyNinjaLiveComponent），Actor 通过组件引用访问。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyNinjaFluidEnums.h"
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

	/** 是否在 Pawn 靠近时激活模拟（性能优化，关闭则始终运行） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	bool MySimActivatedByPawnProximity = false;

	/** 是否禁用蓝图（关闭模拟） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	bool MyDisableBlueprint = false;
};

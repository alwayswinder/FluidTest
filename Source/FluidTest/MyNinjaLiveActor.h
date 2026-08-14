// MyNinjaLiveActor.h — 复刻 FluidNinjaLive 的 NinjaLive 蓝图（逐步迁移）
//
// 迁移策略（小步增量，逐步替换蓝图）：
//   第 1 步：创建极简 C++ 父类，蓝图 NinjaLive 改为继承它（已完成）。
//   第 2 步（本文件）：提供组件访问辅助 GetNinjaLiveComponent()。
//             数组与数组函数（TempArray0~39 / ResetTempArrays / GetTempArray）
//             属于 NinjaLiveComponent，已迁移到 UMyNinjaLiveComponent，
//             Actor 蓝图通过组件引用访问它们（与蓝图原实现一致）。
//   后续步骤：每次把蓝图中的一个函数/变量迁移为 C++ 虚函数/属性，
//             迁移一次、测试一次，直到完整替换。
//
// 蓝图位置：/Game/_MyTest/Fluid/Bp/NinjaLive（Actor 蓝图，内含 NinjaLiveComponent 组件实例）
// 类名/函数前缀 My：避免与蓝图内同名类型混淆。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyNinjaFluidEnums.h"
#include "MyNinjaLiveActor.generated.h"

class UMyNinjaLiveComponent;

/**
 * NinjaLive 蓝图 Actor 的 C++ 父类。
 * Actor 自身不持有临时数组；数组属于其 NinjaLiveComponent 组件，
 * 通过 GetNinjaLiveComponent() 获取类型化组件引用后访问。
 */
UCLASS(Blueprintable, BlueprintType)
class FLUIDTEST_API AMyNinjaLiveActor : public AActor
{
	GENERATED_BODY()

public:
	AMyNinjaLiveActor();

	/** 获取 NinjaLiveComponent 组件引用（对应蓝图中的 NinjaLiveComponent 组件） */
	UFUNCTION(BlueprintPure, Category = "FluidSim|Component")
	UMyNinjaLiveComponent* GetNinjaLiveComponent() const;

	// ------------------------------------------------------------------
	// 复刻蓝图变量 UserInputBasedInteraction（UserInput_Enum）
	// 默认 Mouse single，与蓝图 UserInput_Enum 显示名一致
	// ------------------------------------------------------------------
	/** 用户输入方式（复刻蓝图 UserInputBasedInteraction，默认 Mouse single） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Input")
	EMyUserInput MyUserInputBasedInteraction = EMyUserInput::MouseSingle;
};

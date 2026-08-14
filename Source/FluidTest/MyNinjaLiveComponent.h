// MyNinjaLiveComponent.h — 复刻 FluidNinjaLive 的 NinjaLiveComponent 蓝图（第一步：C++ 父类）
//
// 迁移策略（小步增量，逐步替换蓝图）：
//   第 1 步（本文件）：创建极简 C++ 父类，蓝图 NinjaLiveComponent 改为继承它。
//                      此阶段父类**不添加任何逻辑/子对象**，仅提供骨架；
//                      蓝图原有逻辑完全不变，替换后应无任何行为差异。
//   后续步骤：每次把蓝图中的一个函数/变量迁移为 C++ 虚函数/属性，
//             迁移一次、测试一次，直到完整替换。
//
// 蓝图位置：/Game/_MyTest/Fluid/Bp/NinjaLiveComponent（已从原 FluidNinjaLive 目录移入测试目录）
// 类名前缀 My：避免与蓝图内同名类型混淆。
// 注意：修改蓝图父类的操作在编辑器中进行（改蓝图后需重新编译保存），
//       C++ 侧只负责提供父类骨架。

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MyNinjaLiveComponent.generated.h"

/**
 * NinjaLiveComponent 蓝图组件的 C++ 父类。
 * 极简空壳：仅继承 UActorComponent，不添加默认子对象、不启用额外 Tick，
 * 保证蓝图替换父类后原有组件与逻辑完全不受影响。
 * Blueprintable + BlueprintType：允许被蓝图继承/在蓝图中创建。
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup = (FluidSim), meta = (BlueprintSpawnableComponent))
class FLUIDTEST_API UMyNinjaLiveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMyNinjaLiveComponent();
};

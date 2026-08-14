// MyNinjaLiveActor.h — 复刻 FluidNinjaLive 的 NinjaLive 蓝图（第一步：C++ 父类）
//
// 迁移策略（小步增量，逐步替换蓝图）：
//   第 1 步（本文件）：创建极简 C++ 父类，蓝图 NinjaLive 改为继承它。
//                      此阶段父类**不添加任何组件**，避免与蓝图内已有组件冲突；
//                      蓝图原有逻辑完全不变，替换后应无任何行为差异。
//   后续步骤：每次把蓝图中的一个函数/变量迁移为 C++ 虚函数/属性，
//             迁移一次、测试一次，直到完整替换。
//
// 类名前缀 My：避免与蓝图内同名类型混淆。
// 注意：修改蓝图父类的操作在编辑器中进行（改蓝图后需重新编译保存），
//       C++ 侧只负责提供父类骨架。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyNinjaLiveActor.generated.h"

/**
 * NinjaLive 蓝图 Actor 的 C++ 父类。
 * 极简空壳：仅继承 AActor，无默认子对象、无 Tick、无额外依赖，
 * 保证蓝图替换父类后原有组件与逻辑完全不受影响。
 * Blueprintable + BlueprintType：允许被蓝图继承/在蓝图中创建。
 */
UCLASS(Blueprintable, BlueprintType)
class FLUIDTEST_API AMyNinjaLiveActor : public AActor
{
	GENERATED_BODY()

public:
	AMyNinjaLiveActor();
};

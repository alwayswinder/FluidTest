// MyNinjaLiveActor.cpp — AMyNinjaLiveActor 极简父类实现
// 第一步仅提供空构造函数：不添加组件、不启用 Tick，保证蓝图替换父类后零行为差异。

#include "MyNinjaLiveActor.h"

AMyNinjaLiveActor::AMyNinjaLiveActor()
{
	// 极简空壳：不创建任何默认子对象。
	// 蓝图侧已有的组件（如 NinjaLiveComponent、TraceMesh 等）不受影响。
	PrimaryActorTick.bCanEverTick = false;
}

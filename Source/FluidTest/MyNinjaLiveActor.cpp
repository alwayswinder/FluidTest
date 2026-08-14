// MyNinjaLiveActor.cpp — AMyNinjaLiveActor 实现
// Actor 不持有临时数组；数组属于 NinjaLiveComponent 组件，通过组件引用访问。

#include "MyNinjaLiveActor.h"

#include "MyNinjaLiveComponent.h"

AMyNinjaLiveActor::AMyNinjaLiveActor()
{
	// 不创建任何默认子对象（蓝图侧组件不受影响），不启用 Tick。
	PrimaryActorTick.bCanEverTick = false;
}

UMyNinjaLiveComponent* AMyNinjaLiveActor::GetNinjaLiveComponent() const
{
	// 按类型查找组件（蓝图中的 NinjaLiveComponent 实例）
	return FindComponentByClass<UMyNinjaLiveComponent>();
}

// MyNinjaLiveActor.cpp — AMyNinjaLiveActor 实现

#include "MyNinjaLiveActor.h"

#include "MyNinjaLiveComponent.h"

AMyNinjaLiveActor::AMyNinjaLiveActor()
{
	// 不创建默认子对象（蓝图侧组件不受影响），不启用 Tick
	PrimaryActorTick.bCanEverTick = false;
}

UMyNinjaLiveComponent* AMyNinjaLiveActor::GetNinjaLiveComponent() const
{
	return FindComponentByClass<UMyNinjaLiveComponent>();
}

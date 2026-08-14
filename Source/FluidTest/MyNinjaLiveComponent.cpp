// MyNinjaLiveComponent.cpp — UMyNinjaLiveComponent 极简父类实现
// 第一步仅提供空构造函数：不启用额外 Tick、不添加子对象，保证蓝图替换父类后零行为差异。

#include "MyNinjaLiveComponent.h"

UMyNinjaLiveComponent::UMyNinjaLiveComponent()
{
	// 极简空壳：不启用 Tick（蓝图侧若需要 Tick，可在蓝图里自行开启）。
	PrimaryComponentTick.bCanEverTick = false;
}

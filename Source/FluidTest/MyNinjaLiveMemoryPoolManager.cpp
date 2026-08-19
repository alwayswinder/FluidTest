// MyNinjaLiveMemoryPoolManager.cpp — AMyNinjaLiveMemoryPoolManager 实现

#include "MyNinjaLiveMemoryPoolManager.h"

AMyNinjaLiveMemoryPoolManager::AMyNinjaLiveMemoryPoolManager()
{
	// 占位父类不执行 Actor Tick。
	PrimaryActorTick.bCanEverTick = false;
}

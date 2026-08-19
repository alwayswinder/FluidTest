// MyNinjaLiveFunctions.cpp — UMyNinjaLiveFunctions 实现

#include "MyNinjaLiveFunctions.h"

#include "MyNinjaLiveMemoryPoolManager.h"
#include "Kismet/KismetRenderingLibrary.h"

UTextureRenderTarget2D* UMyNinjaLiveFunctions::MyCreateRenderTarget(
	UObject* WorldContextObject,
	int32 Width,
	int32 Height,
	TEnumAsByte<ETextureRenderTargetFormat> Format,
	bool Clamping,
	TEnumAsByte<TextureGroup> LODgroup,
	TEnumAsByte<TextureFilter> Filter)
{
	UTextureRenderTarget2D* RTout = UKismetRenderingLibrary::CreateRenderTarget2D(
		WorldContextObject,
		Width,
		Height,
		Format,
		FLinearColor::Black,
		false,
		false);

	if (!IsValid(RTout))
	{
		return nullptr;
	}

	RTout->AddressX = Clamping ? TA_Clamp : TA_Wrap;
	RTout->AddressY = Clamping ? TA_Clamp : TA_Wrap;
	RTout->Filter = Filter;
	RTout->LODGroup = LODgroup;
	return RTout;
}

void UMyNinjaLiveFunctions::MyAcquireRenderTargetsFromPool(
	int32 Request,
	int32 HostRenderTGListIndex,
	const TArray<FString>& RenderTargetList,
	AMyNinjaLiveMemoryPoolManager* MemoryPoolManager,
	TMap<FString, UTextureRenderTarget2D*>& RenderTargetsMapTmp)
{
	RenderTargetsMapTmp.Empty();

	if (!IsValid(MemoryPoolManager) || !RenderTargetList.IsValidIndex(HostRenderTGListIndex))
	{
		return;
	}

	TArray<FMyRenderTargetListItem>* Pool = nullptr;
	switch (Request)
	{
	case 0:
		Pool = &MemoryPoolManager->MyRGBA_RenderTargetsList;
		break;
	case 1:
		Pool = &MemoryPoolManager->MyRG_RenderTargetsList;
		break;
	case 2:
		Pool = &MemoryPoolManager->MyR_RenderTargetsList;
		break;
	default:
		return;
	}

	for (FMyRenderTargetListItem& Item : *Pool)
	{
		if (!Item.MyFree)
		{
			continue;
		}

		RenderTargetsMapTmp.Add(RenderTargetList[HostRenderTGListIndex], Item.MyRT);
		// 对应 VariableSetRef：将已分配的 RenderTarget 标记为占用。
		Item.MyFree = false;
		return;
	}
}

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

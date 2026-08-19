// MyNinjaLiveFunctions.h — NinjaLiveFunctions 蓝图函数库的 C++ 占位父类

#pragma once

#include "CoreMinimal.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyNinjaLiveFunctions.generated.h"

class AMyNinjaLiveMemoryPoolManager;

/** NinjaLiveFunctions 蓝图函数库的 C++ 迁移入口。 */
UCLASS()
class FLUIDTEST_API UMyNinjaLiveFunctions : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 创建并配置一个 2D RenderTarget，返回蓝图中的 RTout。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|RenderTarget", meta = (WorldContext = "WorldContextObject"))
	static UTextureRenderTarget2D* MyCreateRenderTarget(
		UObject* WorldContextObject,
		int32 Width = 256,
		int32 Height = 256,
		TEnumAsByte<ETextureRenderTargetFormat> Format = RTF_R8,
		bool Clamping = false,
		TEnumAsByte<TextureGroup> LODgroup = TEXTUREGROUP_World,
		TEnumAsByte<TextureFilter> Filter = TF_Bilinear);
	
};

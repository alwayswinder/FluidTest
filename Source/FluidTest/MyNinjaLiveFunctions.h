// MyNinjaLiveFunctions.h — NinjaLiveFunctions 蓝图函数库的 C++ 占位父类

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/EngineTypes.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyNinjaLiveFunctions.generated.h"

class AMyNinjaLiveMemoryPoolManager;
class USceneComponent;
class AActor;

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

	/** 按预设行的 SourceString 定位并加载模板资源。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Template", meta = (WorldContext = "WorldContextObject"))
	static void MyTemplateLoader(
		UObject* WorldContextObject,
		FName TemplateDefinition,
		UDataTable* LoadedDataTable,
		const FString& LoadedDatatablePath,
		bool& LoadFailed,
		UObject*& LoadedTemplateObject,
		FString& LoadedTmpFullPath,
		FString& LoadedTemplateNameOnly,
		bool& UsesAbsolutePath);

	/** 加载指定预设的数据表，并将每行 SourceString 转换为预设数值。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Preset", meta = (WorldContext = "WorldContextObject"))
	static void MyPresetLoader(
		UObject* WorldContextObject,
		const FString& PresetName,
		const TArray<FName>& AssetPath,
		FName AssetTrimmedName,
		bool ForcePreferredPreset,
		UDataTable* PreferredPreset,
		UDataTable*& LoadedDataTable,
		FString& LoadedDataTablePath,
		TMap<FString, double>& PresetMap);

	/** 按原蓝图的相机旋转、LookAt 与 LockY 分支设置组件世界旋转。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Camera", meta = (WorldContext = "WorldContextObject"))
	static void MyCameraFacing(
		UObject* WorldContextObject,
		USceneComponent* InMesh,
		bool UseLegacyFacing,
		bool LockY,
		FRotator TraceMeshInitRot);

	/** 从 Start 追踪到 End 生成画笔命中信息（占位：内部实现待补全）。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Trace", meta = (WorldContext = "WorldContextObject"))
	static void MyTraceOverlap(
		UObject* WorldContextObject,
		FVector Start,
		FVector End,
		double TracelineOvershoot,
		TEnumAsByte<ETraceTypeQuery> TraceChannel,
		UPARAM(ref) TArray<AActor*>& FluidNinjaLIVEActors,
		bool PainterV2,
		FLinearColor& HitUV,
		FVector& TracePosition,
		bool& HitValid);
	
};



#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/EngineTypes.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyNinjaLiveFunctions.generated.h"

class AMyNinjaLiveMemoryPoolManager;
class USceneComponent;
class UPrimitiveComponent;
class AActor;


UCLASS()
class FLUIDTEST_API UMyNinjaLiveFunctions : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "FluidSim|RenderTarget", meta = (WorldContext = "WorldContextObject"))
	static UTextureRenderTarget2D* MyCreateRenderTarget(
		UObject* WorldContextObject,
		int32 Width = 256,
		int32 Height = 256,
		TEnumAsByte<ETextureRenderTargetFormat> Format = RTF_R8,
		bool Clamping = false,
		TEnumAsByte<TextureGroup> LODgroup = TEXTUREGROUP_World,
		TEnumAsByte<TextureFilter> Filter = TF_Bilinear);


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


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Camera", meta = (WorldContext = "WorldContextObject"))
	static void MyCameraFacing(
		UObject* WorldContextObject,
		USceneComponent* InMesh,
		bool UseLegacyFacing,
		bool LockY,
		FRotator TraceMeshInitRot);


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Trace", meta = (WorldContext = "WorldContextObject"))
	static void MyTraceMouse(
		UObject* WorldContextObject,
		UPrimitiveComponent* HitComponent,
		bool TouchSensitive,
		uint8 FingerIndex,
		TEnumAsByte<ETraceTypeQuery> TraceChannel,
		const TArray<AActor*>& FluidNinjaLIVEActors,
		FLinearColor& HitUV,
		bool& SimHitByMouse,
		bool& MouseClickValid,
		bool& TouchValid);


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

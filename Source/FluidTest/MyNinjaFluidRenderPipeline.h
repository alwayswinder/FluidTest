#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;
class UMyNinjaLiveComponent;
class UTextureRenderTarget2D;

enum class EMyNinjaRDGDiffTarget : uint8
{
	Output,
	Advection,
	Divergence,
	Pressure,
	PressureTemp
};

class FMyNinjaFluidRenderPipeline
{
public:
	static bool MyUseRDGOutput();
	static bool MyIsOutputValidationEnabled();
	static bool MyShouldValidateOutput(uint64 FrameIndex);
	static bool MyUseRDGCore();
	static bool MyIsCoreValidationEnabled();
	static bool MyShouldValidateCore(uint64 FrameIndex);
	static bool MyUseRDGPressure();
	static bool MyIsPressureValidationEnabled();
	static bool MyShouldValidatePressure(uint64 FrameIndex);
	static void MyPollOutputDiffs();
	static void MyShutdown();
	static void MyDrawOutput(
		UObject* WorldContextObject,
		UTextureRenderTarget2D* OutputTarget,
		UMaterialInterface* OutputMaterial,
		UTextureRenderTarget2D* ComparisonTarget,
		UMyNinjaLiveComponent* Component,
		uint64 SampleId);
	static void MyDrawAdvectionDivergence(
		UObject* WorldContextObject,
		UTextureRenderTarget2D* AdvectionTarget,
		UMaterialInterface* AdvectionMaterial,
		UTextureRenderTarget2D* DivergenceTarget,
		UMaterialInterface* DivergenceMaterial,
		UTextureRenderTarget2D* ComparisonAdvectionTarget,
		UMaterialInterface* ComparisonAdvectionMaterial,
		UTextureRenderTarget2D* ComparisonDivergenceTarget,
		UMaterialInterface* ComparisonDivergenceMaterial,
		UMyNinjaLiveComponent* Component,
		uint64 SampleId);
	static void MyCopyPressureTargets(
		UTextureRenderTarget2D* SourcePressureTarget,
		UTextureRenderTarget2D* DestinationPressureTarget,
		UTextureRenderTarget2D* SourcePressureTempTarget,
		UTextureRenderTarget2D* DestinationPressureTempTarget);
	static void MyDrawPressurePair(
		UObject* WorldContextObject,
		UTextureRenderTarget2D* PressureTarget,
		UTextureRenderTarget2D* PressureTempTarget,
		UMaterialInterface* PressureCycle1Material,
		UMaterialInterface* PressureCycle2Material,
		bool bUseRDG);
	static void MyComparePressureTargets(
		UObject* WorldContextObject,
		UTextureRenderTarget2D* ReferencePressureTarget,
		UTextureRenderTarget2D* CandidatePressureTarget,
		UTextureRenderTarget2D* ReferencePressureTempTarget,
		UTextureRenderTarget2D* CandidatePressureTempTarget,
		UMyNinjaLiveComponent* Component,
		uint64 SampleId);
};

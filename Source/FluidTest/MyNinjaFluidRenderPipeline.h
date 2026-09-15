#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;
class UMyNinjaLiveComponent;
class UTextureRenderTarget2D;

class FMyNinjaFluidRenderPipeline
{
public:
	static bool MyUseRDGOutput();
	static bool MyIsOutputValidationEnabled();
	static bool MyShouldValidateOutput(uint64 FrameIndex);
	static void MyPollOutputDiffs();
	static void MyShutdown();
	static void MyDrawOutput(
		UObject* WorldContextObject,
		UTextureRenderTarget2D* OutputTarget,
		UMaterialInterface* OutputMaterial,
		UTextureRenderTarget2D* ComparisonTarget,
		UMyNinjaLiveComponent* Component,
		uint64 SampleId);
};

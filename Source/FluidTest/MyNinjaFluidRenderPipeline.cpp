#include "FluidTest/MyNinjaFluidRenderPipeline.h"

#include "Async/Async.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "FluidTest/MyNinjaLiveComponent.h"
#include "GlobalShader.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInterface.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderingThread.h"
#include "RHI.h"
#include "RHIGPUReadback.h"
#include "ShaderParameterStruct.h"

#include <atomic>

namespace
{
	TAutoConsoleVariable<int32> CVarMyNinjaOutputRenderPath(
		TEXT("FluidTest.NinjaLive.OutputRenderPath"),
		0,
		TEXT("0 uses the legacy output draw path. 1 uses the RDG output draw path."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarMyNinjaOutputRDGValidation(
		TEXT("FluidTest.NinjaLive.OutputRDGValidation"),
		0,
		TEXT("Enables asynchronous GPU comparison between legacy and RDG output draws."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarMyNinjaOutputRDGValidationInterval(
		TEXT("FluidTest.NinjaLive.OutputRDGValidationInterval"),
		30,
		TEXT("Number of output frames between RDG validation samples."),
		ECVF_Default);

	class FMyNinjaOutputDiffCS : public FGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FMyNinjaOutputDiffCS);
		SHADER_USE_PARAMETER_STRUCT(FMyNinjaOutputDiffCS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER(FIntPoint, TextureExtent)
			SHADER_PARAMETER(float, Tolerance)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D<float4>, ReferenceTexture)
			SHADER_PARAMETER_RDG_TEXTURE(Texture2D<float4>, CandidateTexture)
			SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, ResultBuffer)
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
		{
			return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
		}
	};

	IMPLEMENT_GLOBAL_SHADER(FMyNinjaOutputDiffCS, "/Project/Private/MyNinjaOutputDiff.usf", "MainCS", SF_Compute);

	struct FMyNinjaPendingOutputDiff
	{
		TSharedPtr<FRHIGPUBufferReadback, ESPMode::ThreadSafe> Readback;
		TWeakObjectPtr<UMyNinjaLiveComponent> Component;
		int64 SampleId = 0;
		int32 ComparedPixelCount = 0;
		float Tolerance = 0.0f;
	};

	TArray<FMyNinjaPendingOutputDiff> GMyNinjaPendingOutputDiffs;
	std::atomic<int32> GMyNinjaPendingOutputDiffCount = 0;
	std::atomic_bool GMyNinjaOutputDiffPollQueued = false;
	std::atomic_bool GMyNinjaOutputDiffShuttingDown = false;
	constexpr int32 GMyNinjaMaxPendingOutputDiffs = 4;

	float MyGetOutputTolerance(const UTextureRenderTarget2D* OutputTarget)
	{
		if (OutputTarget->RenderTargetFormat == RTF_RGBA8)
		{
			return 0.0f;
		}
		return OutputTarget->RenderTargetFormat == RTF_RGBA32f ? 0.000001f : 0.001f;
	}

	void MyProcessCompletedOutputDiffs()
	{
		check(IsInRenderingThread());
		for (int32 Index = GMyNinjaPendingOutputDiffs.Num() - 1; Index >= 0; --Index)
		{
			FMyNinjaPendingOutputDiff& Pending = GMyNinjaPendingOutputDiffs[Index];
			if (!Pending.Readback->IsReady())
			{
				continue;
			}

			const uint32* Result = static_cast<const uint32*>(Pending.Readback->Lock(sizeof(uint32) * 5));
			const FLinearColor MaxDifference(
				FPlatformMath::AsFloat(Result[0]),
				FPlatformMath::AsFloat(Result[1]),
				FPlatformMath::AsFloat(Result[2]),
				FPlatformMath::AsFloat(Result[3]));
			const int32 ExceededPixelCount = static_cast<int32>(Result[4]);
			Pending.Readback->Unlock();

			const TWeakObjectPtr<UMyNinjaLiveComponent> Component = Pending.Component;
			const int64 SampleId = Pending.SampleId;
			const int32 ComparedPixelCount = Pending.ComparedPixelCount;
			const float Tolerance = Pending.Tolerance;
			if (!GMyNinjaOutputDiffShuttingDown.load(std::memory_order_acquire))
			{
				AsyncTask(ENamedThreads::GameThread,
					[Component, SampleId, MaxDifference, ExceededPixelCount, ComparedPixelCount, Tolerance]()
					{
						if (UMyNinjaLiveComponent* ValidComponent = Component.Get())
						{
							ValidComponent->MyApplyRDGOutputDiffResult(
								SampleId, MaxDifference, ExceededPixelCount, ComparedPixelCount, Tolerance);
						}
					});
			}

			GMyNinjaPendingOutputDiffs.RemoveAtSwap(Index, 1, EAllowShrinking::No);
		}
		GMyNinjaPendingOutputDiffCount.store(GMyNinjaPendingOutputDiffs.Num(), std::memory_order_release);
	}

	FRDGTextureRef MyAddOutputMaterialPass(
		FRDGBuilder& GraphBuilder,
		FTextureRenderTargetResource* TargetResource,
		const FMaterialRenderProxy* MaterialRenderProxy,
		FIntPoint Extent,
		const FGameTime& Time,
		ERHIFeatureLevel::Type FeatureLevel,
		const TCHAR* TextureName)
	{
		FRDGTextureRef TargetTexture = RegisterExternalTexture(
			GraphBuilder, TargetResource->GetRenderTargetTexture(), TextureName);
		FCanvas& Canvas = *FCanvas::Create(GraphBuilder, TargetTexture, nullptr, Time, FeatureLevel);
		Canvas.SetRenderTargetRect(FIntRect(FIntPoint::ZeroValue, Extent));
		FCanvasTileItem TileItem(FVector2D::ZeroVector, MaterialRenderProxy, FVector2D(Extent));
		TileItem.SetColor(FLinearColor::White);
		Canvas.DrawItem(TileItem);
		Canvas.Flush_RenderThread(GraphBuilder, false);
		GraphBuilder.SetTextureAccessFinal(TargetTexture, ERHIAccess::SRVMask);
		return TargetTexture;
	}

	void MyAddOutputDiffPass(
		FRDGBuilder& GraphBuilder,
		FRDGTextureRef ReferenceTexture,
		FRDGTextureRef CandidateTexture,
		FIntPoint Extent,
		float Tolerance,
		ERHIFeatureLevel::Type FeatureLevel,
		TWeakObjectPtr<UMyNinjaLiveComponent> Component,
		int64 SampleId)
	{
		MyProcessCompletedOutputDiffs();
		if (GMyNinjaPendingOutputDiffs.Num() >= GMyNinjaMaxPendingOutputDiffs)
		{
			return;
		}

		FRDGBufferDesc ResultDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), 5);
		ResultDesc.Usage = EBufferUsageFlags(ResultDesc.Usage | BUF_SourceCopy);
		FRDGBufferRef ResultBuffer = GraphBuilder.CreateBuffer(ResultDesc, TEXT("FluidTest.NinjaLive.OutputDiffResult"));
		FRDGBufferUAVRef ResultUAV = GraphBuilder.CreateUAV(ResultBuffer, PF_R32_UINT);
		AddClearUAVPass(GraphBuilder, ResultUAV, 0u);

		FMyNinjaOutputDiffCS::FParameters* PassParameters =
			GraphBuilder.AllocParameters<FMyNinjaOutputDiffCS::FParameters>();
		PassParameters->TextureExtent = Extent;
		PassParameters->Tolerance = Tolerance;
		PassParameters->ReferenceTexture = ReferenceTexture;
		PassParameters->CandidateTexture = CandidateTexture;
		PassParameters->ResultBuffer = ResultUAV;

		TShaderMapRef<FMyNinjaOutputDiffCS> ComputeShader(GetGlobalShaderMap(FeatureLevel));
		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("FluidTest.NinjaLive.OutputDiff"),
			ComputeShader,
			PassParameters,
			FIntVector(FMath::DivideAndRoundUp(Extent.X, 8), FMath::DivideAndRoundUp(Extent.Y, 8), 1));

		TSharedPtr<FRHIGPUBufferReadback, ESPMode::ThreadSafe> Readback =
			MakeShared<FRHIGPUBufferReadback, ESPMode::ThreadSafe>(TEXT("FluidTest.NinjaLive.OutputDiffReadback"));
		AddEnqueueCopyPass(GraphBuilder, Readback.Get(), ResultBuffer, sizeof(uint32) * 5);
		GMyNinjaPendingOutputDiffs.Add({ Readback, Component, SampleId, Extent.X * Extent.Y, Tolerance });
		GMyNinjaPendingOutputDiffCount.store(GMyNinjaPendingOutputDiffs.Num(), std::memory_order_release);
	}

	void MyDrawOutputRDG(
		UWorld* World,
		UTextureRenderTarget2D* Target,
		UMaterialInterface* Material,
		UTextureRenderTarget2D* ReferenceTarget,
		UMyNinjaLiveComponent* Component,
		int64 SampleId)
	{
		Material->EnsureIsComplete();
		World->FlushDeferredParameterCollectionInstanceUpdates();

		FTextureRenderTargetResource* TargetResource = Target->GameThread_GetRenderTargetResource();
		FTextureRenderTargetResource* ReferenceResource = ReferenceTarget
			? ReferenceTarget->GameThread_GetRenderTargetResource()
			: nullptr;
		const FMaterialRenderProxy* MaterialRenderProxy = Material->GetRenderProxy();
		const FIntPoint Extent(Target->SizeX, Target->SizeY);
		const FGameTime Time = World->GetTime();
		const ERHIFeatureLevel::Type FeatureLevel = World->GetFeatureLevel();
		const float Tolerance = MyGetOutputTolerance(Target);
		const TWeakObjectPtr<UMyNinjaLiveComponent> WeakComponent(Component);

		ENQUEUE_RENDER_COMMAND(MyNinjaDrawOutputRDG)(
			[TargetResource, ReferenceResource, MaterialRenderProxy, Extent, Time, FeatureLevel,
				Tolerance, WeakComponent, SampleId](FRHICommandListImmediate& RHICmdList)
			{
				MyProcessCompletedOutputDiffs();
				TargetResource->FlushDeferredResourceUpdate(RHICmdList);
				if (ReferenceResource)
				{
					ReferenceResource->FlushDeferredResourceUpdate(RHICmdList);
				}

				FRDGBuilder GraphBuilder(RHICmdList);
				{
					RDG_EVENT_SCOPE(GraphBuilder, "FluidTest.NinjaLive.OutputPipeline");
					FRDGTextureRef CandidateTexture = MyAddOutputMaterialPass(
						GraphBuilder,
						TargetResource,
						MaterialRenderProxy,
						Extent,
						Time,
						FeatureLevel,
						TEXT("FluidTest.NinjaLive.RDGOutput"));

					if (ReferenceResource)
					{
						FRDGTextureRef ReferenceTexture = RegisterExternalTexture(
							GraphBuilder,
							ReferenceResource->GetRenderTargetTexture(),
							TEXT("FluidTest.NinjaLive.LegacyOutput"));
						GraphBuilder.SetTextureAccessFinal(ReferenceTexture, ERHIAccess::SRVMask);
						MyAddOutputDiffPass(
							GraphBuilder,
							ReferenceTexture,
							CandidateTexture,
							Extent,
							Tolerance,
							FeatureLevel,
							WeakComponent,
							SampleId);
					}
				}
				GraphBuilder.Execute();
			});

		Target->UpdateResourceImmediate(false);
	}
}

bool FMyNinjaFluidRenderPipeline::MyUseRDGOutput()
{
	return CVarMyNinjaOutputRenderPath.GetValueOnGameThread() != 0;
}

bool FMyNinjaFluidRenderPipeline::MyIsOutputValidationEnabled()
{
	return CVarMyNinjaOutputRDGValidation.GetValueOnGameThread() != 0;
}

bool FMyNinjaFluidRenderPipeline::MyShouldValidateOutput(uint64 FrameIndex)
{
	if (!MyIsOutputValidationEnabled())
	{
		return false;
	}
	const uint64 Interval = static_cast<uint64>(
		FMath::Max(CVarMyNinjaOutputRDGValidationInterval.GetValueOnGameThread(), 1));
	return FrameIndex % Interval == 0;
}

void FMyNinjaFluidRenderPipeline::MyPollOutputDiffs()
{
	if (GMyNinjaPendingOutputDiffCount.load(std::memory_order_acquire) == 0 ||
		GMyNinjaOutputDiffPollQueued.exchange(true, std::memory_order_acq_rel))
	{
		return;
	}

	ENQUEUE_RENDER_COMMAND(MyNinjaPollOutputDiffs)(
		[](FRHICommandListImmediate& RHICmdList)
		{
			MyProcessCompletedOutputDiffs();
			GMyNinjaOutputDiffPollQueued.store(false, std::memory_order_release);
		});
}

void FMyNinjaFluidRenderPipeline::MyShutdown()
{
	GMyNinjaOutputDiffShuttingDown.store(true, std::memory_order_release);
	if (!GIsRHIInitialized)
	{
		return;
	}

	ENQUEUE_RENDER_COMMAND(MyNinjaShutdownOutputDiffs)(
		[](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.SubmitAndBlockUntilGPUIdle();
			GMyNinjaPendingOutputDiffs.Reset();
			GMyNinjaPendingOutputDiffCount.store(0, std::memory_order_release);
			GMyNinjaOutputDiffPollQueued.store(false, std::memory_order_release);
		});
	FlushRenderingCommands();
}

void FMyNinjaFluidRenderPipeline::MyDrawOutput(
	UObject* WorldContextObject,
	UTextureRenderTarget2D* OutputTarget,
	UMaterialInterface* OutputMaterial,
	UTextureRenderTarget2D* ComparisonTarget,
	UMyNinjaLiveComponent* Component,
	uint64 SampleId)
{
	if (!FApp::CanEverRender() || !IsValid(WorldContextObject) || !IsValid(OutputTarget) ||
		!IsValid(OutputMaterial) || !OutputTarget->GetResource())
	{
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return;
	}

	const bool bUseRDG = MyUseRDGOutput();
	const bool bValidate = IsValid(ComparisonTarget) && ComparisonTarget->GetResource();
	if (!bValidate)
	{
		if (bUseRDG)
		{
			MyDrawOutputRDG(World, OutputTarget, OutputMaterial, nullptr, Component, static_cast<int64>(SampleId));
		}
		else
		{
			UKismetRenderingLibrary::DrawMaterialToRenderTarget(WorldContextObject, OutputTarget, OutputMaterial);
		}
		return;
	}

	if (bUseRDG)
	{
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(WorldContextObject, ComparisonTarget, OutputMaterial);
		MyDrawOutputRDG(World, OutputTarget, OutputMaterial, ComparisonTarget, Component, static_cast<int64>(SampleId));
	}
	else
	{
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(WorldContextObject, OutputTarget, OutputMaterial);
		MyDrawOutputRDG(World, ComparisonTarget, OutputMaterial, OutputTarget, Component, static_cast<int64>(SampleId));
	}
}

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

	TAutoConsoleVariable<int32> CVarMyNinjaCoreRenderPath(
		TEXT("FluidTest.NinjaLive.CoreRenderPath"),
		0,
		TEXT("0 uses legacy core draws. 1 uses RDG for migrated core draws."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarMyNinjaCoreRDGValidation(
		TEXT("FluidTest.NinjaLive.CoreRDGValidation"),
		0,
		TEXT("Enables asynchronous GPU comparison for migrated core draws."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarMyNinjaCoreRDGValidationInterval(
		TEXT("FluidTest.NinjaLive.CoreRDGValidationInterval"),
		30,
		TEXT("Number of core frames between RDG validation samples."),
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

	BEGIN_SHADER_PARAMETER_STRUCT(FMyNinjaTextureAccessParameters, )
		RDG_TEXTURE_ACCESS(Texture, ERHIAccess::SRVGraphics)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	struct FMyNinjaPendingOutputDiff
	{
		TSharedPtr<FRHIGPUBufferReadback, ESPMode::ThreadSafe> Readback;
		TWeakObjectPtr<UMyNinjaLiveComponent> Component;
		int64 SampleId = 0;
		int32 ComparedPixelCount = 0;
		float Tolerance = 0.0f;
		EMyNinjaRDGDiffTarget DiffTarget = EMyNinjaRDGDiffTarget::Output;
	};

	TArray<FMyNinjaPendingOutputDiff> GMyNinjaPendingOutputDiffs;
	std::atomic<int32> GMyNinjaPendingOutputDiffCount = 0;
	std::atomic_bool GMyNinjaOutputDiffPollQueued = false;
	std::atomic_bool GMyNinjaOutputDiffShuttingDown = false;
	constexpr int32 GMyNinjaMaxPendingOutputDiffs = 4;

	float MyGetTextureTolerance(const UTextureRenderTarget2D* Target)
	{
		if (Target->RenderTargetFormat == RTF_RGBA8)
		{
			return 0.0f;
		}
		return Target->RenderTargetFormat == RTF_RGBA32f || Target->RenderTargetFormat == RTF_RG32f
			? 0.000001f
			: 0.001f;
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
			const EMyNinjaRDGDiffTarget DiffTarget = Pending.DiffTarget;
			if (!GMyNinjaOutputDiffShuttingDown.load(std::memory_order_acquire))
			{
				AsyncTask(ENamedThreads::GameThread,
					[Component, SampleId, MaxDifference, ExceededPixelCount, ComparedPixelCount, Tolerance,
						DiffTarget]()
					{
						if (UMyNinjaLiveComponent* ValidComponent = Component.Get())
						{
							if (DiffTarget == EMyNinjaRDGDiffTarget::Output)
							{
								ValidComponent->MyApplyRDGOutputDiffResult(
									SampleId, MaxDifference, ExceededPixelCount, ComparedPixelCount, Tolerance);
							}
							else
							{
								ValidComponent->MyApplyRDGCoreDiffResult(
									DiffTarget, SampleId, MaxDifference, ExceededPixelCount,
									ComparedPixelCount, Tolerance);
							}
						}
					});
			}

			GMyNinjaPendingOutputDiffs.RemoveAtSwap(Index, 1, EAllowShrinking::No);
		}
		GMyNinjaPendingOutputDiffCount.store(GMyNinjaPendingOutputDiffs.Num(), std::memory_order_release);
	}

	FRDGTextureRef MyAddMaterialPass(
		FRDGBuilder& GraphBuilder,
		FTextureRenderTargetResource* TargetResource,
		const FMaterialRenderProxy* MaterialRenderProxy,
		FIntPoint Extent,
		const FGameTime& Time,
		ERHIFeatureLevel::Type FeatureLevel,
		const TCHAR* TextureName,
		const TCHAR* PassName)
	{
		RDG_EVENT_SCOPE(GraphBuilder, "%s", PassName);
		FRDGTextureRef TargetTexture = RegisterExternalTexture(
			GraphBuilder, TargetResource->GetRenderTargetTexture(), TextureName);
		FCanvas& Canvas = *FCanvas::Create(GraphBuilder, TargetTexture, nullptr, Time, FeatureLevel);
		Canvas.SetRenderTargetRect(FIntRect(FIntPoint::ZeroValue, Extent));
		FCanvasTileItem TileItem(FVector2D::ZeroVector, MaterialRenderProxy, FVector2D(Extent));
		TileItem.SetColor(FLinearColor::White);
		Canvas.DrawItem(TileItem);
		Canvas.Flush_RenderThread(GraphBuilder, false);
		return TargetTexture;
	}

	void MyAddTextureReadBarrier(
		FRDGBuilder& GraphBuilder,
		FRDGTextureRef Texture,
		FRDGTextureRef NextRenderTarget)
	{
		FMyNinjaTextureAccessParameters* PassParameters =
			GraphBuilder.AllocParameters<FMyNinjaTextureAccessParameters>();
		PassParameters->Texture = Texture;
		PassParameters->RenderTargets[0] =
			FRenderTargetBinding(NextRenderTarget, ERenderTargetLoadAction::ELoad);
		GraphBuilder.AddPass(
			RDG_EVENT_NAME("FluidTest.NinjaLive.TextureReadBarrier"),
			PassParameters,
			ERDGPassFlags::Raster | ERDGPassFlags::NeverCull,
			[](FRHICommandList& RHICmdList)
			{
			});
	}

	void MyAddTextureDiffPass(
		FRDGBuilder& GraphBuilder,
		FRDGTextureRef ReferenceTexture,
		FRDGTextureRef CandidateTexture,
		FIntPoint Extent,
		float Tolerance,
		ERHIFeatureLevel::Type FeatureLevel,
		TWeakObjectPtr<UMyNinjaLiveComponent> Component,
		int64 SampleId,
		EMyNinjaRDGDiffTarget DiffTarget)
	{
		MyProcessCompletedOutputDiffs();
		if (GMyNinjaPendingOutputDiffs.Num() >= GMyNinjaMaxPendingOutputDiffs)
		{
			return;
		}

		FRDGBufferDesc ResultDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), 5);
		ResultDesc.Usage = EBufferUsageFlags(ResultDesc.Usage | BUF_SourceCopy);
		FRDGBufferRef ResultBuffer = GraphBuilder.CreateBuffer(ResultDesc, TEXT("FluidTest.NinjaLive.TextureDiffResult"));
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
			RDG_EVENT_NAME("FluidTest.NinjaLive.TextureDiff.%d", static_cast<int32>(DiffTarget)),
			ComputeShader,
			PassParameters,
			FIntVector(FMath::DivideAndRoundUp(Extent.X, 8), FMath::DivideAndRoundUp(Extent.Y, 8), 1));

		TSharedPtr<FRHIGPUBufferReadback, ESPMode::ThreadSafe> Readback =
			MakeShared<FRHIGPUBufferReadback, ESPMode::ThreadSafe>(TEXT("FluidTest.NinjaLive.TextureDiffReadback"));
		AddEnqueueCopyPass(GraphBuilder, Readback.Get(), ResultBuffer, sizeof(uint32) * 5);
		GMyNinjaPendingOutputDiffs.Add(
			{ Readback, Component, SampleId, Extent.X * Extent.Y, Tolerance, DiffTarget });
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
		const float Tolerance = MyGetTextureTolerance(Target);
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
					FRDGTextureRef CandidateTexture = MyAddMaterialPass(
						GraphBuilder,
						TargetResource,
						MaterialRenderProxy,
						Extent,
						Time,
						FeatureLevel,
						TEXT("FluidTest.NinjaLive.RDGOutput"),
						TEXT("Output"));

					if (ReferenceResource)
					{
						FRDGTextureRef ReferenceTexture = RegisterExternalTexture(
							GraphBuilder,
							ReferenceResource->GetRenderTargetTexture(),
							TEXT("FluidTest.NinjaLive.LegacyOutput"));
						MyAddTextureDiffPass(
							GraphBuilder,
							ReferenceTexture,
							CandidateTexture,
							Extent,
							Tolerance,
							FeatureLevel,
							WeakComponent,
							SampleId,
							EMyNinjaRDGDiffTarget::Output);
						GraphBuilder.SetTextureAccessFinal(ReferenceTexture, ERHIAccess::SRVMask);
					}
					GraphBuilder.SetTextureAccessFinal(CandidateTexture, ERHIAccess::SRVMask);
				}
				GraphBuilder.Execute();
			});

		Target->UpdateResourceImmediate(false);
	}

	void MyDrawAdvectionDivergenceRDG(
		UWorld* World,
		UTextureRenderTarget2D* AdvectionTarget,
		UMaterialInterface* AdvectionMaterial,
		UTextureRenderTarget2D* DivergenceTarget,
		UMaterialInterface* DivergenceMaterial,
		UTextureRenderTarget2D* ReferenceAdvectionTarget,
		UTextureRenderTarget2D* ReferenceDivergenceTarget,
		UMyNinjaLiveComponent* Component,
		int64 SampleId)
	{
		AdvectionMaterial->EnsureIsComplete();
		DivergenceMaterial->EnsureIsComplete();
		World->FlushDeferredParameterCollectionInstanceUpdates();

		FTextureRenderTargetResource* AdvectionResource =
			AdvectionTarget->GameThread_GetRenderTargetResource();
		FTextureRenderTargetResource* DivergenceResource =
			DivergenceTarget->GameThread_GetRenderTargetResource();
		FTextureRenderTargetResource* ReferenceAdvectionResource = ReferenceAdvectionTarget
			? ReferenceAdvectionTarget->GameThread_GetRenderTargetResource()
			: nullptr;
		FTextureRenderTargetResource* ReferenceDivergenceResource = ReferenceDivergenceTarget
			? ReferenceDivergenceTarget->GameThread_GetRenderTargetResource()
			: nullptr;
		const FMaterialRenderProxy* AdvectionProxy = AdvectionMaterial->GetRenderProxy();
		const FMaterialRenderProxy* DivergenceProxy = DivergenceMaterial->GetRenderProxy();
		const FIntPoint AdvectionExtent(AdvectionTarget->SizeX, AdvectionTarget->SizeY);
		const FIntPoint DivergenceExtent(DivergenceTarget->SizeX, DivergenceTarget->SizeY);
		const FGameTime Time = World->GetTime();
		const ERHIFeatureLevel::Type FeatureLevel = World->GetFeatureLevel();
		const float AdvectionTolerance = MyGetTextureTolerance(AdvectionTarget);
		const float DivergenceTolerance = MyGetTextureTolerance(DivergenceTarget);
		const TWeakObjectPtr<UMyNinjaLiveComponent> WeakComponent(Component);

		ENQUEUE_RENDER_COMMAND(MyNinjaDrawAdvectionDivergenceRDG)(
			[AdvectionResource, DivergenceResource, ReferenceAdvectionResource,
				ReferenceDivergenceResource, AdvectionProxy, DivergenceProxy, AdvectionExtent,
				DivergenceExtent, Time, FeatureLevel, AdvectionTolerance, DivergenceTolerance,
				WeakComponent, SampleId](FRHICommandListImmediate& RHICmdList)
			{
				MyProcessCompletedOutputDiffs();
				AdvectionResource->FlushDeferredResourceUpdate(RHICmdList);
				DivergenceResource->FlushDeferredResourceUpdate(RHICmdList);
				if (ReferenceAdvectionResource)
				{
					ReferenceAdvectionResource->FlushDeferredResourceUpdate(RHICmdList);
				}
				if (ReferenceDivergenceResource)
				{
					ReferenceDivergenceResource->FlushDeferredResourceUpdate(RHICmdList);
				}

				FRDGBuilder GraphBuilder(RHICmdList);
				{
					RDG_EVENT_SCOPE(GraphBuilder, "FluidTest.NinjaLive.AdvectionDivergencePipeline");
					FRDGTextureRef AdvectionTexture = MyAddMaterialPass(
						GraphBuilder,
						AdvectionResource,
						AdvectionProxy,
						AdvectionExtent,
						Time,
						FeatureLevel,
						TEXT("FluidTest.NinjaLive.RDGAdvection"),
						TEXT("Advection"));
					FRDGTextureRef DivergenceTexture = RegisterExternalTexture(
						GraphBuilder,
						DivergenceResource->GetRenderTargetTexture(),
						TEXT("FluidTest.NinjaLive.RDGDivergence"));
					MyAddTextureReadBarrier(GraphBuilder, AdvectionTexture, DivergenceTexture);

					DivergenceTexture = MyAddMaterialPass(
						GraphBuilder,
						DivergenceResource,
						DivergenceProxy,
						DivergenceExtent,
						Time,
						FeatureLevel,
						TEXT("FluidTest.NinjaLive.RDGDivergence"),
						TEXT("Divergence"));

					if (ReferenceAdvectionResource && ReferenceDivergenceResource)
					{
						FRDGTextureRef ReferenceAdvectionTexture = RegisterExternalTexture(
							GraphBuilder,
							ReferenceAdvectionResource->GetRenderTargetTexture(),
							TEXT("FluidTest.NinjaLive.ReferenceAdvection"));
						FRDGTextureRef ReferenceDivergenceTexture = RegisterExternalTexture(
							GraphBuilder,
							ReferenceDivergenceResource->GetRenderTargetTexture(),
							TEXT("FluidTest.NinjaLive.ReferenceDivergence"));
						MyAddTextureDiffPass(
							GraphBuilder,
							ReferenceAdvectionTexture,
							AdvectionTexture,
							AdvectionExtent,
							AdvectionTolerance,
							FeatureLevel,
							WeakComponent,
							SampleId,
							EMyNinjaRDGDiffTarget::Advection);
						MyAddTextureDiffPass(
							GraphBuilder,
							ReferenceDivergenceTexture,
							DivergenceTexture,
							DivergenceExtent,
							DivergenceTolerance,
							FeatureLevel,
							WeakComponent,
							SampleId,
							EMyNinjaRDGDiffTarget::Divergence);
						GraphBuilder.SetTextureAccessFinal(
							ReferenceAdvectionTexture, ERHIAccess::SRVMask);
						GraphBuilder.SetTextureAccessFinal(
							ReferenceDivergenceTexture, ERHIAccess::SRVMask);
					}
					GraphBuilder.SetTextureAccessFinal(AdvectionTexture, ERHIAccess::SRVMask);
					GraphBuilder.SetTextureAccessFinal(DivergenceTexture, ERHIAccess::SRVMask);
				}
				GraphBuilder.Execute();
			});

		AdvectionTarget->UpdateResourceImmediate(false);
		DivergenceTarget->UpdateResourceImmediate(false);
	}

	void MyCompareAdvectionDivergenceRDG(
		UWorld* World,
		UTextureRenderTarget2D* ReferenceAdvectionTarget,
		UTextureRenderTarget2D* CandidateAdvectionTarget,
		UTextureRenderTarget2D* ReferenceDivergenceTarget,
		UTextureRenderTarget2D* CandidateDivergenceTarget,
		UMyNinjaLiveComponent* Component,
		int64 SampleId)
	{
		FTextureRenderTargetResource* ReferenceAdvectionResource =
			ReferenceAdvectionTarget->GameThread_GetRenderTargetResource();
		FTextureRenderTargetResource* CandidateAdvectionResource =
			CandidateAdvectionTarget->GameThread_GetRenderTargetResource();
		FTextureRenderTargetResource* ReferenceDivergenceResource =
			ReferenceDivergenceTarget->GameThread_GetRenderTargetResource();
		FTextureRenderTargetResource* CandidateDivergenceResource =
			CandidateDivergenceTarget->GameThread_GetRenderTargetResource();
		const FIntPoint AdvectionExtent(CandidateAdvectionTarget->SizeX, CandidateAdvectionTarget->SizeY);
		const FIntPoint DivergenceExtent(CandidateDivergenceTarget->SizeX, CandidateDivergenceTarget->SizeY);
		const ERHIFeatureLevel::Type FeatureLevel = World->GetFeatureLevel();
		const float AdvectionTolerance = MyGetTextureTolerance(CandidateAdvectionTarget);
		const float DivergenceTolerance = MyGetTextureTolerance(CandidateDivergenceTarget);
		const TWeakObjectPtr<UMyNinjaLiveComponent> WeakComponent(Component);

		ENQUEUE_RENDER_COMMAND(MyNinjaCompareAdvectionDivergenceRDG)(
			[ReferenceAdvectionResource, CandidateAdvectionResource, ReferenceDivergenceResource,
				CandidateDivergenceResource, AdvectionExtent, DivergenceExtent, FeatureLevel,
				AdvectionTolerance, DivergenceTolerance, WeakComponent,
				SampleId](FRHICommandListImmediate& RHICmdList)
			{
				MyProcessCompletedOutputDiffs();
				ReferenceAdvectionResource->FlushDeferredResourceUpdate(RHICmdList);
				CandidateAdvectionResource->FlushDeferredResourceUpdate(RHICmdList);
				ReferenceDivergenceResource->FlushDeferredResourceUpdate(RHICmdList);
				CandidateDivergenceResource->FlushDeferredResourceUpdate(RHICmdList);

				FRDGBuilder GraphBuilder(RHICmdList);
				{
					RDG_EVENT_SCOPE(GraphBuilder, "FluidTest.NinjaLive.AdvectionDivergenceValidation");
					FRDGTextureRef ReferenceAdvectionTexture = RegisterExternalTexture(
						GraphBuilder,
						ReferenceAdvectionResource->GetRenderTargetTexture(),
						TEXT("FluidTest.NinjaLive.ReferenceAdvection"));
					FRDGTextureRef CandidateAdvectionTexture = RegisterExternalTexture(
						GraphBuilder,
						CandidateAdvectionResource->GetRenderTargetTexture(),
						TEXT("FluidTest.NinjaLive.CandidateAdvection"));
					FRDGTextureRef ReferenceDivergenceTexture = RegisterExternalTexture(
						GraphBuilder,
						ReferenceDivergenceResource->GetRenderTargetTexture(),
						TEXT("FluidTest.NinjaLive.ReferenceDivergence"));
					FRDGTextureRef CandidateDivergenceTexture = RegisterExternalTexture(
						GraphBuilder,
						CandidateDivergenceResource->GetRenderTargetTexture(),
						TEXT("FluidTest.NinjaLive.CandidateDivergence"));
					MyAddTextureDiffPass(
						GraphBuilder,
						ReferenceAdvectionTexture,
						CandidateAdvectionTexture,
						AdvectionExtent,
						AdvectionTolerance,
						FeatureLevel,
						WeakComponent,
						SampleId,
						EMyNinjaRDGDiffTarget::Advection);
					MyAddTextureDiffPass(
						GraphBuilder,
						ReferenceDivergenceTexture,
						CandidateDivergenceTexture,
						DivergenceExtent,
						DivergenceTolerance,
						FeatureLevel,
						WeakComponent,
						SampleId,
						EMyNinjaRDGDiffTarget::Divergence);
					GraphBuilder.SetTextureAccessFinal(ReferenceAdvectionTexture, ERHIAccess::SRVMask);
					GraphBuilder.SetTextureAccessFinal(CandidateAdvectionTexture, ERHIAccess::SRVMask);
					GraphBuilder.SetTextureAccessFinal(ReferenceDivergenceTexture, ERHIAccess::SRVMask);
					GraphBuilder.SetTextureAccessFinal(CandidateDivergenceTexture, ERHIAccess::SRVMask);
				}
				GraphBuilder.Execute();
			});
	}

	void MyCopyAdvectionDivergenceTargets(
		UTextureRenderTarget2D* SourceAdvectionTarget,
		UTextureRenderTarget2D* DestinationAdvectionTarget,
		UTextureRenderTarget2D* SourceDivergenceTarget,
		UTextureRenderTarget2D* DestinationDivergenceTarget)
	{
		FTextureRenderTargetResource* SourceAdvectionResource =
			SourceAdvectionTarget->GameThread_GetRenderTargetResource();
		FTextureRenderTargetResource* DestinationAdvectionResource =
			DestinationAdvectionTarget->GameThread_GetRenderTargetResource();
		FTextureRenderTargetResource* SourceDivergenceResource =
			SourceDivergenceTarget->GameThread_GetRenderTargetResource();
		FTextureRenderTargetResource* DestinationDivergenceResource =
			DestinationDivergenceTarget->GameThread_GetRenderTargetResource();

		ENQUEUE_RENDER_COMMAND(MyNinjaCopyAdvectionDivergenceTargets)(
			[SourceAdvectionResource, DestinationAdvectionResource, SourceDivergenceResource,
				DestinationDivergenceResource](FRHICommandListImmediate& RHICmdList)
			{
				SourceAdvectionResource->FlushDeferredResourceUpdate(RHICmdList);
				DestinationAdvectionResource->FlushDeferredResourceUpdate(RHICmdList);
				SourceDivergenceResource->FlushDeferredResourceUpdate(RHICmdList);
				DestinationDivergenceResource->FlushDeferredResourceUpdate(RHICmdList);

				FRDGBuilder GraphBuilder(RHICmdList);
				FRDGTextureRef SourceAdvectionTexture = RegisterExternalTexture(
					GraphBuilder,
					SourceAdvectionResource->GetRenderTargetTexture(),
					TEXT("FluidTest.NinjaLive.SourceAdvectionSnapshot"));
				FRDGTextureRef DestinationAdvectionTexture = RegisterExternalTexture(
					GraphBuilder,
					DestinationAdvectionResource->GetRenderTargetTexture(),
					TEXT("FluidTest.NinjaLive.DestinationAdvectionSnapshot"));
				FRDGTextureRef SourceDivergenceTexture = RegisterExternalTexture(
					GraphBuilder,
					SourceDivergenceResource->GetRenderTargetTexture(),
					TEXT("FluidTest.NinjaLive.SourceDivergenceSnapshot"));
				FRDGTextureRef DestinationDivergenceTexture = RegisterExternalTexture(
					GraphBuilder,
					DestinationDivergenceResource->GetRenderTargetTexture(),
					TEXT("FluidTest.NinjaLive.DestinationDivergenceSnapshot"));
				AddCopyTexturePass(GraphBuilder, SourceAdvectionTexture, DestinationAdvectionTexture);
				AddCopyTexturePass(GraphBuilder, SourceDivergenceTexture, DestinationDivergenceTexture);
				GraphBuilder.SetTextureAccessFinal(SourceAdvectionTexture, ERHIAccess::SRVMask);
				GraphBuilder.SetTextureAccessFinal(DestinationAdvectionTexture, ERHIAccess::SRVMask);
				GraphBuilder.SetTextureAccessFinal(SourceDivergenceTexture, ERHIAccess::SRVMask);
				GraphBuilder.SetTextureAccessFinal(DestinationDivergenceTexture, ERHIAccess::SRVMask);
				GraphBuilder.Execute();
			});
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

bool FMyNinjaFluidRenderPipeline::MyUseRDGCore()
{
	return CVarMyNinjaCoreRenderPath.GetValueOnGameThread() != 0;
}

bool FMyNinjaFluidRenderPipeline::MyIsCoreValidationEnabled()
{
	return CVarMyNinjaCoreRDGValidation.GetValueOnGameThread() != 0;
}

bool FMyNinjaFluidRenderPipeline::MyShouldValidateCore(uint64 FrameIndex)
{
	if (!MyIsCoreValidationEnabled())
	{
		return false;
	}
	const uint64 Interval = static_cast<uint64>(
		FMath::Max(CVarMyNinjaCoreRDGValidationInterval.GetValueOnGameThread(), 1));
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

void FMyNinjaFluidRenderPipeline::MyDrawAdvectionDivergence(
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
	uint64 SampleId)
{
	if (!FApp::CanEverRender() || !IsValid(WorldContextObject) || !IsValid(AdvectionTarget) ||
		!IsValid(AdvectionMaterial) || !IsValid(DivergenceTarget) || !IsValid(DivergenceMaterial) ||
		!AdvectionTarget->GetResource() || !DivergenceTarget->GetResource())
	{
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return;
	}

	const bool bUseRDG = MyUseRDGCore();
	const bool bValidate =
		IsValid(ComparisonAdvectionTarget) && ComparisonAdvectionTarget->GetResource() &&
		IsValid(ComparisonAdvectionMaterial) && IsValid(ComparisonDivergenceTarget) &&
		ComparisonDivergenceTarget->GetResource() && IsValid(ComparisonDivergenceMaterial);
	if (!bValidate)
	{
		if (bUseRDG)
		{
			MyDrawAdvectionDivergenceRDG(
				World,
				AdvectionTarget,
				AdvectionMaterial,
				DivergenceTarget,
				DivergenceMaterial,
				nullptr,
				nullptr,
				Component,
				static_cast<int64>(SampleId));
		}
		else
		{
			UKismetRenderingLibrary::DrawMaterialToRenderTarget(
				WorldContextObject, AdvectionTarget, AdvectionMaterial);
			UKismetRenderingLibrary::DrawMaterialToRenderTarget(
				WorldContextObject, DivergenceTarget, DivergenceMaterial);
		}
		return;
	}

	MyCopyAdvectionDivergenceTargets(
		AdvectionTarget,
		ComparisonAdvectionTarget,
		DivergenceTarget,
		ComparisonDivergenceTarget);

	if (bUseRDG)
	{
		MyDrawAdvectionDivergenceRDG(
			World,
			AdvectionTarget,
			AdvectionMaterial,
			DivergenceTarget,
			DivergenceMaterial,
			nullptr,
			nullptr,
			Component,
			static_cast<int64>(SampleId));
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(
			WorldContextObject, ComparisonAdvectionTarget, ComparisonAdvectionMaterial);
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(
			WorldContextObject, ComparisonDivergenceTarget, ComparisonDivergenceMaterial);
		MyCompareAdvectionDivergenceRDG(
			World,
			ComparisonAdvectionTarget,
			AdvectionTarget,
			ComparisonDivergenceTarget,
			DivergenceTarget,
			Component,
			static_cast<int64>(SampleId));
	}
	else
	{
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(
			WorldContextObject, AdvectionTarget, AdvectionMaterial);
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(
			WorldContextObject, DivergenceTarget, DivergenceMaterial);
		MyDrawAdvectionDivergenceRDG(
			World,
			ComparisonAdvectionTarget,
			ComparisonAdvectionMaterial,
			ComparisonDivergenceTarget,
			ComparisonDivergenceMaterial,
			AdvectionTarget,
			DivergenceTarget,
			Component,
			static_cast<int64>(SampleId));
	}
}

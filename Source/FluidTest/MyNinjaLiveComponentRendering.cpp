

#include "MyNinjaLiveComponent.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Components/VolumetricCloudComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "FileMediaSource.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Misc/EngineVersion.h"
#include "MyNinjaLiveActor.h"
#include "FluidTest/MyNinjaFluidRenderPipeline.h"
#include "FluidTest/MyNinjaLiveFunctions.h"
#include "MyNinjaLiveMemoryPoolManager.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "TimerManager.h"

void UMyNinjaLiveComponent::MySetScalarParameterByCachedIndex(UMaterialInstanceDynamic* Material,
	TMap<FName, int32>& ParameterIndices, FName ParameterName, float Value)
{
	if (!IsValid(Material))
	{
		return;
	}

	if (const int32* ParameterIndex = ParameterIndices.Find(ParameterName))
	{
		Material->SetScalarParameterByIndex(*ParameterIndex, Value);
		return;
	}

	int32 ParameterIndex = INDEX_NONE;
	if (Material->InitializeScalarParameterAndGetIndex(ParameterName, Value, ParameterIndex))
	{
		ParameterIndices.Add(ParameterName, ParameterIndex);
	}
}

void UMyNinjaLiveComponent::MySetAdditionalFluidsimParams()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FluidSim_MySetAdditionalFluidsimParams);

	double VelocityX = 0.0;
	double VelocityY = 0.0;
	double VelocityZ = 0.0;
	MyVelocityHandlerForSimArea(-0.01, VelocityX, VelocityY, VelocityZ);

	if (IsValid(MyMICompositeAndGradient))
	{
		auto SetCompositeScalar = [this](FName ParameterName, double Value)
		{
			MySetScalarParameterByCachedIndex(MyMICompositeAndGradient,
				MyCompositeScalarParameterIndices, ParameterName, static_cast<float>(Value));
		};

		SetCompositeScalar(TEXT("VeloFromBrushMotion"), MyVeloFromBrushMotion);
		SetCompositeScalar(TEXT("VeloStrength"), MyVeloStrength);
		SetCompositeScalar(TEXT("VeloRotate"), MyVeloRotate);
		SetCompositeScalar(TEXT("VeloOffsetX"), MyVeloOffsetX + VelocityX);
		SetCompositeScalar(TEXT("VeloOffsetY"), MyVeloOffsetY + VelocityY);
		SetCompositeScalar(TEXT("VeloAmpNoise"), MyVeloAmpNoise);
		SetCompositeScalar(TEXT("VeloDirNoise"), MyVeloDirNoise);
		SetCompositeScalar(TEXT("SimEdgeBouncyness"),
			FMath::Max(MySimEdgeBouncyness, MyCollisionMaskIsNonDefault ? 1.0 : 0.0));
		SetCompositeScalar(TEXT("VeloDirNoiseSize"), MyVeloDirNoiseSize);
		SetCompositeScalar(TEXT("VeloDirNoiseSpeed"), MyVeloDirNoiseSpeed);
		SetCompositeScalar(TEXT("EdgeMaskWidth"), MyEdgeMaskWidth);
		SetCompositeScalar(TEXT("DensityTxtOffsetX"), MyDensityTxtOffsetX);
		SetCompositeScalar(TEXT("DensityTxtScale"), MyDensityTxtScale);
		SetCompositeScalar(TEXT("DensityTxtOffsetY"), MyDensityTxtOffsetY);
		SetCompositeScalar(TEXT("VeloInputTile"), MyVeloInputTile);
		SetCompositeScalar(TEXT("VeloInputOffsetSpeed"), MyVeloInputOffsetSpeed);
		SetCompositeScalar(TEXT("DensityNoiseSpeed"), MyDensityInputNoiseOffset);
		SetCompositeScalar(TEXT("DensityNoiseAmount"), MyDensityInputNoiseAmp);
		SetCompositeScalar(TEXT("DensityNoiseTile"), MyDensityInputNoiseTile);
		SetCompositeScalar(TEXT("DensityTxtMult"), MyDensityTxtMult);
		SetCompositeScalar(TEXT("FlowFeedback"), MyFlowFeedback);
		SetCompositeScalar(TEXT("FadeDensityAtSimEdge"), MyFadeDensityAtSimEdge);
	}

	if (IsValid(MyMIDivergence))
	{
		MySetScalarParameterByCachedIndex(MyMIDivergence, MyDivergenceScalarParameterIndices,
			TEXT("Divergence"), static_cast<float>(MyDivergence));
		MySetScalarParameterByCachedIndex(MyMIDivergence, MyDivergenceScalarParameterIndices,
			TEXT("BrushPuncture"), static_cast<float>(MyBrushPuncture + VelocityZ));
	}
}

void UMyNinjaLiveComponent::MyCoreFluidsimOPs(bool& ThenExec, bool& PainterV2Exec)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FluidSim_MyCoreFluidsimOPs);
	FMyNinjaFluidRenderPipeline::MyPollOutputDiffs();

	ThenExec = false;
	PainterV2Exec = true;
	const bool bOutputRequired =
		MyMake1stOutputAvailableFor2ndOutput || MyMake1stOutputAvailableForNiagara;
	const bool bValidateOutput = FMyNinjaFluidRenderPipeline::MyIsOutputValidationEnabled();
	const bool bValidateCore = FMyNinjaFluidRenderPipeline::MyIsCoreValidationEnabled();
	const bool bValidatePressure = FMyNinjaFluidRenderPipeline::MyIsPressureValidationEnabled();
	const bool bUseRDGPainter = FMyNinjaFluidRenderPipeline::MyUseRDGPainter();
	const bool bValidatePainter = FMyNinjaFluidRenderPipeline::MyIsPainterValidationEnabled();

	auto FindRenderTarget = [this](const TCHAR* Name) -> UTextureRenderTarget2D*
	{
		const TObjectPtr<UTextureRenderTarget2D>* Found = MyRenderTargetsMap.Find(Name);
		return Found ? Found->Get() : nullptr;
	};
	auto Draw = [this](UTextureRenderTarget2D* Target, UMaterialInterface* Material)
	{
		if (IsValid(Target) && IsValid(Material))
		{
			UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, Target, Material);
		}
	};
	UTextureRenderTarget2D* const CompositeTarget = FindRenderTarget(TEXT("RT_Composite"));
	UTextureRenderTarget2D* const AdvectionTarget = FindRenderTarget(TEXT("RT_Advection"));
	UTextureRenderTarget2D* const PainterTarget = FindRenderTarget(TEXT("RT_Painter"));
	UTextureRenderTarget2D* const PressureTarget = FindRenderTarget(TEXT("RT_PressureDivergence"));
	UTextureRenderTarget2D* const PressureTempTarget = FindRenderTarget(TEXT("RT_PressureDivergenceTemp"));
	UTextureRenderTarget2D* const DensityInputTarget = FindRenderTarget(TEXT("RT_DensityInputMaterial"));
	UTextureRenderTarget2D* OutputTarget = FindRenderTarget(TEXT("RT_Output"));
	if (!bValidateCore)
	{
		MyRDGAdvectionComparisonTarget = nullptr;
		MyRDGDivergenceComparisonTarget = nullptr;
	}
	if (!bValidatePressure)
	{
		MyRDGPressureComparisonTarget = nullptr;
		MyRDGPressureTempComparisonTarget = nullptr;
		MyMIPressureCycle1Comparison = nullptr;
		MyMIPressureCycle2Comparison = nullptr;
	}
	if (!bValidatePainter)
	{
		MyRDGPainterComparisonTarget = nullptr;
		MyRDGCompositeComparisonTarget = nullptr;
		MyMICollisionPainterOffsetComparisonFirstPass = nullptr;
		MyMICollisionPainterOffsetComparisonSecondPass = nullptr;
		MyMICompositeAndGradientComparison = nullptr;
		if (!bUseRDGPainter)
		{
			MyMICollisionPainterOffsetFirstPass = nullptr;
		}
	}
	if (bOutputRequired)
	{
		MyRDGOutputTargetCreatedForValidation = false;
	}
	else if (!bValidateOutput && MyRDGOutputTargetCreatedForValidation)
	{
		MyRenderTargetsMap.Remove(TEXT("RT_Output"));
		OutputTarget = nullptr;
		MyRDGOutputTargetCreatedForValidation = false;
	}
	if (!bValidateOutput)
	{
		MyRDGOutputComparisonTarget = nullptr;
	}
	else if (!IsValid(OutputTarget))
	{
		const int32 OutputMultiplier = MyForce2xResolutionOutputBuffer ? 2 : 1;
		const ETextureRenderTargetFormat OutputFormat = MyForce8bitOutputBuffer
			? RTF_RGBA8
			: (MySimPrecisionIndex == 0 ? RTF_RGBA16f : RTF_RGBA32f);
		OutputTarget = UMyNinjaLiveFunctions::MyCreateRenderTarget(
			this,
			FMath::Max(1, MyResolutionX) * OutputMultiplier,
			FMath::Max(1, MyResolutionY) * OutputMultiplier,
			OutputFormat,
			MySimAreaClamp,
			TEXTUREGROUP_RenderTarget,
			TF_Bilinear);
		if (IsValid(OutputTarget))
		{
			MyRenderTargetsMap.Add(TEXT("RT_Output"), OutputTarget);
			MyRDGOutputTargetCreatedForValidation = true;
		}
	}


	if (MyUseInputMaterials && MyInputMaterials.IsValidIndex(MyInputMaterialSelected))
	{
		Draw(DensityInputTarget, MyInputMaterials[MyInputMaterialSelected]);
	}

	MyWorldSpaceOffset.Broadcast(MyTraceMeshPos);
	const bool bUpdateTraceMeshPosition =
		MyQuantizerStepSize > 0 || MyMovementIsLockedOnThisAxis != EMyQuantizerAxisIgnore::None;
	if (bUpdateTraceMeshPosition)
	{
		if (IsValid(MyTraceMeshComponent))
		{
			MyTraceMeshComponent->SetWorldLocation(MyTraceMeshPos, false, nullptr, ETeleportType::TeleportPhysics);
		}
		if (MyInteractionVolumeIsPresent && IsValid(MyInteractionVolume))
		{
			MyInteractionVolume->SetWorldLocation(MyTraceMeshPos, false, nullptr, ETeleportType::TeleportPhysics);
		}

		const FLinearColor TracePositionColor(MyTraceMeshPos);
		auto SetTraceMeshPosition = [this, &TracePositionColor](UMaterialInstanceDynamic* Material)
		{
			if (!IsValid(Material))
			{
				return;
			}

			Material->SetVectorParameterValue(TEXT("TraceMeshPos"), TracePositionColor);
			if (MyLWCSupport)
			{
				Material->SetDoubleVectorParameterValue(
					TEXT("TraceMeshPosDouble"), FVector4(MyTraceMeshPos, 0.0));
			}
		};


		SetTraceMeshPosition(MyMIOutput);
		if (MySecondaryMaterialsPresent)
		{
			SetTraceMeshPosition(MyMISecondaryOutput);
		}
		if (MyTertiaryMaterialsPresent)
		{
			SetTraceMeshPosition(MyMITertiaryOutput);
		}
		if (MyMaterialCollectionPresent && IsValid(MySetInternalParamsToMaterialParamCollection))
		{
			UKismetMaterialLibrary::SetVectorParameterValue(
				this, MySetInternalParamsToMaterialParamCollection, TEXT("TraceMeshPos"), TracePositionColor);
		}

		if (MyNiagaraSystemsPresent)
		{
			for (UNiagaraComponent* NiagaraComponent : MyNiagaraSystemsToDrive)
			{
				if (!IsValid(NiagaraComponent))
				{
					continue;
				}

				if (MyLWCSupport)
				{
					NiagaraComponent->SetVariablePosition(TEXT("TraceMeshPosDouble"), MyTraceMeshPos);
				}
				else if (!MyLWCAvoidNiagaraWarnings)
				{
					NiagaraComponent->SetVectorParameter(TEXT("TraceMeshPos"), MyTraceMeshPos);
				}
			}
		}
	}

	const double KernelMultiplier = MyLODSteps > 0
		? MyPressureSolver2KernelReduction * static_cast<double>(MyLODLevel) / static_cast<double>(MyLODSteps)
		: 0.0;

	if (MyEnablePainterDoubleBuffering && IsValid(MyMICollisionPainterOffset))
	{
		if ((!bUseRDGPainter && !bValidatePainter) ||
			!IsValid(PainterTarget) || !IsValid(CompositeTarget) ||
			(!MySimplePainterMode && !IsValid(MyMICompositeAndGradient)))
		{
			MyMICollisionPainterOffset->SetTextureParameterValue(TEXT("Texture"), PainterTarget);
			Draw(CompositeTarget, MyMICollisionPainterOffset);
			MyMICollisionPainterOffset->SetScalarParameterValue(TEXT("WorldOffsetDeltaX"), 0.0f);
			MyMICollisionPainterOffset->SetScalarParameterValue(TEXT("WorldOffsetDeltaY"), 0.0f);
			MyMICollisionPainterOffset->SetTextureParameterValue(TEXT("Texture"), CompositeTarget);

			if (MySimplePainterMode)
			{
				for (UMaterialInstanceDynamic* PainterMaterial : { MyMICollisionPainterLine.Get(), MyMICollisionPainterDot.Get() })
				{
					if (IsValid(PainterMaterial))
					{
						PainterMaterial->SetScalarParameterValue(TEXT("VeloMult"), static_cast<float>(MyVeloFromBrushMotion));
					}
				}
				MyMICollisionPainterOffset->SetScalarParameterValue(TEXT("DensityTxtScale"), static_cast<float>(MyDensityTxtScale));
				MyMICollisionPainterOffset->SetScalarParameterValue(TEXT("DensityTxtMult"), static_cast<float>(MyDensityTxtMult));
			}

			Draw(PainterTarget, MyMICollisionPainterOffset);
			if (!MySimplePainterMode)
			{
				Draw(CompositeTarget, MyMICompositeAndGradient);
			}
		}
		else
		{
			auto EnsureComparisonTarget = [this](
				TObjectPtr<UTextureRenderTarget2D>& ComparisonTarget,
				UTextureRenderTarget2D* SourceTarget)
			{
				if (!IsValid(ComparisonTarget) ||
					ComparisonTarget->SizeX != SourceTarget->SizeX ||
					ComparisonTarget->SizeY != SourceTarget->SizeY ||
					ComparisonTarget->RenderTargetFormat != SourceTarget->RenderTargetFormat)
				{
					ComparisonTarget = UMyNinjaLiveFunctions::MyCreateRenderTarget(
						this,
						SourceTarget->SizeX,
						SourceTarget->SizeY,
						SourceTarget->RenderTargetFormat,
						MySimAreaClamp,
						SourceTarget->LODGroup,
						SourceTarget->Filter);
				}
			};
			auto EnsureComparisonMaterial = [this](
				TObjectPtr<UMaterialInstanceDynamic>& ComparisonMaterial,
				UMaterialInstanceDynamic* SourceMaterial)
			{
				UMaterialInterface* ParentMaterial = IsValid(SourceMaterial->Parent)
					? SourceMaterial->Parent.Get()
					: SourceMaterial;
				if (!IsValid(ComparisonMaterial) || ComparisonMaterial->Parent != ParentMaterial)
				{
					ComparisonMaterial = UMaterialInstanceDynamic::Create(ParentMaterial, this);
				}
			};

			EnsureComparisonMaterial(MyMICollisionPainterOffsetFirstPass, MyMICollisionPainterOffset);
			const uint64 PainterFrameIndex = MyRDGPainterFrameIndex++;
			bool bValidatePainterFrame =
				FMyNinjaFluidRenderPipeline::MyShouldValidatePainter(PainterFrameIndex) &&
				IsValid(PainterTarget) && IsValid(CompositeTarget) &&
				(MySimplePainterMode || IsValid(MyMICompositeAndGradient));
			if (bValidatePainterFrame)
			{
				EnsureComparisonTarget(MyRDGPainterComparisonTarget, PainterTarget);
				EnsureComparisonTarget(MyRDGCompositeComparisonTarget, CompositeTarget);
				EnsureComparisonMaterial(
					MyMICollisionPainterOffsetComparisonFirstPass, MyMICollisionPainterOffset);
				EnsureComparisonMaterial(
					MyMICollisionPainterOffsetComparisonSecondPass, MyMICollisionPainterOffset);
				if (!MySimplePainterMode)
				{
					EnsureComparisonMaterial(
						MyMICompositeAndGradientComparison, MyMICompositeAndGradient);
				}
				bValidatePainterFrame =
					IsValid(MyRDGPainterComparisonTarget) &&
					IsValid(MyRDGCompositeComparisonTarget) &&
					IsValid(MyMICollisionPainterOffsetComparisonFirstPass) &&
					IsValid(MyMICollisionPainterOffsetComparisonSecondPass) &&
					(MySimplePainterMode || IsValid(MyMICompositeAndGradientComparison));
				if (bValidatePainterFrame)
				{
					FMyNinjaFluidRenderPipeline::MyCopyPainterCompositeTargets(
						PainterTarget,
						MyRDGPainterComparisonTarget,
						CompositeTarget,
						MyRDGCompositeComparisonTarget);
				}
			}

			if (!IsValid(MyMICollisionPainterOffsetFirstPass))
			{
				MyMICollisionPainterOffset->SetTextureParameterValue(TEXT("Texture"), PainterTarget);
				Draw(CompositeTarget, MyMICollisionPainterOffset);
				MyMICollisionPainterOffset->SetScalarParameterValue(TEXT("WorldOffsetDeltaX"), 0.0f);
				MyMICollisionPainterOffset->SetScalarParameterValue(TEXT("WorldOffsetDeltaY"), 0.0f);
				MyMICollisionPainterOffset->SetTextureParameterValue(TEXT("Texture"), CompositeTarget);
				if (MySimplePainterMode)
				{
					for (UMaterialInstanceDynamic* PainterMaterial : { MyMICollisionPainterLine.Get(), MyMICollisionPainterDot.Get() })
					{
						if (IsValid(PainterMaterial))
						{
							PainterMaterial->SetScalarParameterValue(TEXT("VeloMult"), static_cast<float>(MyVeloFromBrushMotion));
						}
					}
					MyMICollisionPainterOffset->SetScalarParameterValue(TEXT("DensityTxtScale"), static_cast<float>(MyDensityTxtScale));
					MyMICollisionPainterOffset->SetScalarParameterValue(TEXT("DensityTxtMult"), static_cast<float>(MyDensityTxtMult));
				}
				Draw(PainterTarget, MyMICollisionPainterOffset);
				if (!MySimplePainterMode)
				{
					Draw(CompositeTarget, MyMICompositeAndGradient);
				}
			}
			else
			{
				MyMICollisionPainterOffsetFirstPass->CopyParameterOverrides(MyMICollisionPainterOffset);
				MyMICollisionPainterOffsetFirstPass->SetTextureParameterValue(TEXT("Texture"), PainterTarget);
				if (bValidatePainterFrame)
				{
					MyMICollisionPainterOffsetComparisonFirstPass->CopyParameterOverrides(
						MyMICollisionPainterOffsetFirstPass);
					MyMICollisionPainterOffsetComparisonFirstPass->SetTextureParameterValue(
						TEXT("Texture"), MyRDGPainterComparisonTarget);
				}

				MyMICollisionPainterOffset->SetScalarParameterValue(TEXT("WorldOffsetDeltaX"), 0.0f);
				MyMICollisionPainterOffset->SetScalarParameterValue(TEXT("WorldOffsetDeltaY"), 0.0f);
				MyMICollisionPainterOffset->SetTextureParameterValue(TEXT("Texture"), CompositeTarget);
				if (MySimplePainterMode)
				{
					for (UMaterialInstanceDynamic* PainterMaterial : { MyMICollisionPainterLine.Get(), MyMICollisionPainterDot.Get() })
					{
						if (IsValid(PainterMaterial))
						{
							PainterMaterial->SetScalarParameterValue(TEXT("VeloMult"), static_cast<float>(MyVeloFromBrushMotion));
						}
					}
					MyMICollisionPainterOffset->SetScalarParameterValue(TEXT("DensityTxtScale"), static_cast<float>(MyDensityTxtScale));
					MyMICollisionPainterOffset->SetScalarParameterValue(TEXT("DensityTxtMult"), static_cast<float>(MyDensityTxtMult));
				}
				if (bValidatePainterFrame)
				{
					MyMICollisionPainterOffsetComparisonSecondPass->CopyParameterOverrides(
						MyMICollisionPainterOffset);
					MyMICollisionPainterOffsetComparisonSecondPass->SetTextureParameterValue(
						TEXT("Texture"), MyRDGCompositeComparisonTarget);
					if (!MySimplePainterMode)
					{
						MyMICompositeAndGradientComparison->CopyParameterOverrides(MyMICompositeAndGradient);
						MyMICompositeAndGradientComparison->SetTextureParameterValue(
							TEXT("VeloPainter"), MyRDGPainterComparisonTarget);
					}
				}

				FMyNinjaFluidRenderPipeline::MyDrawPainterComposite(
					this,
					PainterTarget,
					CompositeTarget,
					MyMICollisionPainterOffsetFirstPass,
					MyMICollisionPainterOffset,
					MySimplePainterMode ? nullptr : MyMICompositeAndGradient.Get(),
					bUseRDGPainter);
				if (bValidatePainterFrame)
				{
					FMyNinjaFluidRenderPipeline::MyDrawPainterComposite(
						this,
						MyRDGPainterComparisonTarget,
						MyRDGCompositeComparisonTarget,
						MyMICollisionPainterOffsetComparisonFirstPass,
						MyMICollisionPainterOffsetComparisonSecondPass,
						MySimplePainterMode ? nullptr : MyMICompositeAndGradientComparison.Get(),
						!bUseRDGPainter);
					FMyNinjaFluidRenderPipeline::MyComparePainterCompositeTargets(
						this,
						bUseRDGPainter ? MyRDGPainterComparisonTarget.Get() : PainterTarget,
						bUseRDGPainter ? PainterTarget : MyRDGPainterComparisonTarget.Get(),
						bUseRDGPainter ? MyRDGCompositeComparisonTarget.Get() : CompositeTarget,
						bUseRDGPainter ? CompositeTarget : MyRDGCompositeComparisonTarget.Get(),
						this,
						PainterFrameIndex);
				}
			}
		}
	}
	else if (!MySimplePainterMode)
	{
		Draw(CompositeTarget, MyMICompositeAndGradient);
	}


	if (bOutputRequired || bValidateOutput)
	{
		const uint64 OutputFrameIndex = MyRDGOutputFrameIndex++;
		UTextureRenderTarget2D* ComparisonTarget = nullptr;
		if (IsValid(OutputTarget) &&
			FMyNinjaFluidRenderPipeline::MyShouldValidateOutput(OutputFrameIndex))
		{
			const bool bNeedsComparisonTarget =
				!IsValid(MyRDGOutputComparisonTarget) ||
				MyRDGOutputComparisonTarget->SizeX != OutputTarget->SizeX ||
				MyRDGOutputComparisonTarget->SizeY != OutputTarget->SizeY ||
				MyRDGOutputComparisonTarget->RenderTargetFormat != OutputTarget->RenderTargetFormat;
			if (bNeedsComparisonTarget)
			{
				MyRDGOutputComparisonTarget = UMyNinjaLiveFunctions::MyCreateRenderTarget(
					this,
					OutputTarget->SizeX,
					OutputTarget->SizeY,
					OutputTarget->RenderTargetFormat,
					MySimAreaClamp,
					OutputTarget->LODGroup,
					OutputTarget->Filter);
			}
			ComparisonTarget = MyRDGOutputComparisonTarget;
		}
		FMyNinjaFluidRenderPipeline::MyDrawOutput(
			this,
			OutputTarget,
			MyMIOutput,
			ComparisonTarget,
			this,
			OutputFrameIndex);
	}

	if (!MySimplePainterMode)
	{
		const uint64 CoreFrameIndex = MyRDGCoreFrameIndex++;
		UTextureRenderTarget2D* ComparisonAdvectionTarget = nullptr;
		UTextureRenderTarget2D* ComparisonDivergenceTarget = nullptr;
		UMaterialInstanceDynamic* ComparisonAdvectionMaterial = nullptr;
		UMaterialInstanceDynamic* ComparisonDivergenceMaterial = nullptr;
		if (FMyNinjaFluidRenderPipeline::MyShouldValidateCore(CoreFrameIndex) &&
			IsValid(AdvectionTarget) && IsValid(PressureTarget) &&
			IsValid(MyMIAdvection) && IsValid(MyMIDivergence))
		{
			auto EnsureComparisonTarget = [this](
				TObjectPtr<UTextureRenderTarget2D>& ComparisonTarget,
				UTextureRenderTarget2D* SourceTarget)
			{
				if (!IsValid(ComparisonTarget) ||
					ComparisonTarget->SizeX != SourceTarget->SizeX ||
					ComparisonTarget->SizeY != SourceTarget->SizeY ||
					ComparisonTarget->RenderTargetFormat != SourceTarget->RenderTargetFormat)
				{
					ComparisonTarget = UMyNinjaLiveFunctions::MyCreateRenderTarget(
						this,
						SourceTarget->SizeX,
						SourceTarget->SizeY,
						SourceTarget->RenderTargetFormat,
						MySimAreaClamp,
						SourceTarget->LODGroup,
						SourceTarget->Filter);
				}
			};
			EnsureComparisonTarget(MyRDGAdvectionComparisonTarget, AdvectionTarget);
			EnsureComparisonTarget(MyRDGDivergenceComparisonTarget, PressureTarget);

			ComparisonAdvectionTarget = MyRDGAdvectionComparisonTarget;
			ComparisonDivergenceTarget = MyRDGDivergenceComparisonTarget;
			ComparisonAdvectionMaterial = MyMIAdvection;
			ComparisonDivergenceMaterial = MyMIDivergence;
		}

		FMyNinjaFluidRenderPipeline::MyDrawAdvectionDivergence(
			this,
			AdvectionTarget,
			MyMIAdvection,
			PressureTarget,
			MyMIDivergence,
			ComparisonAdvectionTarget,
			ComparisonAdvectionMaterial,
			ComparisonDivergenceTarget,
			ComparisonDivergenceMaterial,
			this,
			CoreFrameIndex);

		const int32 Solver1Iterations = MyLOD1ReduceSimQuality
			? FMath::Min(MyFluidSolver1Iterations, MyPressureSolver1MaxIterations)
			: MyPressureSolver1MaxIterations;
		const int32 LastIteration = MyUsePressureSolver1DefaultIs2
			? FMath::Max(Solver1Iterations - 2, 0)
			: MyPressureSolver2MaxIterations - 1;
		const uint64 PressureFrameIndex = MyRDGPressureFrameIndex++;
		const bool bUseRDGPressure = FMyNinjaFluidRenderPipeline::MyUseRDGPressure();
		bool bValidatePressureFrame =
			FMyNinjaFluidRenderPipeline::MyShouldValidatePressure(PressureFrameIndex) &&
			IsValid(PressureTarget) && IsValid(PressureTempTarget) &&
			IsValid(MyMIPressureCycle1) && IsValid(MyMIPressureCycle2);
		if (bValidatePressureFrame)
		{
			auto EnsureComparisonTarget = [this](
				TObjectPtr<UTextureRenderTarget2D>& ComparisonTarget,
				UTextureRenderTarget2D* SourceTarget)
			{
				if (!IsValid(ComparisonTarget) ||
					ComparisonTarget->SizeX != SourceTarget->SizeX ||
					ComparisonTarget->SizeY != SourceTarget->SizeY ||
					ComparisonTarget->RenderTargetFormat != SourceTarget->RenderTargetFormat)
				{
					ComparisonTarget = UMyNinjaLiveFunctions::MyCreateRenderTarget(
						this,
						SourceTarget->SizeX,
						SourceTarget->SizeY,
						SourceTarget->RenderTargetFormat,
						MySimAreaClamp,
						SourceTarget->LODGroup,
						SourceTarget->Filter);
				}
			};
			EnsureComparisonTarget(MyRDGPressureComparisonTarget, PressureTarget);
			EnsureComparisonTarget(MyRDGPressureTempComparisonTarget, PressureTempTarget);

			auto EnsureComparisonMaterial = [this](
				TObjectPtr<UMaterialInstanceDynamic>& ComparisonMaterial,
				UMaterialInstanceDynamic* SourceMaterial)
			{
				UMaterialInterface* ParentMaterial = IsValid(SourceMaterial->Parent)
					? SourceMaterial->Parent.Get()
					: SourceMaterial;
				if (!IsValid(ComparisonMaterial) || ComparisonMaterial->Parent != ParentMaterial)
				{
					ComparisonMaterial = UMaterialInstanceDynamic::Create(ParentMaterial, this);
				}
			};
			EnsureComparisonMaterial(MyMIPressureCycle1Comparison, MyMIPressureCycle1);
			EnsureComparisonMaterial(MyMIPressureCycle2Comparison, MyMIPressureCycle2);

			bValidatePressureFrame =
				IsValid(MyRDGPressureComparisonTarget) &&
				IsValid(MyRDGPressureTempComparisonTarget) &&
				IsValid(MyMIPressureCycle1Comparison) &&
				IsValid(MyMIPressureCycle2Comparison);
			if (bValidatePressureFrame)
			{
				FMyNinjaFluidRenderPipeline::MyCopyPressureTargets(
					PressureTarget,
					MyRDGPressureComparisonTarget,
					PressureTempTarget,
					MyRDGPressureTempComparisonTarget);
			}
		}
		for (int32 Iteration = 0; Iteration <= LastIteration; ++Iteration)
		{
			const bool bLastIteration = Iteration == LastIteration;
			const float SingleIterationFlag =
				(!(Iteration != LastIteration && LastIteration > 0)) ? 1.0f : 0.0f;
			for (UMaterialInstanceDynamic* PressureMaterial : { MyMIPressureCycle1.Get(), MyMIPressureCycle2.Get() })
			{
				if (IsValid(PressureMaterial))
				{
					PressureMaterial->SetScalarParameterValue(TEXT("SingleIterationFlag"), SingleIterationFlag);
				}
			}

			if (IsValid(MyMIPressureCycle2))
			{
				MyMIPressureCycle2->SetScalarParameterValue(TEXT("KeepDivergenceBuffer"),
					bLastIteration ? 0.0f : 1.0f);
				MyMIPressureCycle2->SetScalarParameterValue(TEXT("WorldOffsetDeltaX"), 0.0f);
				MyMIPressureCycle2->SetScalarParameterValue(TEXT("WorldOffsetDeltaY"), 0.0f);
			}

			if (bValidatePressureFrame)
			{
				MyMIPressureCycle1Comparison->CopyParameterOverrides(MyMIPressureCycle1);
				MyMIPressureCycle1Comparison->SetTextureParameterValue(
					TEXT("Texture"), MyRDGPressureComparisonTarget);
				MyMIPressureCycle2Comparison->CopyParameterOverrides(MyMIPressureCycle2);
				MyMIPressureCycle2Comparison->SetTextureParameterValue(
					TEXT("Texture"), MyRDGPressureTempComparisonTarget);
			}

			FMyNinjaFluidRenderPipeline::MyDrawPressurePair(
				this,
				PressureTarget,
				PressureTempTarget,
				MyMIPressureCycle1,
				MyMIPressureCycle2,
				bUseRDGPressure);
			if (bValidatePressureFrame)
			{
				FMyNinjaFluidRenderPipeline::MyDrawPressurePair(
					this,
					MyRDGPressureComparisonTarget,
					MyRDGPressureTempComparisonTarget,
					MyMIPressureCycle1Comparison,
					MyMIPressureCycle2Comparison,
					!bUseRDGPressure);
			}

			if (IsValid(MyMIPressureCycle1))
			{
				MyMIPressureCycle1->SetScalarParameterValue(TEXT("KernelMult"), static_cast<float>(KernelMultiplier));
				MyMIPressureCycle1->SetScalarParameterValue(TEXT("WorldOffsetDeltaX"), 0.0f);
				MyMIPressureCycle1->SetScalarParameterValue(TEXT("WorldOffsetDeltaY"), 0.0f);
			}
			if (IsValid(MyMIPressureCycle2))
			{
				MyMIPressureCycle2->SetScalarParameterValue(TEXT("KernelMult"), static_cast<float>(KernelMultiplier));
			}
		}

		if (bValidatePressureFrame)
		{
			FMyNinjaFluidRenderPipeline::MyComparePressureTargets(
				this,
				bUseRDGPressure ? MyRDGPressureComparisonTarget.Get() : PressureTarget,
				bUseRDGPressure ? PressureTarget : MyRDGPressureComparisonTarget.Get(),
				bUseRDGPressure ? MyRDGPressureTempComparisonTarget.Get() : PressureTempTarget,
				bUseRDGPressure ? PressureTempTarget : MyRDGPressureTempComparisonTarget.Get(),
				this,
				PressureFrameIndex);
		}

		ThenExec = true;
	}
}

void UMyNinjaLiveComponent::MyApplyRDGOutputDiffResult(
	int64 SampleId,
	FLinearColor MaxDifference,
	int32 ExceededPixelCount,
	int32 ComparedPixelCount,
	float Tolerance)
{
	MyRDGOutputDiffSampleId = SampleId;
	MyRDGOutputDiffMax = MaxDifference;
	MyRDGOutputDiffExceededPixelCount = ExceededPixelCount;
	MyRDGOutputDiffComparedPixelCount = ComparedPixelCount;
	MyRDGOutputDiffTolerance = Tolerance;
	MyRDGOutputDiffWithinTolerance = ExceededPixelCount == 0;

	UE_LOG(LogTemp, Display,
		TEXT("FluidTest NinjaLive RT_Output GPU diff sample=%lld max=(%.9g, %.9g, %.9g, %.9g) exceeded=%d/%d tolerance=%.9g"),
		SampleId,
		MaxDifference.R,
		MaxDifference.G,
		MaxDifference.B,
		MaxDifference.A,
		ExceededPixelCount,
		ComparedPixelCount,
		Tolerance);
}

void UMyNinjaLiveComponent::MyApplyRDGCoreDiffResult(
	EMyNinjaRDGDiffTarget DiffTarget,
	int64 SampleId,
	FLinearColor MaxDifference,
	int32 ExceededPixelCount,
	int32 ComparedPixelCount,
	float Tolerance)
{
	FMyNinjaRDGTextureDiffDiagnostics* Diagnostics = nullptr;
	const TCHAR* TargetName = TEXT("Unknown");
	switch (DiffTarget)
	{
	case EMyNinjaRDGDiffTarget::Advection:
		Diagnostics = &MyRDGAdvectionDiff;
		TargetName = TEXT("Advection");
		break;
	case EMyNinjaRDGDiffTarget::Divergence:
		Diagnostics = &MyRDGDivergenceDiff;
		TargetName = TEXT("Divergence");
		break;
	case EMyNinjaRDGDiffTarget::Pressure:
		Diagnostics = &MyRDGPressureDiff;
		TargetName = TEXT("Pressure");
		break;
	case EMyNinjaRDGDiffTarget::PressureTemp:
		Diagnostics = &MyRDGPressureTempDiff;
		TargetName = TEXT("PressureTemp");
		break;
	case EMyNinjaRDGDiffTarget::Painter:
		Diagnostics = &MyRDGPainterDiff;
		TargetName = TEXT("Painter");
		break;
	case EMyNinjaRDGDiffTarget::Composite:
		Diagnostics = &MyRDGCompositeDiff;
		TargetName = TEXT("Composite");
		break;
	default:
		return;
	}
	Diagnostics->SampleId = SampleId;
	Diagnostics->MaxDifference = MaxDifference;
	Diagnostics->ExceededPixelCount = ExceededPixelCount;
	Diagnostics->ComparedPixelCount = ComparedPixelCount;
	Diagnostics->Tolerance = Tolerance;
	Diagnostics->WithinTolerance = ExceededPixelCount == 0;

	UE_LOG(LogTemp, Display,
		TEXT("FluidTest NinjaLive %s GPU diff sample=%lld max=(%.9g, %.9g, %.9g, %.9g) exceeded=%d/%d tolerance=%.9g"),
		TargetName,
		SampleId,
		MaxDifference.R,
		MaxDifference.G,
		MaxDifference.B,
		MaxDifference.A,
		ExceededPixelCount,
		ComparedPixelCount,
		Tolerance);
}

void UMyNinjaLiveComponent::MyFluidCoreStep()
{
	MySetPosVelocityScaleArraysToPainterV2();


	if (MySimplePainterMode && !MyEnablePainterDoubleBuffering)
	{
		return;
	}

	MyDynamicSimspeedAndWorldOffsetAdjustment();

	bool ThenExec = false;
	bool PainterV2Exec = false;
	MyCoreFluidsimOPs(ThenExec, PainterV2Exec);


	if (ThenExec)
	{
		MySetAdditionalFluidsimParams();


		if (MyEnableRayMarching)
		{
			MyRaymarchBasedLightingOPs();
		}

		MyDrawInternalRenderTargetToExternal();
	}


	if (PainterV2Exec)
	{
		MyForwardScalarParamsToNiagara();
	}
}

void UMyNinjaLiveComponent::MyInitPainterV2()
{
	MyDestroyPainterV2();
	MyPainterScalarParameterInfos.Reset();
	MyLastForwardedPainterScalarValues.Reset();
	bMyPainterScalarParameterCacheInitialized = false;
	MyLastSentPositionArray.Reset();
	MyLastSentLastPositionArray.Reset();
	MyLastSentVelocityArray.Reset();
	MyLastSentBrushSizeArray.Reset();
	bMyPainterArraysSent = false;
	bMyLastSentPosInterpolValid = false;


	if (!MyUsePAINTER_V2_ToTrackObjects || MySingleTargetMode_LEGACY)
	{
		MyUsePAINTER_V2_ToTrackObjects = false;
		return;
	}

	const int32 SystemIndex = MyPV2_Connect_TrackpointsWithLines ? 1 : 0;

	if (!MyCoreNiagaraSystems.IsValidIndex(SystemIndex) || !IsValid(MyCoreNiagaraSystems[SystemIndex]))
	{
		MyUsePAINTER_V2_ToTrackObjects = false;
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		MyUsePAINTER_V2_ToTrackObjects = false;
		return;
	}


	MyNiagaraBasedPainter = NewObject<UNiagaraComponent>(OwnerActor, UNiagaraComponent::StaticClass(), NAME_None);
	if (!IsValid(MyNiagaraBasedPainter))
	{
		MyUsePAINTER_V2_ToTrackObjects = false;
		return;
	}
	OwnerActor->AddInstanceComponent(MyNiagaraBasedPainter);
	if (USceneComponent* RootComponent = OwnerActor->GetRootComponent())
	{
		MyNiagaraBasedPainter->SetupAttachment(RootComponent);
	}
	MyNiagaraBasedPainter->RegisterComponent();
	MyNiagaraBasedPainter->SetAsset(MyCoreNiagaraSystems[SystemIndex], false);


	const TObjectPtr<UTextureRenderTarget2D>* PainterTarget = MyRenderTargetsMap.Find(TEXT("RT_Painter"));
	UTextureRenderTarget2D* PainterRenderTarget = PainterTarget ? PainterTarget->Get() : nullptr;
	MyNiagaraBasedPainter->SetVariableTextureRenderTarget(TEXT("User.PaintbufferOutput"), PainterRenderTarget);

	MyNiagaraBasedPainter->SetVariableBool(TEXT("User.PosInterpol"), false);

	MyApplyPainterV2SharedParameters();

	if (UWorld* World = GetWorld())
	{

		World->GetTimerManager().ClearTimer(MyNiagaraPainterV2SafetyTimer);
		if (MyNiagaraVariableSetSafetyDelay > 0.0)
		{
			World->GetTimerManager().SetTimer(MyNiagaraPainterV2SafetyTimer, this,
				&UMyNinjaLiveComponent::MySetPainterV2PaintbufferInput, MyNiagaraVariableSetSafetyDelay, false);
		}
		else
		{
			MyNiagaraPainterV2SafetyTimer = World->GetTimerManager().SetTimerForNextTick(this,
				&UMyNinjaLiveComponent::MySetPainterV2PaintbufferInput);
		}
		World->GetTimerManager().ClearTimer(MyNiagaraPainterV2CooldownTimer);
		if (MyPV2LineDrawingFailCooldownTime > 0.0)
		{
			World->GetTimerManager().SetTimer(MyNiagaraPainterV2CooldownTimer, this,
				&UMyNinjaLiveComponent::MyFinalizePainterV2Setup, MyPV2LineDrawingFailCooldownTime * 2.0, false);
		}
		else
		{
			MyNiagaraPainterV2CooldownTimer = World->GetTimerManager().SetTimerForNextTick(this,
				&UMyNinjaLiveComponent::MyFinalizePainterV2Setup);
		}
	}
	else
	{

		MySetPainterV2PaintbufferInput();
		MyFinalizePainterV2Setup();
	}
}

void UMyNinjaLiveComponent::MyDestroyPainterV2()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MyNiagaraPainterV2SafetyTimer);
		World->GetTimerManager().ClearTimer(MyNiagaraPainterV2CooldownTimer);
	}

	UNiagaraComponent* ExistingPainter = MyNiagaraBasedPainter.Get();
	MyNiagaraBasedPainter = nullptr;
	if (!IsValid(ExistingPainter))
	{
		return;
	}

	ExistingPainter->Deactivate();
	if (AActor* PainterOwner = ExistingPainter->GetOwner())
	{
		PainterOwner->RemoveInstanceComponent(ExistingPainter);
	}
	ExistingPainter->DestroyComponent();
}

void UMyNinjaLiveComponent::MyForwardScalarParamsToNiagara()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FluidSim_MyForwardScalarParamsToNiagara);

	if (!MyUsePAINTER_V2_ToTrackObjects || MySingleTargetMode_LEGACY ||
		!IsValid(MyMICollisionPainterDot) || !IsValid(MyNiagaraBasedPainter))
	{
		return;
	}

	if (!bMyPainterScalarParameterCacheInitialized)
	{
		TArray<FGuid> ParameterIds;
		MyMICollisionPainterDot->GetAllScalarParameterInfo(MyPainterScalarParameterInfos, ParameterIds);
		MyPainterScalarParameterInfos.RemoveAll([](const FMaterialParameterInfo& ParameterInfo)
		{
			return ParameterInfo.Name == TEXT("BrushSize");
		});
		bMyPainterScalarParameterCacheInitialized = true;
	}

	for (const FMaterialParameterInfo& ParameterInfo : MyPainterScalarParameterInfos)
	{
		float ParameterValue = 0.0f;
		if (MyMICollisionPainterDot->GetScalarParameterValue(
			FHashedMaterialParameterInfo(ParameterInfo), ParameterValue))
		{
			const float* LastValue = MyLastForwardedPainterScalarValues.Find(ParameterInfo.Name);
			if (LastValue && *LastValue == ParameterValue)
			{
				continue;
			}
			MyNiagaraBasedPainter->SetVariableFloat(ParameterInfo.Name, ParameterValue);
			MyLastForwardedPainterScalarValues.Add(ParameterInfo.Name, ParameterValue);
		}
	}
}

void UMyNinjaLiveComponent::MySetPosVelocityScaleArraysToPainterV2()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FluidSim_MySetPosVelocityScaleArraysToPainterV2);

	if (!MyUsePAINTER_V2_ToTrackObjects || MySingleTargetMode_LEGACY || !IsValid(MyNiagaraBasedPainter))
	{
		return;
	}

	const bool bForceUpload = !bMyPainterArraysSent;
	if (MyPV2_Connect_TrackpointsWithLines)
	{
		const bool bPositionArraysMatch = MyLastPositionArray.Num() == MyPositionArray.Num();
		const bool bTracePositionUnchanged = FVector2D(MyTraceMeshPos) == FVector2D(MyTraceMeshLastPos);
		const bool bCanInterpolate =
			(MyQuantizerStepSize < 1 || bTracePositionUnchanged) && bPositionArraysMatch;
		const bool bEnableInterpolation = bCanInterpolate && MyPV2_Interpolation &&
			MyMaxSamplingFPS == MySamplingFPS && MyHitValid;
		if (!bMyLastSentPosInterpolValid || bMyLastSentPosInterpol != bEnableInterpolation)
		{
			MyNiagaraBasedPainter->SetVariableBool(TEXT("User.PosInterpol"), bEnableInterpolation);
			bMyLastSentPosInterpol = bEnableInterpolation;
			bMyLastSentPosInterpolValid = true;
		}

		if (bForceUpload || MyLastSentPositionArray != MyPositionArray)
		{
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector2D(
				MyNiagaraBasedPainter, TEXT("User.PositionArray2D"), MyPositionArray);
			MyLastSentPositionArray = MyPositionArray;
		}

		if (MyPositionArray.IsEmpty())
		{
			MyLastPositionArray.Reset();
		}
		else if (MyLastPositionArray.IsEmpty())
		{
			MyLastPositionArray = MyPositionArray;
		}

		const bool bTrackedComponentsUnchanged =
			MyPrimitivesArray == MyLastPrimitivesArray && MySKmeshesArray == MyLastSKmeshesArray;
		if (!bTrackedComponentsUnchanged)
		{
			MyLastPositionArray = MyPositionArray;
		}

		if (bForceUpload || MyLastSentLastPositionArray != MyLastPositionArray)
		{
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector2D(
				MyNiagaraBasedPainter, TEXT("User.LastPositionArray2D"), MyLastPositionArray);
			MyLastSentLastPositionArray = MyLastPositionArray;
		}
	}
	else
	{
		if (!bMyLastSentPosInterpolValid || bMyLastSentPosInterpol)
		{
			MyNiagaraBasedPainter->SetVariableBool(TEXT("User.PosInterpol"), false);
			bMyLastSentPosInterpol = false;
			bMyLastSentPosInterpolValid = true;
		}
		if (bForceUpload || MyLastSentPositionArray != MyPositionArray)
		{
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector2D(
				MyNiagaraBasedPainter, TEXT("User.PositionArray2D"), MyPositionArray);
			MyLastSentPositionArray = MyPositionArray;
		}
	}

	if (bForceUpload || MyLastSentVelocityArray != MyVelocityArray)
	{
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayColor(
			MyNiagaraBasedPainter, TEXT("User.VelocityArray"), MyVelocityArray);
		MyLastSentVelocityArray = MyVelocityArray;
	}

	if (bForceUpload || MyLastSentBrushSizeArray != MyBrushSizeArray)
	{
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
			MyNiagaraBasedPainter, TEXT("User.BrushSizeArray"), MyBrushSizeArray);
		MyLastSentBrushSizeArray = MyBrushSizeArray;
	}
	bMyPainterArraysSent = true;
}

void UMyNinjaLiveComponent::MyClearPosVelocityScaleArraysPainterV2()
{
	if (!MyUsePAINTER_V2_ToTrackObjects || MySingleTargetMode_LEGACY)
	{
		return;
	}

	if (MyPositionArray.IsEmpty())
	{
		MyLastPositionArray.Reset();
	}
	else
	{
		MyLastPositionArray = MyPositionArray;
	}

	MyPositionArray.Reset();
	MyVelocityArray.Reset();
	MyBrushSizeArray.Reset();

	if (MyPV2_Connect_TrackpointsWithLines)
	{
		MyLastPrimitivesArray = MyPrimitivesArray;
		MyPrimitivesArray.Reset();
		MyLastSKmeshesArray = MySKmeshesArray;
		MySKmeshesArray.Reset();
	}
}

void UMyNinjaLiveComponent::MyBuildBrushPositionArray()
{

	if (MyUsePAINTER_V2_ToTrackObjects && !MySingleTargetMode_LEGACY)
	{
		MyPositionArray.Add(FVector2D(MyPosition1_2D.R, MyPosition1_2D.G));
	}
}

void UMyNinjaLiveComponent::MyFinalDealRTAndBrush()
{

	if (!(MyUsePAINTER_V2_ToTrackObjects && !MySingleTargetMode_LEGACY))
	{
		const TObjectPtr<UTextureRenderTarget2D>* PainterRT = MyRenderTargetsMap.Find(TEXT("RT_Painter"));
		if (PainterRT && IsValid(PainterRT->Get()) && IsValid(MyMICollisionPainterDot))
		{
			UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, PainterRT->Get(), MyMICollisionPainterDot);
		}
	}


	if (IsValid(MyMICollisionPainterDot))
	{
		MyMICollisionPainterDot->SetScalarParameterValue(TEXT("Multitarget"), 1.0f);
	}
	MyBuildBrushPositionArray();
}

void UMyNinjaLiveComponent::MyDrawInternalRenderTargetToExternal()
{
	if (!MyDrawInternalRenderTargetToExternalEnabled)
	{
		return;
	}


	if (!bMyExternalRenderTargetExportValidated)
	{
		bMyExternalRenderTargetExportValidated = true;
		bMyExternalRenderTargetExportGateOpen =
			MyInternalRenderTargetsToExport.Num() == MyExternalRenderTargets.Num();
		for (UTextureRenderTarget2D* ExternalTarget : MyExternalRenderTargets)
		{
			if (!IsValid(ExternalTarget))
			{
				bMyExternalRenderTargetExportGateOpen = false;
				break;
			}
		}
	}

	if (!bMyExternalRenderTargetExportGateOpen)
	{
		return;
	}

	for (int32 Index = 0; Index < MyInternalRenderTargetsToExport.Num(); ++Index)
	{
		UMaterialInstanceDynamic* SourceMaterial = nullptr;
		switch (MyInternalRenderTargetsToExport[Index])
		{
		case EMyRenderTargetList::VelocityDensity:
			SourceMaterial = MyMICompositeAndGradient;
			break;
		case EMyRenderTargetList::Divergence:
			SourceMaterial = MyMIDivergence;
			break;
		case EMyRenderTargetList::Pressure:
			SourceMaterial = MyMIPressureCycle1;
			break;
		case EMyRenderTargetList::Painter:
			SourceMaterial = MySingleTargetMode_LEGACY
				? MyMICollisionPainterLine.Get()
				: MyMICollisionPainterDot.Get();
			break;
		case EMyRenderTargetList::Output:
			SourceMaterial = MyMIOutput;
			break;
		default:
			continue;
		}

		if (IsValid(SourceMaterial))
		{
			UKismetRenderingLibrary::DrawMaterialToRenderTarget(
				this, MyExternalRenderTargets[Index], SourceMaterial);
		}
	}
}

void UMyNinjaLiveComponent::MySetPainterV2PaintbufferInput()
{
	if (IsValid(MyNiagaraBasedPainter))
	{

		const TObjectPtr<UTextureRenderTarget2D>* PainterTarget = MyRenderTargetsMap.Find(TEXT("RT_Painter"));
		MyNiagaraBasedPainter->SetVariableTexture(TEXT("User.PaintbufferInput"),
			PainterTarget ? PainterTarget->Get() : nullptr);
	}
}

void UMyNinjaLiveComponent::MyFinalizePainterV2Setup()
{
	if (!IsValid(MyNiagaraBasedPainter))
	{
		return;
	}

	const bool bEnableInterpolation = MyPV2_Interpolation && MyMaxSamplingFPS == MySamplingFPS;

	MyNiagaraBasedPainter->SetVariableBool(TEXT("User.PosInterpol"), bEnableInterpolation);
	MyNiagaraBasedPainter->SetVariableBool(TEXT("User.GenerateVelocity"), MyPV2_GenerateVelocity);
	MyApplyPainterV2SharedParameters();
}

void UMyNinjaLiveComponent::MyApplyPainterV2SharedParameters()
{
	if (!IsValid(MyNiagaraBasedPainter))
	{
		return;
	}


	MyNiagaraBasedPainter->SetVariableBool(TEXT("User.Quantizer"), MyQuantizerStepSize > 0);
	MyNiagaraBasedPainter->SetVariableVec2(TEXT("User.SimResolution"),
		FVector2D(static_cast<double>(MyResolutionX), static_cast<double>(MyResolutionY)));
	MyNiagaraBasedPainter->SetVariableFloat(TEXT("User.StopLineDrawAboveThisVelocity"),
		static_cast<float>(MyPV2StopLineDrawingAboveThisVelocity));
	MyNiagaraBasedPainter->SetVariableFloat(TEXT("User.AmplifyV2BrushStrength"),
		static_cast<float>(MyAdjustPainterV2BrushStrength));
	MyNiagaraBasedPainter->SetVariableFloat(TEXT("User.BrushNoiseVelo"),
		static_cast<float>(MyAdjustPainterV2BrushVeloNoise));
	if (IsValid(MyPainterV2BrushVeloNoiseTexture))
	{

		MyNiagaraBasedPainter->SetVariableTexture(TEXT("User.BrushNoiseVeloTexture"), MyPainterV2BrushVeloNoiseTexture);
	}
	if (IsValid(MyTraceMeshComponent))
	{

		const FVector Scale = MyTraceMeshComponent->GetRelativeScale3D();
		MyNiagaraBasedPainter->SetVariableFloat(TEXT("User.TraceMeshMaxExtent"), FMath::Max3(Scale.X, Scale.Y, Scale.Z));
	}


	MyPositionArray.Reset();
	MyLastPositionArray.Reset();
	MyVelocityArray.Reset();
	MyBrushSizeArray.Reset();

	if (MyForceMaxSamplingFPSToNiagara)
	{

		MyNiagaraBasedPainter->SetForceSolo(true);
		MyNiagaraBasedPainter->SetComponentTickInterval(1.0 / static_cast<double>(FMath::Max(MyMaxSamplingFPS, 1)));
		MyNiagaraBasedPainter->ReinitializeSystem();
	}
}

void UMyNinjaLiveComponent::MyCreateOrAcquireRenderTargets()
{
	MyRenderTargetsMap.Empty();
	MyRDGOutputComparisonTarget = nullptr;
	MyRDGOutputTargetCreatedForValidation = false;
	MyRDGAdvectionComparisonTarget = nullptr;
	MyRDGDivergenceComparisonTarget = nullptr;
	MyRDGCoreFrameIndex = 0;
	MyRDGPressureComparisonTarget = nullptr;
	MyRDGPressureTempComparisonTarget = nullptr;
	MyMIPressureCycle1Comparison = nullptr;
	MyMIPressureCycle2Comparison = nullptr;
	MyRDGPressureFrameIndex = 0;
	MyRDGPainterComparisonTarget = nullptr;
	MyRDGCompositeComparisonTarget = nullptr;
	MyMICollisionPainterOffsetFirstPass = nullptr;
	MyMICollisionPainterOffsetComparisonFirstPass = nullptr;
	MyMICollisionPainterOffsetComparisonSecondPass = nullptr;
	MyMICompositeAndGradientComparison = nullptr;
	MyRDGPainterFrameIndex = 0;
	MyMapLengthTmp = MyRenderTargetsMap.Num();

	const int32 FullWidth = FMath::Max(1, MyResolutionX);
	const int32 FullHeight = FMath::Max(1, MyResolutionY);
	const ETextureRenderTargetFormat RGBAFormat =
		MySimPrecisionIndex == 0 ? RTF_RGBA16f : RTF_RGBA32f;
	const ETextureRenderTargetFormat RGFormat =
		MySimPrecisionIndex == 0 ? RTF_RG16f : RTF_RG32f;

	auto AddRenderTarget = [this](const FString& Name, int32 Width, int32 Height,
		ETextureRenderTargetFormat Format, bool bClamp)
	{
		UTextureRenderTarget2D* RenderTarget = UMyNinjaLiveFunctions::MyCreateRenderTarget(
			this, Width, Height, Format, bClamp, TEXTUREGROUP_RenderTarget, TF_Bilinear);
		if (IsValid(RenderTarget))
		{
			MyRenderTargetsMap.Add(Name, RenderTarget);
		}
	};

	if (MySimplePainterMode)
	{
		const bool bUse8BitPainterFormat =
			MyForce8bitSimplePainterBuffers && !MyUsePAINTER_V2_ToTrackObjects;
		const ETextureRenderTargetFormat PainterFormat = bUse8BitPainterFormat ? RTF_RGBA8 : RGBAFormat;
		AddRenderTarget(TEXT("RT_Painter"), FullWidth, FullHeight, PainterFormat, MySimAreaClamp);

		if (MyEnablePainterDoubleBuffering)
		{
			AddRenderTarget(TEXT("RT_Composite"), FullWidth, FullHeight, PainterFormat, MySimAreaClamp);
		}
		return;
	}

	for (int32 Index = 0; Index <= 2; ++Index)
	{
		if (MyRenderTargetsList.IsValidIndex(Index))
		{
			AddRenderTarget(MyRenderTargetsList[Index], FullWidth, FullHeight, RGBAFormat, MySimAreaClamp);
		}
	}

	const int32 PressureDivisor = MyHalfResPressureAndDivergenceBuffers ? 2 : 1;
	const int32 PressureWidth = FullWidth / PressureDivisor;
	const int32 PressureHeight = FullHeight / PressureDivisor;
	for (int32 Index = 3; Index <= 4; ++Index)
	{
		if (MyRenderTargetsList.IsValidIndex(Index))
		{
			AddRenderTarget(MyRenderTargetsList[Index], PressureWidth, PressureHeight, RGFormat, MySimAreaClamp);
		}
	}

	const bool bHasDensityInput = MyInputMaterials.Num() > 0 || IsValid(MyInputSceneCaptureCamera);
	if (bHasDensityInput && MyRenderTargetsList.IsValidIndex(5))
	{
		AddRenderTarget(MyRenderTargetsList[5], FullWidth, FullHeight, RTF_R8, false);
	}

	if (MyMake1stOutputAvailableFor2ndOutput || MyMake1stOutputAvailableForNiagara ||
		FMyNinjaFluidRenderPipeline::MyIsOutputValidationEnabled())
	{
		const int32 OutputMultiplier = MyForce2xResolutionOutputBuffer ? 2 : 1;
		const ETextureRenderTargetFormat OutputFormat = MyForce8bitOutputBuffer ? RTF_RGBA8 : RGBAFormat;
		AddRenderTarget(TEXT("RT_Output"), FullWidth * OutputMultiplier, FullHeight * OutputMultiplier,
			OutputFormat, MySimAreaClamp);
		MyRDGOutputTargetCreatedForValidation =
			!MyMake1stOutputAvailableFor2ndOutput && !MyMake1stOutputAvailableForNiagara;
	}
}

void UMyNinjaLiveComponent::MyCreateDynamicMaterialInstances()
{
	MyCompositeScalarParameterIndices.Reset();
	MyDivergenceScalarParameterIndices.Reset();
	MyMIPressureCycle1Comparison = nullptr;
	MyMIPressureCycle2Comparison = nullptr;
	MyMICollisionPainterOffsetFirstPass = nullptr;
	MyMICollisionPainterOffsetComparisonFirstPass = nullptr;
	MyMICollisionPainterOffsetComparisonSecondPass = nullptr;
	MyMICompositeAndGradientComparison = nullptr;

	auto CreateMaterialAt = [this](int32 MaterialIndex) -> UMaterialInstanceDynamic*
	{
		if (!MyCoreSimMaterials.IsValidIndex(MaterialIndex) || !IsValid(MyCoreSimMaterials[MaterialIndex]))
		{
			return nullptr;
		}
		return UMaterialInstanceDynamic::Create(MyCoreSimMaterials[MaterialIndex], this);
	};
	auto CreatePlatformMaterial = [&CreateMaterialAt, this](int32 DesktopIndex) -> UMaterialInstanceDynamic*
	{
		return CreateMaterialAt(DesktopIndex + (MyFlipRenderTargetsForMobile ? 1 : 0));
	};



	if (!MySimplePainterMode)
	{
		MyMICompositeAndGradient = CreatePlatformMaterial(2);
		MyMIAdvection = CreatePlatformMaterial(4);
		MyMIDivergence = CreatePlatformMaterial(6);
		const int32 SolverIndex = MyUsePressureSolver1DefaultIs2 ? 1 : 0;
		MyMIPressureCycle1 = CreateMaterialAt(
			(MyFlipRenderTargetsForMobile ? 10 : 8) + SolverIndex);
		MyMIPressureCycle2 = CreateMaterialAt(
			(MyFlipRenderTargetsForMobile ? 14 : 12) + SolverIndex);
	}
	else
	{
		MyMICompositeAndGradient = nullptr;
		MyMIAdvection = nullptr;
		MyMIDivergence = nullptr;
		MyMIPressureCycle1 = nullptr;
		MyMIPressureCycle2 = nullptr;
	}
	MyMICollisionPainterLine = CreateMaterialAt(1);
	MyMICollisionPainterDot = CreateMaterialAt(0);
	MyMICollisionPainterOffset = CreatePlatformMaterial(16);

	MyMINull = IsValid(MyNullMaterial)
		? UMaterialInstanceDynamic::Create(MyNullMaterial, this)
		: nullptr;

	auto FindRenderTarget = [this](const TCHAR* Name) -> UTextureRenderTarget2D*
	{
		const TObjectPtr<UTextureRenderTarget2D>* Found = MyRenderTargetsMap.Find(Name);
		return Found ? Found->Get() : nullptr;
	};

	UTextureRenderTarget2D* Painter = FindRenderTarget(TEXT("RT_Painter"));
	UTextureRenderTarget2D* Composite = FindRenderTarget(TEXT("RT_Composite"));
	UTextureRenderTarget2D* Advection = FindRenderTarget(TEXT("RT_Advection"));
	UTextureRenderTarget2D* Pressure = FindRenderTarget(TEXT("RT_PressureDivergence"));
	UTextureRenderTarget2D* PressureTemp = FindRenderTarget(TEXT("RT_PressureDivergenceTemp"));
	UTextureRenderTarget2D* DensityInput = FindRenderTarget(TEXT("RT_DensityInputMaterial"));

	auto SetTexture = [](UMaterialInstanceDynamic* Material, FName Parameter, UTexture* Texture)
	{

		if (IsValid(Material))
		{
			Material->SetTextureParameterValue(Parameter, Texture);
		}
	};

	if (!MySimplePainterMode)
	{

		SetTexture(MyMICompositeAndGradient, TEXT("Texture"), Advection);
		SetTexture(MyMICompositeAndGradient, TEXT("PressureTexture"), Pressure);
		SetTexture(MyMICompositeAndGradient, TEXT("VeloPainter"), Painter);
		SetTexture(MyMIAdvection, TEXT("Texture"), Composite);
		SetTexture(MyMIDivergence, TEXT("Texture"), Advection);
		SetTexture(MyMIDivergence, TEXT("Texture3"), Painter);
		SetTexture(MyMIPressureCycle1, TEXT("Texture"), Pressure);
		SetTexture(MyMIPressureCycle2, TEXT("Texture"), PressureTemp);
		SetTexture(MyMICompositeAndGradient, TEXT("VeloInputTexture"), MyVelocityInput);
		if (MyUseRenderTargetAsInput)
		{

			if (UTextureRenderTarget2D* InputRenderTarget = Cast<UTextureRenderTarget2D>(MyInputRenderTarget))
			{
				SetTexture(MyMICompositeAndGradient, TEXT("TextureAdd2"), InputRenderTarget);
			}
		}
		else
		{
			SetTexture(MyMICompositeAndGradient, TEXT("TextureAdd2"), MyDensityInput);
		}
		SetTexture(MyMICompositeAndGradient, TEXT("MaterialInput"),
			IsValid(MyInputMediaPlayer) ? static_cast<UTexture*>(MyMediaTexture.Get()) : static_cast<UTexture*>(DensityInput));
		SetTexture(MyMICompositeAndGradient, TEXT("CollisionMask"), MyCollisionMask);

		if (IsValid(MyCollisionMask) &&
			UKismetSystemLibrary::GetDisplayName(MyCollisionMask) != TEXT("T_maskframe_256"))
		{
			MyCollisionMaskIsNonDefault = true;
		}
	}
	SetTexture(MyMICollisionPainterOffset, TEXT("Texture"), Painter);

	const TArray<UMaterialInstanceDynamic*> SimulationMIDs = {
		MyMICompositeAndGradient, MyMIAdvection, MyMIDivergence,
		MyMIPressureCycle1, MyMIPressureCycle2 };
	const float TexelSizeMultiplier = MyHalfResPressureAndDivergenceBuffers ? 1.0f : static_cast<float>(MySpeed);
	const float NoiseRandomOffset = MyRandomizeNoiseOffsets ? FMath::FRand() : 0.0f;
	const float DensityRandomOffset = MyRandomizeDensityTextureOffset ? FMath::FRand() : 0.0f;
	const float LargestResolution = static_cast<float>(FMath::Max(MyResolutionX, MyResolutionY));
	const FLinearColor PaintAspect = LargestResolution > 0.0f
		? FLinearColor(MyResolutionX / LargestResolution, MyResolutionY / LargestResolution, 1.0f, 1.0f)
		: FLinearColor::White;
	for (UMaterialInstanceDynamic* Material : SimulationMIDs)
	{
		if (IsValid(Material))
		{
			Material->SetScalarParameterValue(TEXT("TexelSizeMult"), TexelSizeMultiplier);
		}
	}

	if (IsValid(MyMICompositeAndGradient))
	{
		MyMICompositeAndGradient->SetScalarParameterValue(TEXT("FlowFeedback"), MyFlowFeedback);
		MyMICompositeAndGradient->SetScalarParameterValue(TEXT("Randomize"), NoiseRandomOffset);
		MyMICompositeAndGradient->SetScalarParameterValue(TEXT("DensityTxtRandomOffset"), DensityRandomOffset);
		MyMICompositeAndGradient->SetScalarParameterValue(TEXT("RGBInputMaterial"), MyRGBInputMaterial ? 1.0f : 0.0f);
		MyMICompositeAndGradient->SetScalarParameterValue(TEXT("RGBInputTexture"), MyUseRenderTargetAsInput ? 1.0f : 0.0f);
		MyMICompositeAndGradient->SetScalarParameterValue(TEXT("EnablePainterOffset"), MyEnablePainterDoubleBuffering ? 0.0f : 1.0f);
		MyMICompositeAndGradient->SetScalarParameterValue(TEXT("NullValue"),
			MyAllowAbsoluteBlackDensity ? 0.0f : 0.000001f);
	}

	if (IsValid(MyMIDivergence))
	{
		MyMIDivergence->SetScalarParameterValue(TEXT("Divergence"), MyDivergence);
	}

	for (UMaterialInstanceDynamic* PainterMaterial : { MyMICollisionPainterLine.Get(), MyMICollisionPainterDot.Get() })
	{
		if (IsValid(PainterMaterial))
		{
			PainterMaterial->SetScalarParameterValue(TEXT("DensityNoiseScale"), MyBrushDensityNoiseScale);
			PainterMaterial->SetScalarParameterValue(TEXT("DensityNoiseFreq"), MyBrushDensityNoiseFreq);
			PainterMaterial->SetScalarParameterValue(TEXT("VeloNoiseScale"), MyBrushVelocityNoiseScale);
			PainterMaterial->SetScalarParameterValue(TEXT("VeloNoiseFreq"), MyBrushVelocityNoiseFreq);
			PainterMaterial->SetScalarParameterValue(TEXT("BrushVelocityPow"), MyBrushVelocityPow);
			PainterMaterial->SetScalarParameterValue(TEXT("NoiseInWorldSpace"), MyBrushNoiseInWorldSpace ? 1.0f : 0.0f);
			PainterMaterial->SetScalarParameterValue(TEXT("BrushSensitivityToVelocity"), MyDampenBrushFactor);
			PainterMaterial->SetScalarParameterValue(TEXT("KillBrushBelowThisVelocity"),
				(MyUE5EAFLAG ? 1.0f : 0.0f) * static_cast<float>(MyDampenBrushBelowThisVelocity));
			PainterMaterial->SetScalarParameterValue(TEXT("EdgeMaskValue"), MyQuantizerStepSize < 1 ? 1.0f : 0.0f);
			PainterMaterial->SetVectorParameterValue(TEXT("PaintAspect"), PaintAspect);
		}
	}

	if (IsValid(MyMICollisionPainterOffset))
	{
		MyMICollisionPainterOffset->SetScalarParameterValue(TEXT("EdgeMask"), MyAdjustPainterV2EdgeMask);
	}

	auto ConfigurePressureMaterial = [this](UMaterialInstanceDynamic* Material, float Direction)
	{
		if (IsValid(Material))
		{
			Material->SetScalarParameterValue(TEXT("Direction"), Direction);
			Material->SetScalarParameterValue(TEXT("KernelIndexOffset"), MyExperimentalPSolver2KernelIndexOffset);
			Material->SetScalarParameterValue(TEXT("FeedbackDampening"), MyExperimentalPressureFeedback);
			Material->SetScalarParameterValue(TEXT("PressureEdgeMasking"),
				FMath::Max(static_cast<float>(MyPressureEdgeMasking), 0.01f));
			Material->SetScalarParameterValue(TEXT("DisablePressureEdgeMasking"), MyPressureEdgeMasking == 0.0 ? 1.0f : 0.0f);
			Material->SetScalarParameterValue(TEXT("PressureFeedback"), MyExpPressureFeedbackComponent);
			Material->SetScalarParameterValue(TEXT("DivergenceFeedback"), MyExpDivergenceFeedbackComponent);
		}
	};
	ConfigurePressureMaterial(MyMIPressureCycle1, 0.0f);
	ConfigurePressureMaterial(MyMIPressureCycle2, 1.0f);

}

void UMyNinjaLiveComponent::MyCreateOutputMaterialAndSetItOnTargetsStep01()
{

	MySecondaryMaterialsPresent = MySecondaryOutputMaterials.Num() != 0;
	MyTertiaryMaterialsPresent = MyTertiaryOutputMaterials.Num() != 0;
	MyMaterialCollectionPresent = IsValid(MySetInternalParamsToMaterialParamCollection);
	const int32 LastIndex = static_cast<int32>(MySecondaryMaterialsPresent) + static_cast<int32>(MyTertiaryMaterialsPresent);

	auto FindRenderTarget = [this](const TCHAR* Name) -> UTextureRenderTarget2D*
	{
		const TObjectPtr<UTextureRenderTarget2D>* Found = MyRenderTargetsMap.Find(Name);
		return Found ? Found->Get() : nullptr;
	};

	UTextureRenderTarget2D* Painter = FindRenderTarget(TEXT("RT_Painter"));
	UTextureRenderTarget2D* Pressure = FindRenderTarget(TEXT("RT_PressureDivergence"));
	UTextureRenderTarget2D* PressureTemp = FindRenderTarget(TEXT("RT_PressureDivergenceTemp"));

	for (int32 Index = 0; Index <= LastIndex; ++Index)
	{
		const TArray<TObjectPtr<UMaterialInterface>>* Materials = &MyOutputMaterials;
		int32 SelectedMaterial = MyOutputMaterialSelected;
		if (Index == 1)
		{
			Materials = &MySecondaryOutputMaterials;
			SelectedMaterial = MySecondaryOutputMaterialSelected;
		}
		else if (Index == 2)
		{
			Materials = &MyTertiaryOutputMaterials;
			SelectedMaterial = MyTertiaryOutputMaterialSelected;
		}

		const int32 ClampedIndex = FMath::Min(Materials->Num() - 1, SelectedMaterial);
		UMaterialInstanceDynamic* OutputMaterial = Materials->IsValidIndex(ClampedIndex) && IsValid((*Materials)[ClampedIndex])
			? UMaterialInstanceDynamic::Create((*Materials)[ClampedIndex], this)
			: nullptr;
		if (Index == 0)
		{
			MyMIOutput = OutputMaterial;
		}
		else if (Index == 1)
		{
			MyMISecondaryOutput = OutputMaterial;
		}
		else
		{
			MyMITertiaryOutput = OutputMaterial;
		}

		if (!IsValid(OutputMaterial))
		{
			continue;
		}

		const bool bPickPainter = MySimplePainterMode && !MyEnablePainterDoubleBuffering;
		const bool bUseOutputBuffer = Index == 1 && MyMake1stOutputAvailableFor2ndOutput;
		UTextureRenderTarget2D* FoundTexture = FindRenderTarget(
			bUseOutputBuffer ? TEXT("RT_Output") : (bPickPainter ? TEXT("RT_Painter") : TEXT("RT_Composite")));

		OutputMaterial->SetTextureParameterValue(TEXT("VelocityDensityBuffer"), FoundTexture);
		OutputMaterial->SetTextureParameterValue(TEXT("PressureBuffer"), Pressure);
		OutputMaterial->SetTextureParameterValue(TEXT("DivergenceBuffer"), PressureTemp);
		OutputMaterial->SetTextureParameterValue(TEXT("PaintBuffer"), Painter);
		OutputMaterial->SetScalarParameterValue(TEXT("FlowMapUVOffsetRandomize"),
			FMath::FRandRange(0.0f, MyRandomizeNoiseOffsets ? 1.0f : 0.0f));

		const FVector TraceMeshScale = IsValid(MyTraceMeshComponent) ? MyTraceMeshComponent->GetComponentScale() : FVector::ZeroVector;
		const FVector TraceMeshPosition = IsValid(MyTraceMeshComponent) ? MyTraceMeshComponent->GetComponentLocation() : FVector::ZeroVector;
		const FLinearColor ScaleColor(TraceMeshScale);
		const FLinearColor PositionColor(TraceMeshPosition);
		OutputMaterial->SetVectorParameterValue(TEXT("TraceMeshSize"), ScaleColor);
		OutputMaterial->SetVectorParameterValue(TEXT("TraceMeshPos"), PositionColor);

		if (MyMaterialCollectionPresent)
		{
			UKismetMaterialLibrary::SetVectorParameterValue(this, MySetInternalParamsToMaterialParamCollection,
				TEXT("TraceMeshSize"), ScaleColor);
			UKismetMaterialLibrary::SetVectorParameterValue(this, MySetInternalParamsToMaterialParamCollection,
				TEXT("TraceMeshPos"), PositionColor);
		}

		if (Index != 0)
		{
			OutputMaterial->SetTextureParameterValue(TEXT("DensityBuffer"), FoundTexture);
			OutputMaterial->SetTextureParameterValue(TEXT("VelocityBuffer"), FoundTexture);
			OutputMaterial->SetTextureParameterValue(TEXT("VelocityDensityMap"), FoundTexture);
			OutputMaterial->SetTextureParameterValue(TEXT("CloudVelocity"), FoundTexture);
			OutputMaterial->SetTextureParameterValue(TEXT("CloudDensity"), FoundTexture);
		}
	}
}

void UMyNinjaLiveComponent::MyCreateOutputMaterialAndSetItOnTargetsStep02()
{
	MyApplyOutputMaterialToTraceMesh();
	MyApplyOutputMaterialsToTaggedActors();
}

void UMyNinjaLiveComponent::MyApplyOutputMaterialToTraceMesh()
{

	if (!IsValid(MyTraceMeshComponent))
	{
		return;
	}

	UMaterialInterface* TraceMaterial = MyTraceMeshInvisible
		? MyNullMaterial.Get()
		: MyMIOutput.Get();
	TraceMaterial = MyDisableComponent ? MyInactiveGrayMaterial.Get() : TraceMaterial;
	if (IsValid(TraceMaterial))
	{
		MyTraceMeshComponent->SetMaterial(0, TraceMaterial);
	}
}

void UMyNinjaLiveComponent::MyApplyOutputMaterialsToTaggedActors()
{
	const int32 LastIndex = static_cast<int32>(MySecondaryMaterialsPresent) +
		static_cast<int32>(MyTertiaryMaterialsPresent);
	const TArray<FName> ActorTags = {
		MyApply1stOutMatToActorsWithTag,
		MyApply2ndOutMatToActorsWithTag,
		MyApply3rdOutMatToActorsWithTag };
	const TArray<FName> OutputComponentTags = {
		MyApply1stOutMatToComponentsWithTag,
		MyApply2ndOutMatToComponentsWithTag,
		MyApply3rdOutMatToComponentsWithTag };
	const TArray<UMaterialInstanceDynamic*> OutputMaterials = {
		MyMIOutput,
		MyMISecondaryOutput,
		MyMITertiaryOutput };
	AActor* LastActor = nullptr;
	int32 LastMaterialIndex = 0;

	for (int32 Index = 0; Index <= LastIndex; ++Index)
	{
		const FName ActorTag = ActorTags[Index];
		UMaterialInstanceDynamic* OutputMaterial = OutputMaterials[Index];
		if (ActorTag.IsNone())
		{
			continue;
		}

		TArray<AActor*> TargetActors;
		UGameplayStatics::GetAllActorsWithTag(this, ActorTag, TargetActors);
		if (!TargetActors.IsEmpty())
		{

			LastActor = TargetActors.Last();
			LastMaterialIndex = Index;
		}

		for (AActor* TargetActor : TargetActors)
		{
			if (!IsValid(TargetActor))
			{
				continue;
			}

			const FName ComponentTag = OutputComponentTags[Index];
			TArray<UActorComponent*> CandidateComponents;
			if (ComponentTag.IsNone())
			{
				TArray<UPrimitiveComponent*> PrimitiveComponents;
				TargetActor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
				for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
				{
					CandidateComponents.Add(PrimitiveComponent);
				}
			}
			else
			{
				CandidateComponents = TargetActor->GetComponentsByTag(UActorComponent::StaticClass(), ComponentTag);
			}

			for (UActorComponent* Component : CandidateComponents)
			{
				if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
				{
					PrimitiveComponent->SetMaterial(0, OutputMaterial);
				}
			}
		}
	}

	if (IsValid(LastActor))
	{
		if (UVolumetricCloudComponent* CloudComponent =
			LastActor->FindComponentByClass<UVolumetricCloudComponent>())
		{
			CloudComponent->SetMaterial(OutputMaterials[LastMaterialIndex]);
		}
	}
}

void UMyNinjaLiveComponent::MyCreateOutputMaterialAndSetItOnTargetsStep03()
{
	if (MyFeedTaggedActorNiagaraComponent.IsNone())
	{
		return;
	}

	TArray<AActor*> TargetActors;
	UGameplayStatics::GetAllActorsWithTag(this, MyFeedTaggedActorNiagaraComponent, TargetActors);
	if (TargetActors.IsEmpty())
	{
		return;
	}

	const FString VelocityDensityKey = MySimplePainterMode && !MyEnablePainterDoubleBuffering
		? TEXT("RT_Painter")
		: TEXT("RT_Composite");
	const TObjectPtr<UTextureRenderTarget2D>* VelocityDensityTarget = MyRenderTargetsMap.Find(VelocityDensityKey);
	const TObjectPtr<UTextureRenderTarget2D>* PressureTarget =
		MyRenderTargetsMap.Find(TEXT("RT_PressureDivergenceTemp"));
	const TObjectPtr<UTextureRenderTarget2D>* OutputTarget = MyRenderTargetsMap.Find(TEXT("RT_Output"));

	for (AActor* TargetActor : TargetActors)
	{
		if (!IsValid(TargetActor))
		{
			continue;
		}

		TArray<UNiagaraComponent*> NiagaraComponents;
		TargetActor->GetComponents<UNiagaraComponent>(NiagaraComponents);
		for (UNiagaraComponent* NiagaraComponent : NiagaraComponents)
		{
			if (!IsValid(NiagaraComponent))
			{
				continue;
			}

			UTextureRenderTarget2D* VelocityDensityTexture =
				VelocityDensityTarget ? VelocityDensityTarget->Get() : nullptr;
			UNiagaraFunctionLibrary::SetTextureObject(NiagaraComponent,
				TEXT("NinjaVelocityDensityBuffer"), VelocityDensityTexture);
			NiagaraComponent->SetVariableTexture(TEXT("User.NinjaVelocityDensityBufferRaw"), VelocityDensityTexture);

			if (MyMakePressureAvailableForNiagara)
			{
				UTextureRenderTarget2D* PressureTexture = PressureTarget ? PressureTarget->Get() : nullptr;
				UNiagaraFunctionLibrary::SetTextureObject(NiagaraComponent,
					TEXT("NinjaPressureDivergenceBuffer"), PressureTexture);
				NiagaraComponent->SetVariableTexture(TEXT("User.NinjaPressureDivergenceBufferRaw"), PressureTexture);
			}

			if (MyMake1stOutputAvailableForNiagara)
			{
				UNiagaraFunctionLibrary::SetTextureObject(NiagaraComponent, TEXT("NinjaOutputBuffer"),
					OutputTarget ? OutputTarget->Get() : nullptr);
			}

			if (MyLWCSupport)
			{
				NiagaraComponent->SetVariablePosition(TEXT("TraceMeshPosDouble"), MyTraceMeshPos);
			}

			NiagaraComponent->SetVectorParameter(TEXT("TraceMeshPos"), MyTraceMeshPos);
			NiagaraComponent->SetVectorParameter(TEXT("TraceMeshSize"),
				IsValid(MyTraceMeshComponent) ? MyTraceMeshComponent->GetComponentScale() : FVector::ZeroVector);
			MyNiagaraSystemsToDrive.Add(NiagaraComponent);
			MyNiagaraSystemsPresent = true;

			if (MyForceMaxSamplingFPSToNiagara)
			{
				NiagaraComponent->SetForceSolo(true);
				NiagaraComponent->SetComponentTickInterval(1.0 / static_cast<double>(FMath::Max(MyMaxSamplingFPS, 1)));
				NiagaraComponent->ReinitializeSystem();
			}
		}
	}
}

void UMyNinjaLiveComponent::MyAfterCreateRT()
{
	MyNiagaraSystemsToDrive.Reset();
	MyNiagaraSystemsPresent = false;
	bMyExternalRenderTargetExportValidated = false;
	bMyExternalRenderTargetExportGateOpen = false;
	MyCreateDynamicMaterialInstances();
	MyBuildTraceExcludeList();
	MyManageContinuousInteractions();
	MyAlternativeInputsFedToCompositeDensityInput();
	MyCreateOutputMaterialAndSetItOnTargetsStep01();
	MyCreateOutputMaterialAndSetItOnTargetsStep02();
	MyCreateOutputMaterialAndSetItOnTargetsStep03();
	MyApplyPlatformCompatibilityOptions();

	MyInitPainterV2();
	MyInitDone = true;
	MyMaterialInstacesDone = true;
	if (IsValid(MyTraceMeshComponent))
	{
		MyTraceMeshComponent->SetVisibility(true, false);
	}

	MyApplyPresetAndInputTextures();
}

void UMyNinjaLiveComponent::MyBuildTraceExcludeList()
{
	MyNinjaLiveTraceExclude.Reset();
	MyNinjaLiveTraceExcludeRaw.Reset();
	MyTraceExcludeRefreshFrame = MAX_uint64;


	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	TArray<AActor*> SameClassActors;
	UGameplayStatics::GetAllActorsOfClass(this, Owner->GetClass(), SameClassActors);
	for (AActor* SameClassActor : SameClassActors)
	{
		if (IsValid(SameClassActor) && SameClassActor != Owner)
		{
			MyNinjaLiveTraceExclude.Add(SameClassActor);
		}
	}
	MyRefreshTraceExcludeActors();
}

void UMyNinjaLiveComponent::MyApplyPlatformCompatibilityOptions()
{
	if (!MySupressUE51TextureSmearing)
	{
		return;
	}

	UKismetSystemLibrary::ExecuteConsoleCommand(this, TEXT("r.TSR.ShadingRejection.Flickering 0"));
	UKismetSystemLibrary::ExecuteConsoleCommand(this, TEXT("r.TSR.ShadingRejection.Flickering.Period 0"));
}

void UMyNinjaLiveComponent::MyApplyPresetAndInputTextures()
{
	if (IsValid(MyDefaultPreset))
	{
		MyActualPreset = UKismetSystemLibrary::GetDisplayName(MyDefaultPreset);
		MyForceAutoLoadPreset = true;
	}

	UDataTable* LoadedDataTable = nullptr;
	FString LoadedDataTablePath;
	TMap<FString, double> PresetMap;
	UMyNinjaLiveFunctions::MyPresetLoader(
		this,
		MyActualPreset,
		MyPresetSearchPaths,
		MyPresetNameFilterCriteria,
		MyForceAutoLoadPreset && IsValid(MyDefaultPreset),
		MyDefaultPreset,
		LoadedDataTable,
		LoadedDataTablePath,
		PresetMap);
	MyLoadedDataTable = LoadedDataTable;
	MyLoadedDataTablePath = MoveTemp(LoadedDataTablePath);
	MyPresetMap = MoveTemp(PresetMap);
	UE_LOG(LogTemp, Display, TEXT("[FluidSim][Preset] Loaded preset='%s', data table='%s', path='%s', values=%d"),
		*MyActualPreset,
		*GetPathNameSafe(MyLoadedDataTable),
		*MyLoadedDataTablePath,
		MyPresetMap.Num());

	TArray<FString> PresetKeys;
	MyPresetMap.GetKeys(PresetKeys);
	PresetKeys.Sort();
	for (const FString& Key : PresetKeys)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[FluidSim][Preset] %s = %.17g"),
			*Key,
			MyPresetMap.FindRef(Key));
	}

	MyParsePresetMapAndSetVariables(MyPresetMap);
	MyLoadTextures();
}

void UMyNinjaLiveComponent::MyUpdateCollisionMaskIsNonDefault()
{

	MyCollisionMaskIsNonDefault = IsValid(MyCollisionMask) &&
		UKismetSystemLibrary::GetDisplayName(MyCollisionMask) != TEXT("T_maskframe_256");
}

void UMyNinjaLiveComponent::MyAlternativeInputsFedToCompositeDensityInput()
{

	if (IsValid(MyInputSceneCaptureCamera))
	{
		const TObjectPtr<UTextureRenderTarget2D>* DensityInputTarget =
			MyRenderTargetsMap.Find(TEXT("RT_DensityInputMaterial"));
		if (USceneCaptureComponent2D* CaptureComponent = MyInputSceneCaptureCamera->GetCaptureComponent2D())
		{
			CaptureComponent->TextureTarget = DensityInputTarget ? DensityInputTarget->Get() : nullptr;
		}
		MyUseInputMaterials = false;
	}

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(MyInputMediaLoopTimer);
	}


	if (!IsValid(MyInputMediaPlayer) || !IsValid(MyMediaTexture) || !IsValid(MyInputMediaSource))
	{
		return;
	}

	MyMediaTexture->SetMediaPlayer(MyInputMediaPlayer);
	MyInputMediaPlayer->OpenUrl(MyInputMediaSource->GetUrl());
	MyInputMediaPlayer->Play();

	if (World && MyInputMediaLoopLength > 0.0)
	{
		World->GetTimerManager().SetTimer(MyInputMediaLoopTimer, this,
			&UMyNinjaLiveComponent::MyRestartInputMedia, MyInputMediaLoopLength, true);
	}
}

void UMyNinjaLiveComponent::MyLoadVelocityInputTexture()
{
	if (IsValid(MyOverwritePresetVelocityInput))
	{
		MyVelocityInput = MyOverwritePresetVelocityInput;
		if (IsValid(MyMICompositeAndGradient))
		{
			MyMICompositeAndGradient->SetScalarParameterValue(TEXT("VeloInputSelect"), 1.0f);
			MyMICompositeAndGradient->SetTextureParameterValue(TEXT("VeloInputTexture"), MyVelocityInput);
		}
		return;
	}

	bool LoadFailed = false;
	UObject* LoadedTemplateObject = nullptr;
	FString LoadedTmpFullPath;
	FString LoadedTemplateNameOnly;
	bool UsesAbsolutePath = false;
	UMyNinjaLiveFunctions::MyTemplateLoader(
		this,
		TEXT("VelocityTemplate"),
		MyLoadedDataTable,
		MyLoadedDataTablePath,
		LoadFailed,
		LoadedTemplateObject,
		LoadedTmpFullPath,
		LoadedTemplateNameOnly,
		UsesAbsolutePath);

	if (LoadFailed)
	{
		MyVelocityInput = nullptr;
		if (IsValid(MyMICompositeAndGradient))
		{
			MyMICompositeAndGradient->SetScalarParameterValue(TEXT("VeloInputSelect"), 0.0f);
			MyMICompositeAndGradient->SetTextureParameterValue(TEXT("VeloInputTexture"), nullptr);
		}
		return;
	}

	UTexture2D* LoadedTexture = Cast<UTexture2D>(LoadedTemplateObject);
	if (!IsValid(LoadedTexture))
	{
		return;
	}

	MyVelocityInput = LoadedTexture;
	if (IsValid(MyMICompositeAndGradient))
	{
		MyMICompositeAndGradient->SetScalarParameterValue(TEXT("VeloInputSelect"), 1.0f);
		MyMICompositeAndGradient->SetTextureParameterValue(TEXT("VeloInputTexture"), MyVelocityInput);
	}
}

void UMyNinjaLiveComponent::MyLoadDensityInputTexture()
{

	if (MyUseRenderTargetAsInput)
	{
		return;
	}

	UMaterialInstanceDynamic* DensityInputMaterial = MySimplePainterMode
		? MyMICollisionPainterOffset.Get()
		: MyMICompositeAndGradient.Get();

	if (IsValid(MyOverwritePresetDensityInput))
	{
		MyDensityInput = MyOverwritePresetDensityInput;
		if (IsValid(DensityInputMaterial))
		{
			DensityInputMaterial->SetTextureParameterValue(TEXT("TextureAdd2"), MyDensityInput);
		}
		return;
	}

	bool LoadFailed = false;
	UObject* LoadedTemplateObject = nullptr;
	FString LoadedTmpFullPath;
	FString LoadedTemplateNameOnly;
	bool UsesAbsolutePath = false;
	UMyNinjaLiveFunctions::MyTemplateLoader(
		this,
		TEXT("DensityTemplate"),
		MyLoadedDataTable,
		MyLoadedDataTablePath,
		LoadFailed,
		LoadedTemplateObject,
		LoadedTmpFullPath,
		LoadedTemplateNameOnly,
		UsesAbsolutePath);

	if (LoadFailed)
	{
		MyDensityInput = nullptr;
		if (IsValid(DensityInputMaterial))
		{
			DensityInputMaterial->SetTextureParameterValue(TEXT("TextureAdd2"), nullptr);
		}

		if (const TObjectPtr<UTextureRenderTarget2D>* PainterTarget = MyRenderTargetsMap.Find(TEXT("RT_Painter")))
		{
			if (IsValid(PainterTarget->Get()))
			{
				UKismetRenderingLibrary::ClearRenderTarget2D(this, PainterTarget->Get(), FLinearColor::Black);
			}
		}
		return;
	}

	UTexture2D* LoadedTexture = Cast<UTexture2D>(LoadedTemplateObject);
	if (!IsValid(LoadedTexture))
	{
		return;
	}

	MyDensityInput = LoadedTexture;
	if (IsValid(DensityInputMaterial))
	{
		DensityInputMaterial->SetTextureParameterValue(TEXT("TextureAdd2"), MyDensityInput);
	}
}

void UMyNinjaLiveComponent::MyLoadTextures()
{
	if (MyMaterialInstacesDone)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(MyLoadTexturesTimer);
		}

		if (MySimplePainterMode)
		{
			MyLoadDensityInputTexture();
		}
		else
		{
			MyLoadVelocityInputTexture();
		}
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (!World->GetTimerManager().IsTimerActive(MyLoadTexturesTimer))
		{
			World->GetTimerManager().SetTimer(MyLoadTexturesTimer, this,
				&UMyNinjaLiveComponent::MyLoadTextures, 0.005f, false);
		}
	}
}

void UMyNinjaLiveComponent::MyRestartInputMedia()
{
	if (IsValid(MyInputMediaPlayer))
	{
		MyInputMediaPlayer->Rewind();
		MyInputMediaPlayer->Play();
	}
}

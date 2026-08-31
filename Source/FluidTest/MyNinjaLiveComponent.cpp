// MyNinjaLiveComponent.cpp — UMyNinjaLiveComponent 实现

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
#include "FluidTest/MyNinjaLiveFunctions.h"
#include "MyNinjaLiveMemoryPoolManager.h"
#include "TimerManager.h"

UMyNinjaLiveComponent::UMyNinjaLiveComponent()
{
	// 不启用 Tick（蓝图侧若需要可在蓝图里自行开启）
	PrimaryComponentTick.bCanEverTick = false;
	MyRenderTargetsList = {
		TEXT("RT_Composite"),
		TEXT("RT_Advection"),
		TEXT("RT_Painter"),
		TEXT("RT_PressureDivergence"),
		TEXT("RT_PressureDivergenceTemp"),
		TEXT("RT_DensityInputMaterial"),
		TEXT("RT_Output")
	};
}

void UMyNinjaLiveComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MyTimerCheckReady);
		World->GetTimerManager().SetTimer(MyTimerCheckReady, this,
			&UMyNinjaLiveComponent::MyCheckReady, 0.2f, true, 0.0f);
	}
}

void UMyNinjaLiveComponent::MyCheckReady()
{
	if (!IsValid(MyTraceMeshComponent))
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MyTimerCheckReady);
	}
	MyTimerCheckReady.Invalidate();
	MyComponentRePlayEvent.AddDynamic(this, &UMyNinjaLiveComponent::MyRePlay);
	MyProximityActivationMasterVarsQuantizerOutMat();
	MyAfterBind();
}

void UMyNinjaLiveComponent::MyRePlay()
{
	MyResetTempArrays();
	MyProximityActivationMasterVarsQuantizerOutMatFromOwner();
	MyAfterBind();
}

bool UMyNinjaLiveComponent::MyAfterTickDelay(double DeltaSeconds)
{
	if (MyDisableComponent || MyTickBlocker)
	{
		return false;
	}

	const bool bShouldActivate = !MyComponentActivatedByPawnProximity || MyPawnInsideActivationBounds;
	if (bShouldActivate)
	{
		bool bCollisionTimerUpdated = false;
		AActor* OwnerActor = GetOwner();
		const bool bCanUpdateTimers = !MyPauseSimWhenNotVisible ||
			(IsValid(OwnerActor) && OwnerActor->WasRecentlyRendered(MyWaitBeforePause));
		if (MyInitDone && bCanUpdateTimers)
		{
			MyTimeSinceLastClick += DeltaSeconds;
			MyTimeSinceLastCollision += DeltaSeconds;
			bCollisionTimerUpdated = true;
		}

		// ExecutionSequence 的 then_1：DoOnce_3 仅在重新激活后重置 DoOnce_2 一次。
		if (!MyAfterTickDelayRearmDoOnceClosed)
		{
			MyAfterTickDelayRearmDoOnceClosed = true;
			MyAfterTickDelayDeactivateDoOnceClosed = false;
		}
		return bCollisionTimerUpdated;
	}

	// 未激活分支进入 DoOnce_2，故每次离开激活状态最多停用 Painter v2 一次。
	if (!MyAfterTickDelayDeactivateDoOnceClosed)
	{
		MyAfterTickDelayDeactivateDoOnceClosed = true;
		if (MyUsePAINTER_V2_ToTrackObjects && IsValid(MyNiagaraBasedPainter))
		{
			MyNiagaraBasedPainter->Deactivate();
			// Deactivate 的 then 引脚重置 DoOnce_3，允许下一次激活重新武装 DoOnce_2。
			MyAfterTickDelayRearmDoOnceClosed = false;
		}
	}

	return false;
}

void UMyNinjaLiveComponent::MySetAdditionalFluidsimParams()
{
	double VelocityX = 0.0;
	double VelocityY = 0.0;
	double VelocityZ = 0.0;
	MyVelocityHandlerForSimArea(-0.01, VelocityX, VelocityY, VelocityZ);

	if (IsValid(MyMICompositeAndGradient))
	{
		auto SetCompositeScalar = [this](FName ParameterName, double Value)
		{
			MyMICompositeAndGradient->SetScalarParameterValue(ParameterName, static_cast<float>(Value));
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
		MyMIDivergence->SetScalarParameterValue(TEXT("Divergence"), static_cast<float>(MyDivergence));
		MyMIDivergence->SetScalarParameterValue(TEXT("BrushPuncture"),
			static_cast<float>(MyBrushPuncture + VelocityZ));
	}
}

FVector UMyNinjaLiveComponent::MyCorrectExtremes(FVector DeltaPos, FVector Scale, FVector Composite) const
{
	const double MaxScaleElement = Scale.GetMax();
	const double HalfMaxScale = MaxScaleElement * 0.5;
	const double InverseHalfScale = FMath::Clamp(1.0 / HalfMaxScale, 0.0, 0.9);
	const double DeltaLength = FMath::Max(DeltaPos.Length(), 0.001);
	const double Correction = FMath::Clamp((1.0 - InverseHalfScale) * (MaxScaleElement / DeltaLength), 0.35, 1.0);
	return Composite * Correction;
}

void UMyNinjaLiveComponent::MyDynamicSimspeedAndWorldOffsetAdjustmentFinal()
{
	if (!IsValid(MyTraceMeshComponent))
	{
		return;
	}

	const FVector DeltaPos = MyTraceMeshPos - MyTraceMeshLastPos;
	const int32 Step = FMath::Max(MyQuantizerStepSize, 1);
	const FVector ComponentScale = MyTraceMeshComponent->GetComponentScale();
	const FVector ScaleForSimulation(ComponentScale.X, ComponentScale.Y, ComponentScale.X);
	FVector AdjustedDelta = DeltaPos;
	if (MyQuantizerStepSize >= -1)
	{
		const double StepAsDouble = static_cast<double>(Step);
		AdjustedDelta = (DeltaPos * (1.0 / (StepAsDouble * 100.0))) *
			(FVector::OneVector / ScaleForSimulation) * StepAsDouble;
	}

	if (MyQuantizerStepSize == -1)
	{
		AdjustedDelta = MyCorrectExtremes(DeltaPos, ComponentScale, AdjustedDelta);
	}

	const FVector LocalDelta = FTransform(MyTraceMeshComponent->GetComponentRotation().Quaternion())
		.InverseTransformVectorNoScale(AdjustedDelta);
	const double Multiplier = MyQuantizerStepSize == -3
		? 0.0
		: MyQuantizerStepSize == -2 ? MyOffsetFromSimAreaMotion * 0.001 : 1.0;
	const FVector FrameOffset = LocalDelta * Multiplier;
	MyTraceMeshDeltaPos += FrameOffset;

	const float FrameOffsetX = static_cast<float>(FrameOffset.X);
	const float FrameOffsetY = static_cast<float>(FrameOffset.Y);
	const float AccumulatedOffsetX = static_cast<float>(MyTraceMeshDeltaPos.X);
	const float AccumulatedOffsetY = static_cast<float>(MyTraceMeshDeltaPos.Y);
	const float BrushPuncture = static_cast<float>(MyBrushPuncture +
		FMath::Abs(FrameOffset.Z) * (MySimAreaMotionEffectsBrushPuncture * -20000.0));

	auto SetDeltaOffset = [FrameOffsetX, FrameOffsetY](UMaterialInstanceDynamic* Material)
	{
		if (!IsValid(Material))
		{
			return;
		}

		Material->SetScalarParameterValue(TEXT("WorldOffsetDeltaX"), FrameOffsetX);
		Material->SetScalarParameterValue(TEXT("WorldOffsetDeltaY"), FrameOffsetY);
	};
	auto SetWorldOffsetX = [AccumulatedOffsetX](UMaterialInstanceDynamic* Material)
	{
		if (!IsValid(Material))
		{
			return;
		}

		Material->SetScalarParameterValue(TEXT("WorldOffsetX"), AccumulatedOffsetX);
	};
	auto SetWorldOffsetY = [AccumulatedOffsetY](UMaterialInstanceDynamic* Material)
	{
		if (!IsValid(Material))
		{
			return;
		}

		Material->SetScalarParameterValue(TEXT("WorldOffsetY"), AccumulatedOffsetY);
	};

	if (MySimplePainterMode)
	{
		SetDeltaOffset(MyMICollisionPainterOffset);
		SetDeltaOffset(MyMICollisionPainterDot);
		SetDeltaOffset(MyQuantizerStepSize < 1 ? MyMINull.Get() : MyMICollisionPainterLine.Get());
		for (UMaterialInstanceDynamic* Material : {
			MyMICollisionPainterOffset.Get(), MyMICollisionPainterDot.Get(), MyMIOutput.Get() })
		{
			SetWorldOffsetX(Material);
			SetWorldOffsetY(Material);
		}
	}
	else
	{
		for (UMaterialInstanceDynamic* Material : {
			MyMICompositeAndGradient.Get(), MyMIDivergence.Get(), MyMIPressureCycle1.Get(),
			MyMIPressureCycle2.Get(), MyMICollisionPainterOffset.Get(), MyMICollisionPainterDot.Get(),
			MyQuantizerStepSize < 1 ? MyMINull.Get() : MyMICollisionPainterLine.Get() })
		{
			SetDeltaOffset(Material);
		}
		SetWorldOffsetX(MyMICompositeAndGradient);
		SetWorldOffsetX(MyMIOutput);
		for (UMaterialInstanceDynamic* Material : {
			MyMICompositeAndGradient.Get(), MyMICollisionPainterDot.Get(), MyMIOutput.Get() })
		{
			SetWorldOffsetY(Material);
		}
	}

	for (UMaterialInstanceDynamic* Material : {
		MyMIDivergence.Get(), MyMICollisionPainterLine.Get(), MyMICollisionPainterDot.Get() })
	{
		if (IsValid(Material))
		{
			Material->SetScalarParameterValue(TEXT("BrushPuncture"), BrushPuncture);
		}
	}
}

void UMyNinjaLiveComponent::MyDynamicSimspeedAndWorldOffsetAdjustment()
{
	if (!IsValid(MyTraceMeshComponent))
	{
		return;
	}

	const double BaseTexelSizeMultiplier = FMath::Max(
		static_cast<double>(UKismetMathLibrary::Divide_IntInt(MyMaxSamplingFPS, MySamplingFPS)) * 0.5, 1.0) * MySpeed;
	const double SingleTargetTexelSizeMultiplier =
		(MySpeedTemp * MySingleTargetModeSpeedInfluenceFactor_LEGACY +
			(1.0 - MySingleTargetModeSpeedInfluenceFactor_LEGACY)) * BaseTexelSizeMultiplier;
	const double SingleTargetMultiplier = MyHalfResPressureAndDivergenceBuffers
		? 1.0
		: SingleTargetTexelSizeMultiplier;
	auto SetTexelSizeMultiplier = [](UMaterialInstanceDynamic* Material, double Value)
	{
		if (IsValid(Material))
		{
			Material->SetScalarParameterValue(TEXT("TexelSizeMult"), static_cast<float>(Value));
		}
	};
	auto SetTexelSizeMultiplierOnSolverMaterials = [SetTexelSizeMultiplier, this](double Value)
	{
		for (UMaterialInstanceDynamic* Material : {
			MyMICompositeAndGradient.Get(), MyMIAdvection.Get(), MyMIDivergence.Get(),
			MyMIPressureCycle1.Get(), MyMIPressureCycle2.Get() })
		{
			SetTexelSizeMultiplier(Material, Value);
		}
	};

	if (!MySimplePainterMode)
	{
		if (MySingleTargetMode_LEGACY && MySingleTargetModeSetSimSpeed_LEGACY)
		{
			SetTexelSizeMultiplierOnSolverMaterials(SingleTargetMultiplier);
		}
		else if (!MySimSpeedAdjustmentPending)
		{
			MySimSpeedAdjustmentPending = true;
			const FTimerDelegate ApplyDelayedTexelSizeMultiplier = FTimerDelegate::CreateWeakLambda(this,
				[this, SetTexelSizeMultiplierOnSolverMaterials]()
				{
					MySimSpeedAdjustmentPending = false;
					const double CurrentMultiplier = MyHalfResPressureAndDivergenceBuffers
						? 1.0
						: FMath::Max(static_cast<double>(UKismetMathLibrary::Divide_IntInt(
							MyMaxSamplingFPS, MySamplingFPS)) * 0.5, 1.0) * MySpeed;
					SetTexelSizeMultiplierOnSolverMaterials(CurrentMultiplier);
				});

			if (UWorld* World = GetWorld())
			{
				if (MySimSpeedAdjustmentLatency <= 0.0)
				{
					World->GetTimerManager().SetTimerForNextTick(ApplyDelayedTexelSizeMultiplier);
				}
				else
				{
					World->GetTimerManager().SetTimer(MyTimerSimSpeedAdjustment, ApplyDelayedTexelSizeMultiplier,
						MySimSpeedAdjustmentLatency, false);
				}
			}
			else
			{
				MySimSpeedAdjustmentPending = false;
			}
		}
	}

	USceneComponent* AttachParent = MyTraceMeshComponent->GetAttachParent();
	const FVector CurrentParentPos = IsValid(AttachParent)
		? AttachParent->GetComponentLocation()
		: FVector::ZeroVector;
	const FVector RawTraceMeshPos = MyTraceMeshComponent->GetComponentLocation();

	if (!MyDynamicSimPositionInitialized)
	{
		MyDynamicSimPositionInitialized = true;
		MyTraceMeshPosInitialWorld = FVector(RawTraceMeshPos.X, RawTraceMeshPos.Y,
			MyForceTraceMeshToCustomVerticalPos ? MyForceTraceMeshVerticalPosition : RawTraceMeshPos.Z);
		MyTraceMeshPosInitialLocal = MyTraceMeshPosInitialWorld - CurrentParentPos;
		const double InitialQuantizerDivisor = static_cast<double>(FMath::Max(MyQuantizerStepSize, 1)) * 100.0;
		const FVector InitialScaledPosition =
			(MyTraceMeshPosInitialLocal + CurrentParentPos) / InitialQuantizerDivisor;
		MyTraceMeshPosInitialFractionalPart = FVector(
			FMath::Frac(InitialScaledPosition.X), FMath::Frac(InitialScaledPosition.Y),
			FMath::Frac(InitialScaledPosition.Z));

		AMyNinjaLiveActor* NinjaLive = nullptr;
		if (MyCheckComponentOwner(NinjaLive) && IsValid(NinjaLive))
		{
			MyInteractionVolume = NinjaLive->MyInteractionVolume;
			MyInteractionVolumeIsPresent = IsValid(MyInteractionVolume);
		}
		else
		{
			MyInteractionVolumeIsPresent = false;
		}

		if (MyQuantizerStepSize > 0 || MyMovementIsLockedOnThisAxis != EMyQuantizerAxisIgnore::None)
		{
			MyTraceMeshComponent->SetAbsolute(true, MyTraceMeshComponent->IsUsingAbsoluteRotation(),
				MyTraceMeshComponent->IsUsingAbsoluteScale());
			if (IsValid(MyInteractionVolume))
			{
				MyInteractionVolume->SetAbsolute(true, MyInteractionVolume->IsUsingAbsoluteRotation(),
					MyInteractionVolume->IsUsingAbsoluteScale());
			}
		}
	}

	if (!MyEnablePainterDoubleBuffering &&
		!MyTraceMeshPos.Equals(MyTraceMeshLastPos, 0.0001) &&
		!MyTraceMeshLastPos.Equals(FVector::ZeroVector, 0.0001))
	{
		MyInputFeedback = 0.0;
	}

	MyTraceMeshParentLastPos = MyTraceMeshParentPos;
	MyTraceMeshParentPos = CurrentParentPos;
	MyTraceMeshLastPos = MyTraceMeshPos;

	const int32 QuantizerStep = FMath::Max(MyQuantizerStepSize, 1);
	const double QuantizerDivisor = static_cast<double>(QuantizerStep) * 100.0;
	const FVector ScaledInitialPosition = (MyTraceMeshPosInitialLocal + MyTraceMeshParentPos) / QuantizerDivisor;
	const FVector ScaledPositionFraction(
		FMath::Frac(ScaledInitialPosition.X), FMath::Frac(ScaledInitialPosition.Y),
		FMath::Frac(ScaledInitialPosition.Z));
	FVector FractionToRemove;
	FVector InitialFractionToRestore;
	MyKillFracOnGivenAxis(ScaledPositionFraction, MyTraceMeshPosInitialFractionalPart,
		MyMovementNotQuantizedToStepsOnAxis, FractionToRemove, InitialFractionToRestore);
	const FVector QuantizedPosition =
		(ScaledInitialPosition - FractionToRemove + InitialFractionToRestore) * QuantizerDivisor;
	const bool bUseRawPosition = MyQuantizerStepSize < 1 &&
		MyMovementIsLockedOnThisAxis == EMyQuantizerAxisIgnore::None;
	MyTraceMeshPos = MyLockMovementOnGivenAxis(
		bUseRawPosition ? RawTraceMeshPos : QuantizedPosition, MyMovementIsLockedOnThisAxis);

	MyDynamicSimspeedAndWorldOffsetAdjustmentFinal();
}

FVector UMyNinjaLiveComponent::MyLockMovementOnGivenAxis(FVector Pos, EMyQuantizerAxisIgnore LockThisAxis) const
{
	switch (LockThisAxis)
	{
	case EMyQuantizerAxisIgnore::X:
		return FVector(MyTraceMeshPosInitialWorld.X, Pos.Y, Pos.Z);
	case EMyQuantizerAxisIgnore::Y:
		return FVector(Pos.X, MyTraceMeshPosInitialWorld.Y, Pos.Z);
	case EMyQuantizerAxisIgnore::Z:
		return FVector(Pos.X, Pos.Y, MyTraceMeshPosInitialWorld.Z);
	case EMyQuantizerAxisIgnore::All:
		return MyTraceMeshPosInitialWorld;
	case EMyQuantizerAxisIgnore::Camera:
	case EMyQuantizerAxisIgnore::None:
	default:
		return Pos;
	}
}

void UMyNinjaLiveComponent::MyKillFracOnGivenAxis(FVector Frac, FVector FracInit,
	EMyQuantizerAxisIgnore QuantizerIgnoresThisAxis, FVector& FracOut, FVector& FracInitOut) const
{
	const APlayerCameraManager* PlayerCameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const FVector CameraLocation = IsValid(PlayerCameraManager)
		? PlayerCameraManager->GetCameraLocation()
		: FVector::ZeroVector;
	const FVector LookAtTarget = Frac + MyTraceMeshPos;
	const FVector CameraForward = UKismetMathLibrary::GetForwardVector(
		UKismetMathLibrary::FindLookAtRotation(CameraLocation, LookAtTarget));
	const FVector CameraMask = FVector::OneVector - CameraForward.GetAbs();

	auto KillFracOnAxis = [&CameraMask, QuantizerIgnoresThisAxis](FVector Value)
	{
		switch (QuantizerIgnoresThisAxis)
		{
		case EMyQuantizerAxisIgnore::X:
			return Value * FVector(0.0, 1.0, 1.0);
		case EMyQuantizerAxisIgnore::Y:
			return Value * FVector(1.0, 0.0, 1.0);
		case EMyQuantizerAxisIgnore::Z:
			return Value * FVector(1.0, 1.0, 0.0);
		case EMyQuantizerAxisIgnore::Camera:
			return Value * CameraMask;
		case EMyQuantizerAxisIgnore::All:
			return Value * FVector::ZeroVector;
		case EMyQuantizerAxisIgnore::None:
		default:
			return Value;
		}
	};

	const FVector CameraFacingFrac = Frac * CameraMask;
	const FVector CameraFacingFracInit = FracInit * CameraMask;
	const FVector SelectedFrac = MyCameraFacingTraceMesh ? CameraFacingFrac : KillFracOnAxis(Frac);
	const FVector SelectedFracInit = MyCameraFacingTraceMesh ? CameraFacingFracInit : KillFracOnAxis(FracInit);
	const int32 QuantizerEnabled = MyQuantizerStepSize > 0 ? 1 : 0;
	FracOut = SelectedFrac * QuantizerEnabled;
	FracInitOut = SelectedFracInit * QuantizerEnabled;
}

void UMyNinjaLiveComponent::MyCoreFluidsimOPs(bool& ThenExec, bool& PainterV2Exec)
{
	// 简单画笔仅触发 PainterV2Exec；非简单画笔在压力循环完成后还会触发 ThenExec。
	ThenExec = false;
	PainterV2Exec = true;

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

	// 输入材质绘制在原图中发生于两个执行序列之前。
	if (MyUseInputMaterials && MyInputMaterials.IsValidIndex(MyInputMaterialSelected))
	{
		Draw(FindRenderTarget(TEXT("RT_DensityInputMaterial")), MyInputMaterials[MyInputMaterialSelected]);
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

		// 主输出材质始终写入位置；Secondary/Tertiary 是 Select 节点额外写入的目标。
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
		UTextureRenderTarget2D* Painter = FindRenderTarget(TEXT("RT_Painter"));
		UTextureRenderTarget2D* Composite = FindRenderTarget(TEXT("RT_Composite"));
		MyMICollisionPainterOffset->SetTextureParameterValue(TEXT("Texture"), Painter);
		Draw(Composite, MyMICollisionPainterOffset);
		MyMICollisionPainterOffset->SetScalarParameterValue(TEXT("WorldOffsetDeltaX"), 0.0f);
		MyMICollisionPainterOffset->SetScalarParameterValue(TEXT("WorldOffsetDeltaY"), 0.0f);
		MyMICollisionPainterOffset->SetTextureParameterValue(TEXT("Texture"), Composite);

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

		Draw(Painter, MyMICollisionPainterOffset);
		if (!MySimplePainterMode)
		{
			Draw(Composite, MyMICompositeAndGradient);
		}
	}
	else if (!MySimplePainterMode)
	{
		Draw(FindRenderTarget(TEXT("RT_Composite")), MyMICompositeAndGradient);
	}

	// 原图在压力求解前输出第一缓冲；简单画笔模式到此只会走 PainterV2Exec。
	if (MyMake1stOutputAvailableFor2ndOutput || MyMake1stOutputAvailableForNiagara)
	{
		Draw(FindRenderTarget(TEXT("RT_Output")), MyMIOutput);
	}

	if (!MySimplePainterMode)
	{
		Draw(FindRenderTarget(TEXT("RT_Advection")), MyMIAdvection);
		Draw(FindRenderTarget(TEXT("RT_PressureDivergence")), MyMIDivergence);

		const int32 Solver1Iterations = MyLOD1ReduceSimQuality
			? FMath::Min(MyFluidSolver1Iterations, MyPressureSolver1MaxIterations)
			: MyPressureSolver1MaxIterations;
		const int32 LastIteration = MyUsePressureSolver1DefaultIs2
			? FMath::Max(Solver1Iterations - 2, 0)
			: MyPressureSolver2MaxIterations - 1;
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

			Draw(FindRenderTarget(TEXT("RT_PressureDivergenceTemp")), MyMIPressureCycle1);
			if (IsValid(MyMIPressureCycle1))
			{
				MyMIPressureCycle1->SetScalarParameterValue(TEXT("KernelMult"), static_cast<float>(KernelMultiplier));
				MyMIPressureCycle1->SetScalarParameterValue(TEXT("WorldOffsetDeltaX"), 0.0f);
				MyMIPressureCycle1->SetScalarParameterValue(TEXT("WorldOffsetDeltaY"), 0.0f);
			}
			if (IsValid(MyMIPressureCycle2))
			{
				MyMIPressureCycle2->SetScalarParameterValue(TEXT("KeepDivergenceBuffer"),
					bLastIteration ? 0.0f : 1.0f);
				MyMIPressureCycle2->SetScalarParameterValue(TEXT("WorldOffsetDeltaX"), 0.0f);
				MyMIPressureCycle2->SetScalarParameterValue(TEXT("WorldOffsetDeltaY"), 0.0f);
			}
			Draw(FindRenderTarget(TEXT("RT_PressureDivergence")), MyMIPressureCycle2);
			if (IsValid(MyMIPressureCycle2))
			{
				MyMIPressureCycle2->SetScalarParameterValue(TEXT("KernelMult"), static_cast<float>(KernelMultiplier));
			}
		}

		ThenExec = true;
	}
}

void UMyNinjaLiveComponent::MyFluidCoreStep()
{
	MySetPosVelocityScaleArraysToPainterV2();

	// IfThenElse_16：简单画笔模式且未启用双缓冲时 then 分支无连接，直接结束。
	if (MySimplePainterMode && !MyEnablePainterDoubleBuffering)
	{
		return;
	}

	MyDynamicSimspeedAndWorldOffsetAdjustment();

	bool ThenExec = false;
	bool PainterV2Exec = false;
	MyCoreFluidsimOPs(ThenExec, PainterV2Exec);

	// ExecutionSequence_1 的 then_0：非简单画笔压力循环完成后补充附加流体参数。
	if (ThenExec)
	{
		MySetAdditionalFluidsimParams();

		// ExecutionSequence_25 的 then_0：光线追踪开启时执行光照处理。
		if (MyEnableRayMarching)
		{
			MyRaymarchBasedLightingOPs();
		}
		// ExecutionSequence_25 的 then_1：无条件绘制内部 RT 到外部 RT。
		MyDrawInternalRenderTargetToExternal();
	}

	// ExecutionSequence_1 的 then_1：Painter v2 模式下同步标量参数到 Niagara。
	if (PainterV2Exec)
	{
		MyForwardScalarParamsToNiagara();
	}
}

void UMyNinjaLiveComponent::MyMuteBrush()
{
	const double BrushActiveValue = (MyOverlap1 || MyMousePressed) ? 1.0 : 0.0;
	MyBrushStrengthTemp1 = (BrushActiveValue * MyBrushStrength) + 0.001;
}

bool UMyNinjaLiveComponent::MyBrushFadeOutTimer() const
{
	// Clamp(Feedback + Max(Feedback - 0.68, 0), 0, 1) 控制空闲衰减时间。
	const double FeedbackAboveThreshold = FMath::Max(MyInputFeedback - 0.68, 0.0);
	const double ClampedFeedback = FMath::Clamp(MyInputFeedback + FeedbackAboveThreshold, 0.0, 1.0);
	const double FadeTime = 1.0 - ClampedFeedback;
	const double IdleTime = FMath::Min(MyTimeSinceLastClick, MyTimeSinceLastCollision);
	const double MinimumWaitTime = FMath::Max(MyInputFeedback, 0.05);

	return MyStopUsingPainterCanvasWhenIdle && FadeTime * IdleTime > MinimumWaitTime;
}

void UMyNinjaLiveComponent::MySetBrushDensityParams1(double Value)
{
	FLinearColor Position = MyPosition2_2D;
	FLinearColor LastPosition = MyLastPosition2_2D;
	if (MyMousePass)
	{
		Position = MyPosition3_2D.IsValidIndex(MyTouchLookupIndex)
			? MyPosition3_2D[MyTouchLookupIndex]
			: FLinearColor::Black;
		LastPosition = MyLastPosition3_2D.IsValidIndex(MyTouchLookupIndex)
			? MyLastPosition3_2D[MyTouchLookupIndex]
			: FLinearColor::Black;
	}

	if (IsValid(MyMICollisionPainterLine))
	{
		MyMICollisionPainterLine->SetScalarParameterValue(TEXT("BrushSize"), static_cast<float>(Value));
		MyMICollisionPainterLine->SetScalarParameterValue(TEXT("BrushStrength"),
			static_cast<float>(FMath::Min(MyBrushStrengthTemp2, MyBrushStrengthTemp1)));
		MyMICollisionPainterLine->SetScalarParameterValue(TEXT("BrushHardness"),
			static_cast<float>(FMath::Min(MyBrushHardness, 1.0)));
		MyMICollisionPainterLine->SetVectorParameterValue(TEXT("Position"), Position);
		MyMICollisionPainterLine->SetVectorParameterValue(TEXT("LastPosition"), LastPosition);
		MyMICollisionPainterLine->SetScalarParameterValue(TEXT("BrushPuncture"), static_cast<float>(MyBrushPuncture));
		MyMICollisionPainterLine->SetScalarParameterValue(TEXT("BrushNoise"), static_cast<float>(MyBrushNoise));
	}

	if (!MySimplePainterMode && IsValid(MyMICompositeAndGradient))
	{
		MyMICompositeAndGradient->SetScalarParameterValue(TEXT("EraserSwitch"), MyEraserMode ? 1.0f : 0.0f);
	}
}

void UMyNinjaLiveComponent::MyPaintLine()
{
	MySetBrushDensityParams1(MyBrushSizeCoEff());
	MySingleTargetVelocity();

	const TObjectPtr<UTextureRenderTarget2D>* PainterRT = MyRenderTargetsMap.Find(TEXT("RT_Painter"));
	if (PainterRT && IsValid(PainterRT->Get()) && IsValid(MyMICollisionPainterLine))
	{
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, PainterRT->Get(), MyMICollisionPainterLine);
	}

	for (UMaterialInstanceDynamic* PainterMaterial : {
		MyMICollisionPainterLine.Get(), MyMICollisionPainterDot.Get() })
	{
		if (IsValid(PainterMaterial))
		{
			PainterMaterial->SetScalarParameterValue(TEXT("Multitarget"), 1.0f);
		}
	}
}

void UMyNinjaLiveComponent::MySetBrushDensityParams3(double Value)
{
	if (!IsValid(MyMICollisionPainterDot))
	{
		return;
	}

	MyMICollisionPainterDot->SetScalarParameterValue(TEXT("BrushSize"), static_cast<float>(Value));
	MyMICollisionPainterDot->SetScalarParameterValue(TEXT("BrushStrength"),
		static_cast<float>(FMath::Min(MyBrushStrengthTemp2, MyBrushStrengthTemp1)));
	MyMICollisionPainterDot->SetScalarParameterValue(TEXT("BrushHardness"),
		static_cast<float>(FMath::Min(MyBrushHardness, 1.0)));
	MyMICollisionPainterDot->SetVectorParameterValue(TEXT("Position"), MyPosition1_2D);
	MyMICollisionPainterDot->SetScalarParameterValue(TEXT("BrushPuncture"), static_cast<float>(MyBrushPuncture));
	MyMICollisionPainterDot->SetScalarParameterValue(TEXT("BrushNoise"), static_cast<float>(MyBrushNoise));

	if (!MySimplePainterMode && IsValid(MyMICompositeAndGradient))
	{
		MyMICompositeAndGradient->SetScalarParameterValue(TEXT("EraserSwitch"), MyEraserMode ? 1.0f : 0.0f);
	}
}

bool UMyNinjaLiveComponent::MyBrushSwitch2(FLinearColor InLinearColor) const
{
	// 画笔位置是否落在画布边缘（R/G 通道接近 0 或 1）。
	const auto IsAtCanvasEdge = [](const FLinearColor& Color)
	{
		return Color.R < 0.05 || Color.R > 0.95 || Color.G < 0.05 || Color.G > 0.95;
	};

	return MyPosition1_3D_Static
		|| !(MyContinuousInteractionWithOwnerActor || MyOverlap1)
		|| IsAtCanvasEdge(InLinearColor)
		|| IsAtCanvasEdge(MyLastPosition2_2D);
}

FLinearColor UMyNinjaLiveComponent::MyBrushRnd3(const FLinearColor InColor) const
{
	if (MyPV2_GenerateVelocity)
	{
		return InColor;
	}

	// R/G 通道独立地加 [-0.5*MyBrushRnd, +0.5*MyBrushRnd] 内的随机抖动。
	const double HalfRange = MyBrushRnd * 0.5;
	const FLinearColor Randomized(
		InColor.R + FMath::FRandRange(-HalfRange, HalfRange),
		InColor.G + FMath::FRandRange(-HalfRange, HalfRange),
		InColor.B,
		InColor.A);

	return Randomized;
}

FLinearColor UMyNinjaLiveComponent::MyBrushRnd2(const FLinearColor InColor) const
{
	if (MyPV2_GenerateVelocity)
	{
		return InColor;
	}

	// 对应 BrushRnd2：R/G 通道独立地加 [-0.5*MyBrushRnd, +0.5*MyBrushRnd] 内的随机抖动，B/A 保持不变。
	const double HalfRange = MyBrushRnd * 0.5;
	const FLinearColor Randomized(
		InColor.R + FMath::FRandRange(-HalfRange, HalfRange),
		InColor.G + FMath::FRandRange(-HalfRange, HalfRange),
		InColor.B,
		InColor.A);

	return Randomized;
}

void UMyNinjaLiveComponent::MyCameraFacing()
{
	if (!MyCameraFacingTraceMesh)
	{
		return;
	}

	UMyNinjaLiveFunctions::MyCameraFacing(
		this,
		MyTraceMeshComponent.Get(),
		MyUseLegacyCameraFacing,
		MyCameraFacingLockYAxis,
		MyTraceMeshInitialRotation);
}

void UMyNinjaLiveComponent::MyResetTempArrays()
{
	// 清空 TempArray0~39
	MyTempArray0.Empty();
	MyTempArray1.Empty();
	MyTempArray2.Empty();
	MyTempArray3.Empty();
	MyTempArray4.Empty();
	MyTempArray5.Empty();
	MyTempArray6.Empty();
	MyTempArray7.Empty();
	MyTempArray8.Empty();
	MyTempArray9.Empty();
	MyTempArray10.Empty();
	MyTempArray11.Empty();
	MyTempArray12.Empty();
	MyTempArray13.Empty();
	MyTempArray14.Empty();
	MyTempArray15.Empty();
	MyTempArray16.Empty();
	MyTempArray17.Empty();
	MyTempArray18.Empty();
	MyTempArray19.Empty();
	MyTempArray20.Empty();
	MyTempArray21.Empty();
	MyTempArray22.Empty();
	MyTempArray23.Empty();
	MyTempArray24.Empty();
	MyTempArray25.Empty();
	MyTempArray26.Empty();
	MyTempArray27.Empty();
	MyTempArray28.Empty();
	MyTempArray29.Empty();
	MyTempArray30.Empty();
	MyTempArray31.Empty();
	MyTempArray32.Empty();
	MyTempArray33.Empty();
	MyTempArray34.Empty();
	MyTempArray35.Empty();
	MyTempArray36.Empty();
	MyTempArray37.Empty();
	MyTempArray38.Empty();
	MyTempArray39.Empty();
}

TArray<FName>& UMyNinjaLiveComponent::MyGetTempArray(int32 Index)
{
	// 按 Index 返回 TempArray0~39 中对应数组；越界返回静态空数组
	switch (Index)
	{
	case 0:  return MyTempArray0;
	case 1:  return MyTempArray1;
	case 2:  return MyTempArray2;
	case 3:  return MyTempArray3;
	case 4:  return MyTempArray4;
	case 5:  return MyTempArray5;
	case 6:  return MyTempArray6;
	case 7:  return MyTempArray7;
	case 8:  return MyTempArray8;
	case 9:  return MyTempArray9;
	case 10: return MyTempArray10;
	case 11: return MyTempArray11;
	case 12: return MyTempArray12;
	case 13: return MyTempArray13;
	case 14: return MyTempArray14;
	case 15: return MyTempArray15;
	case 16: return MyTempArray16;
	case 17: return MyTempArray17;
	case 18: return MyTempArray18;
	case 19: return MyTempArray19;
	case 20: return MyTempArray20;
	case 21: return MyTempArray21;
	case 22: return MyTempArray22;
	case 23: return MyTempArray23;
	case 24: return MyTempArray24;
	case 25: return MyTempArray25;
	case 26: return MyTempArray26;
	case 27: return MyTempArray27;
	case 28: return MyTempArray28;
	case 29: return MyTempArray29;
	case 30: return MyTempArray30;
	case 31: return MyTempArray31;
	case 32: return MyTempArray32;
	case 33: return MyTempArray33;
	case 34: return MyTempArray34;
	case 35: return MyTempArray35;
	case 36: return MyTempArray36;
	case 37: return MyTempArray37;
	case 38: return MyTempArray38;
	case 39: return MyTempArray39;
	default:
	{
		static TArray<FName> EmptyArray;
		return EmptyArray;
	}
	}
}

void UMyNinjaLiveComponent::MyAddToTempArray(int32 ArrayIndex, FName Item)
{
	MyGetTempArray(ArrayIndex).Add(Item);
}

void UMyNinjaLiveComponent::MyClearTempArray(int32 ArrayIndex)
{
	MyGetTempArray(ArrayIndex).Empty();
}

void UMyNinjaLiveComponent::MyAppendToTempArray(int32 ArrayIndex, const TArray<FName>& Items)
{
	MyGetTempArray(ArrayIndex).Append(Items);
}

void UMyNinjaLiveComponent::MyCompareMapLength(int32 FirstIndex, int32 LastIndex, int32& MapLength, bool& Equal, int32& Added) const
{
	MapLength = MyRenderTargetsMap.Num();
	Added = (LastIndex + 1) - FirstIndex;
	const int32 Tmp = Added + MyMapLengthTmp;
	Equal = (Tmp == MapLength);
}

void UMyNinjaLiveComponent::MyParsePresetMapAndSetVariables(const TMap<FString, double>& PresetMap)
{
	auto GetValue = [&PresetMap](const TCHAR* Key)
	{
		return PresetMap.FindRef(Key);
	};
	auto GetValueOr = [&PresetMap](const TCHAR* Key, double DefaultValue)
	{
		if (const double* Value = PresetMap.Find(Key))
		{
			return *Value;
		}
		return DefaultValue;
	};

	MySpeed = GetValue(TEXT("Speed"));
	MyVeloOffsetX = GetValue(TEXT("VeloOffsetX"));
	MyVeloOffsetY = GetValue(TEXT("VeloOffsetY"));
	MyVeloFromBrushMotion = GetValue(TEXT("VeloFromBrushMotion"));
	MyOffsetFromSimAreaMotion = GetValueOr(TEXT("OffsetFromSimAreaMotion"), 1.0);
	MyVeloFromSimAreaMotion = GetValue(TEXT("VeloFromSimAreaMotion"));
	MyVeloStrength = GetValue(TEXT("VeloStrength"));
	MyVeloRotate = GetValue(TEXT("VeloRotate"));
	MyVeloAmpNoise = GetValue(TEXT("VeloAmpNoise"));
	MyVeloDirNoise = GetValue(TEXT("VeloDirNoise"));
	MyInputFeedback = GetValue(TEXT("InputFeedback"));
	MyFlowFeedback = GetValue(TEXT("FlowFeedback"));
	MyDivergence = GetValue(TEXT("Divergence"));
	MyBrushSize = GetValue(TEXT("BrushSize"));
	MyBrushStrength = GetValue(TEXT("BrushStrength"));
	MyBrushHardness = GetValue(TEXT("BrushHardness"));
	MyBrushPuncture = GetValue(TEXT("BrushPuncture"));
	MyEraserMode = GetValue(TEXT("EraserMode")) > 0.0;
	MyDensityTxtMult = GetValueOr(TEXT("DensityTxtMult"), 1.0);
	MyFadeDensityAtSimEdge = GetValue(TEXT("FadeDensityAtSimEdge"));
	MySimEdgeBouncyness = GetValueOr(TEXT("SimEdgeBouncyness"), 0.5);
	MyVeloDirNoiseSize = GetValueOr(TEXT("VeloDirNoiseSize"), 1.0);
	MyVeloDirNoiseSpeed = GetValueOr(TEXT("VeloDirNoiseSpeed"), 1.0);
	MyEdgeMaskWidth = GetValueOr(TEXT("EdgeMaskWidth"), 0.25);

	if (!MyUseRenderTargetAsInput)
	{
		MyDensityTxtScale = GetValue(TEXT("DensityTxtScale"));
		MyDensityTxtOffsetX = GetValue(TEXT("DensityTxtOffsetX"));
		MyDensityTxtOffsetY = GetValue(TEXT("DensityTxtOffsetY"));
	}

	MyBrushNoise = GetValue(TEXT("BrushNoise"));
	MyVeloInputTile = GetValue(TEXT("VeloInputTile"));
	MyVeloInputOffsetSpeed = GetValue(TEXT("VeloInputOffsetSpeed"));
	MyDensityInputNoiseAmp = GetValue(TEXT("DensityInputNoiseAmp"));
	MyDensityInputNoiseOffset = GetValue(TEXT("DensityInputNoiseOffset"));
	MyDensityInputNoiseTile = GetValue(TEXT("DensityInputNoiseTile"));
	MyBrushRnd = GetValue(TEXT("BrushRnd"));
}

void UMyNinjaLiveComponent::MyVelocityHandlerForSimArea(double CoEff, double& X, double& Y, double& Z) const
{
	// VeloFromSimAreaMotion 非零时：TraceMesh 前后帧位移 ×20 → 局部方向 × (速度×系数)
	FVector Velocity = FVector::ZeroVector;

	if (MyVeloFromSimAreaMotion != 0.0)
	{
		const FVector WorldDir = (MyTraceMeshParentPos - MyTraceMeshParentLastPos) * 20.0;

		const FTransform TraceMeshTransform(MyTraceMeshComponent
			? MyTraceMeshComponent->GetComponentRotation()
			: FRotator::ZeroRotator);

		const FVector LocalDir = TraceMeshTransform.InverseTransformVectorNoScale(WorldDir);

		Velocity = LocalDir * (MyVeloFromSimAreaMotion * CoEff);
	}

	X = Velocity.X;
	Y = Velocity.Y * -1.0;   // Y 取反
	Z = Velocity.Z;
}

bool UMyNinjaLiveComponent::MyCheckComponentOwner(AMyNinjaLiveActor*& AsNinjaLive) const
{
	// Owner 是否为 NinjaLive 类（用 C++ 父类判断，蓝图类继承自它）
	AsNinjaLive = nullptr;

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->IsA<AMyNinjaLiveActor>())
	{
		return false;
	}

	AsNinjaLive = Cast<AMyNinjaLiveActor>(OwnerActor);
	return true;
}

void UMyNinjaLiveComponent::MyEnableOwnerInput()
{
	// 输入方式为"无输入"时不处理
	if (MyUserInputBasedInteraction == EMyUserInput::None)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	// Owner 必须是 NinjaLive 类
	if (!OwnerActor->IsA<AMyNinjaLiveActor>())
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	OwnerActor->EnableInput(PC);

	if (MyShowMouseCursor)
	{
		PC->bShowMouseCursor = true;
	}
}

void UMyNinjaLiveComponent::MyCheckTouchOptions()
{
	if (MyCheckTouchOptionsDoOnceClosed)
	{
		return;
	}

	MyCheckTouchOptionsDoOnceClosed = true;
	MySingleInput = MyUserInputBasedInteraction == EMyUserInput::MouseSingle ||
		MyUserInputBasedInteraction == EMyUserInput::TouchSingle;
	MyTouch = MyUserInputBasedInteraction == EMyUserInput::TouchSingle ||
		MyUserInputBasedInteraction == EMyUserInput::TouchMultiple;
	MyTouchLookupIndex = 0;
}

int32 UMyNinjaLiveComponent::MyQuantizerValues(EMyQuantizerMode InQuantizerMode) const
{
	switch (InQuantizerMode)
	{
	case EMyQuantizerMode::NoQuantizerNoTextureOffset:						return -3;
	case EMyQuantizerMode::NoQuantizerTextureOffsetManuallySet:			return -2;
	case EMyQuantizerMode::NoQuantizerTextureOffsetAutomaticExtremesCorrected:	return -1;
	case EMyQuantizerMode::NoQuantizerTextureOffsetAutomatic:				return 0;
	case EMyQuantizerMode::Step1mTextureOffsetAutomatic:					return 1;
	case EMyQuantizerMode::Step2mTextureOffsetAutomatic:					return 2;
	case EMyQuantizerMode::Step3mTextureOffsetAutomatic:					return 3;
	case EMyQuantizerMode::Step4mTextureOffsetAutomatic:					return 4;
	case EMyQuantizerMode::Step5mTextureOffsetAutomatic:					return 5;
	case EMyQuantizerMode::Step10mTextureOffsetAutomatic:					return 10;
	case EMyQuantizerMode::Step20mTextureOffsetAutomatic:					return 20;
	case EMyQuantizerMode::Step30mTextureOffsetAutomatic:					return 30;
	case EMyQuantizerMode::Step50mTextureOffsetAutomatic:					return 50;
	case EMyQuantizerMode::Step100mTextureOffsetAutomatic:					return 100;
	case EMyQuantizerMode::Step500mTextureOffsetAutomatic:					return 500;
	default:																return 0;
	}
}

void UMyNinjaLiveComponent::MySingleTargetVelocity()
{
	FLinearColor CurrentPosition = MyPosition2_2D;
	FLinearColor PreviousPosition = MyLastPosition2_2D;
	if (MyMousePass && MyPosition3_2D.IsValidIndex(MyTouchLookupIndex) &&
		MyLastPosition3_2D.IsValidIndex(MyTouchLookupIndex))
	{
		CurrentPosition = MyPosition3_2D[MyTouchLookupIndex];
		PreviousPosition = MyLastPosition3_2D[MyTouchLookupIndex];
	}

	// 原节点将位置色转换为向量、相减后乘以 15，并交由材质侧做范围限制。
	const FVector Velocity = (FVector(CurrentPosition.R, CurrentPosition.G, CurrentPosition.B) -
		FVector(PreviousPosition.R, PreviousPosition.G, PreviousPosition.B)) * 15.0;
	if (IsValid(MyMICollisionPainterLine))
	{
		MyMICollisionPainterLine->SetVectorParameterValue(TEXT("Velocity"), FLinearColor(Velocity));
	}

	if (MySingleTargetMode_LEGACY && MySingleTargetModeSetSimSpeed_LEGACY)
	{
		MySpeedTemp = Velocity.Length();
	}
}

void UMyNinjaLiveComponent::MyMultiObjectVelocity(FLinearColor& Velocity)
{
	Velocity = FLinearColor::Black;

	UPrimitiveComponent* OverlappingPrimitive = MyPosDataType == 1
		? MyOverlappingSkeletalMesh.Get()
		: MyOverlappingComponent.Get();
	if (!IsValid(OverlappingPrimitive) || !IsValid(MyTraceMeshComponent) || !IsValid(MyMICollisionPainterDot))
	{
		return;
	}

	const FVector PhysicsVelocity = OverlappingPrimitive->GetPhysicsLinearVelocity(MyOverlappingBone);
	const bool bUseOwnerVelocity = PhysicsVelocity == FVector::ZeroVector && MyPosDataType == 1;
	const AActor* OwnerActor = GetOwner();
	const FVector SourceVelocity = bUseOwnerVelocity && IsValid(OwnerActor)
		? OwnerActor->GetVelocity()
		: PhysicsVelocity;

	const FVector ScaledVelocity = SourceVelocity * 0.0005;
	const FTransform TraceMeshTransform(MyTraceMeshComponent->GetComponentRotation());
	const FVector LocalVelocity = TraceMeshTransform.InverseTransformVectorNoScale(ScaledVelocity);
	const FVector ClampedVelocity = UKismetMathLibrary::ClampVectorSize(
		LocalVelocity, -MyBrushVelocityClamp, MyBrushVelocityClamp);
	const FLinearColor VelocityColor(ClampedVelocity);

	const bool bIsStaticMesh = IsValid(MyOverlappingComponent) &&
		MyOverlappingComponent->GetClass() == UStaticMeshComponent::StaticClass();
	const float StaticMeshDampen = static_cast<float>(bIsStaticMesh ? 1 - MyPosDataType : 0) * 0.001f;
	const FLinearColor StaticMeshOffset(StaticMeshDampen, StaticMeshDampen, StaticMeshDampen, 1.0f);
	const FLinearColor FinalVelocity = MyDampenIgnoresStaticMeshes
		? VelocityColor + StaticMeshOffset
		: VelocityColor;

	Velocity = FinalVelocity;
	MyMICollisionPainterDot->SetVectorParameterValue(TEXT("Velocity"), FinalVelocity);
}

void UMyNinjaLiveComponent::MyProximityActivationMasterVarsQuantizerOutMat()
{
	// In2 路径（不检查 Owner）：
	// 量化与 CameraFacing 冲突时强制关闭量化（Quantizer 与 CameraFacing 不兼容）
	if ((int32)MyTraceMeshMovingInWorldSpace > 3 && MyCameraFacingTraceMesh)
	{
		MyTraceMeshMovingInWorldSpace = EMyQuantizerMode::NoQuantizerTextureOffsetAutomatic;
	}

	// UsePAINTER_V2 且非 SingleTargetMode_LEGACY 时启用画笔双缓冲
	if (MyUsePAINTER_V2_ToTrackObjects && !MySingleTargetMode_LEGACY)
	{
		MyEnablePainterDoubleBuffering = true;
	}

	// MasterVars 初始化
	MyInitDone = false;
	MyMaterialInstacesDone = false;

	// 预设名过滤条件设为 NinjaLive（蓝图默认值）
	MyPresetNameFilterCriteria = FName(TEXT("NinjaLive"));

	// UE5 EA 版本检测（GetEngineVersion Contains "EarlyAccess" → NOT）
	MyUE5EAFLAG = !FEngineVersion::Current().ToString().Contains(TEXT("EarlyAccess"));

	// QuantizerStepSize = MyQuantizerValues(TraceMeshMovingInWorldSpace)
	MyQuantizerStepSize = MyQuantizerValues(MyTraceMeshMovingInWorldSpace);

	// OutMat 数组为空时补 M_Null 占位材质
	if (MyOutputMaterials.Num() == 0)
	{
		// 蓝图行为：添加空占位，后续由其他逻辑填充实际材质
		MyOutputMaterials.Add(nullptr);
	}
}

void UMyNinjaLiveComponent::MyProximityActivationMasterVarsQuantizerOutMatFromOwner()
{
	// In1 路径（含 CheckComponentOwner）：
	//   Owner 是 NinjaLive 类 → 从 Owner 同步 Disable/Proximity 设置；
	//   Owner 不是 NinjaLive 类 → 与 In2 汇合（直接初始化）
	AMyNinjaLiveActor* NinjaLive = nullptr;
	if (MyCheckComponentOwner(NinjaLive))
	{
		// 从 Owner 同步激活设置（对应蓝图 VariableSet_23/20）
		MyDisableComponent = NinjaLive->MyDisableBlueprint;
		MyComponentActivatedByPawnProximity = NinjaLive->MySimActivatedByPawnProximity;

		// Disable=true：不初始化（蓝图 then 分支空）
		if (MyDisableComponent)
		{
			return;
		}
		// Proximity=true：量化修正 + 双缓冲 + 抑制 BeginPlay（等 Pawn 靠近再激活）
		if (MyComponentActivatedByPawnProximity)
		{
			if ((int32)MyTraceMeshMovingInWorldSpace > 3 && MyCameraFacingTraceMesh)
			{
				MyTraceMeshMovingInWorldSpace = EMyQuantizerMode::NoQuantizerTextureOffsetAutomatic;
			}
			if (MyUsePAINTER_V2_ToTrackObjects && !MySingleTargetMode_LEGACY)
			{
				MyEnablePainterDoubleBuffering = true;
			}
			MyBeginPlaySupressed = true;
			return;
		}
	}
	// Owner 非 NinjaLive 或 Proximity=false：走完整初始化
	MyProximityActivationMasterVarsQuantizerOutMat();
}



void UMyNinjaLiveComponent::MyLightDirectionProviderCheck()
{
	// LightDirectionProviderCheck 复合节点：
	// EnableRayMarching 关闭时直接跳过
	if (!MyEnableRayMarching)
	{
		return;
	}

	// LightDirectionProvider 已有效时跳过初始化
	if (IsValid(MyLightDirectionProvider))
	{
		return;
	}

	// 无效：从 Owner 初始化并提供默认太阳参数
	AActor* OwnerActor = GetOwner();
	if (OwnerActor)
	{
		MyLightDirectionProvider = OwnerActor;
	}

	MyLightDirectionSourceIsRotation_NOT_Pos = true;
	MySunLatitude = 1000.0;
	MySunLongitude = 1000.0;
	MySunHeight = 5000.0;
	MyForceManualSunPosition = true;

}

void UMyNinjaLiveComponent::MyRaymarchBasedLightingOPs()
{
	// RaymarchBasedLightingOPs 复合节点：计算面朝度、光照方向/位置并写入输出材质。
	if (!IsValid(MyMIOutput) || !IsValid(MyTraceMeshComponent.Get()))
	{
		return;
	}

	// Facing = Dot(UpVector(TraceMesh), Normalize(TraceMeshPos - CameraPos)) * -1
	FVector CameraPos = FVector::ZeroVector;
	if (const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		CameraPos = CameraManager->GetCameraLocation();
	}
	const FVector TraceMeshPos = MyTraceMeshComponent->GetComponentLocation();
	const FVector ViewNormal = (TraceMeshPos - CameraPos).GetSafeNormal();

	// 光照方向（SelectVector）：旋转模式取 -Forward(Provider) 转 TraceMesh 局部并归一化周期；位置模式取 ProviderLoc-TraceMeshLoc 转局部。
	FVector LightDir = FVector::ZeroVector;
	if (IsValid(MyLightDirectionProvider))
	{
		const FRotator TraceMeshRot = MyTraceMeshComponent->GetComponentRotation();
		if (MyLightDirectionSourceIsRotation_NOT_Pos)
		{
			const FVector RevForward = -MyLightDirectionProvider->GetActorForwardVector();
			FVector Periodic = UKismetMathLibrary::LessLess_VectorRotator(RevForward, TraceMeshRot) * 0.5 + 0.5;
			Periodic.X = FMath::Fmod(Periodic.X + 1.0, 1.0);
			Periodic.Y = FMath::Fmod(Periodic.Y + 1.0, 1.0);
			Periodic.Z = FMath::Fmod(Periodic.Z + 1.0, 1.0);
			Periodic = (Periodic - 0.5) * 2.0;
			LightDir = Periodic;
		}
		else
		{
			const FVector DirToProvider = MyLightDirectionProvider->K2_GetActorLocation() - TraceMeshPos;
			LightDir = UKismetMathLibrary::LessLess_VectorRotator(DirToProvider, TraceMeshRot);
		}
	}

	// 双面遮罩：TwoSidedShading 时用 Lerp(-1,1, Max(Facing,0)^TwoSideBlendPow)，否则恒 1。
	const double BlendAlpha = FMath::Pow(FMath::Max(MyFacing, 0.0), MyTwoSideBlendPow);
	const double ZMask = MyTwoSidedShading
		? FMath::Lerp(-1.0, 1.0, BlendAlpha)
		: 1.0;
	const FLinearColor LightDirectionColor(FVector(LightDir.X * 1.0, LightDir.Y * 1.0, LightDir.Z * ZMask));

	// LightingPosition = ((ProviderLoc - TraceMeshLoc) * (PointLightMovementMultiplier*0.01/MaxScale) + OffsetLightVector) 转 TraceMesh 局部。
	FLinearColor LightingPositionColor = FLinearColor::Black;
	if (IsValid(MyLightDirectionProvider))
	{
		const FVector ProviderLoc = MyLightDirectionProvider->K2_GetActorLocation();
		const FVector Scale = MyTraceMeshComponent->GetComponentScale();
		const double MaxScale = FMath::Max(Scale.X, FMath::Max(Scale.Y, Scale.Z));
		const double MoveScale = MaxScale != 0.0 ? (MyPointLightMovementMultiplier * 0.01) / MaxScale : 0.0;
		const FVector OffsetPos = (ProviderLoc - TraceMeshPos) * MoveScale + MyOffsetLightVector;
		const FVector LocalPos = UKismetMathLibrary::LessLess_VectorRotator(
			OffsetPos, MyTraceMeshComponent->GetComponentRotation());
		LightingPositionColor = FLinearColor(LocalPos);
	}

	// 写入材质：EnableRayMarching 时才写 LightingDirection；其余参数无条件写。
	if (MyEnableRayMarching)
	{
		const FLinearColor ManualSunColor(FVector(MySunLatitude, MySunLongitude, MySunHeight));
		MyMIOutput->SetVectorParameterValue(TEXT("LightingDirection"),
			MyForceManualSunPosition ? ManualSunColor : LightDirectionColor);
	}
	MyMIOutput->SetVectorParameterValue(TEXT("LightingPosition"), LightingPositionColor);
	MyMIOutput->SetScalarParameterValue(TEXT("LightSource"),
		MyLightDirectionSourceIsRotation_NOT_Pos ? 1.0f : 0.0f);
	MyMIOutput->SetScalarParameterValue(TEXT("AttenuationExponent"),
		static_cast<float>(MyDistanceBasedLightAttenuation ? MyAttenuationPower : 0.0));

	// 更新 Facing 供下一次执行的双面混合使用（蓝图 then_1 在 then_0 之后执行，本次混合读的是上一次的值）。
	MyFacing = FVector::DotProduct(
		FRotationMatrix(MyTraceMeshComponent->GetComponentRotation()).GetUnitAxis(EAxis::Z), ViewNormal) * -1.0;
}


void UMyNinjaLiveComponent::MyTraceChannelAutoFind()
{
	// 设置安全标志：追踪通道尚未设置
	MyTraceChannelsSet = false;

	// PreferredTraceChannelName 为空时设为默认值 "FluidTrace"
	if (MyPreferredTraceChannelName.IsEmpty())
	{
		MyPreferredTraceChannelName = TEXT("FluidTrace");
	}

	// 检查当前 TraceChannel 是否已匹配 PreferredTraceChannelName
	{
		const UEnum* TraceTypeEnum = StaticEnum<ETraceTypeQuery>();
		if (TraceTypeEnum)
		{
			const FString CurrentName = TraceTypeEnum->GetNameStringByValue(MyTraceChannel);
			if (CurrentName == MyPreferredTraceChannelName)
			{
				// 已匹配，跳过 ETraceTypeQuery 遍历
				goto CollisionChannelSearch;
			}
		}
	}

	// 遍历 ETraceTypeQuery，查找匹配的通道名
	{
		const UEnum* TraceTypeEnum = StaticEnum<ETraceTypeQuery>();
		if (TraceTypeEnum)
		{
			for (int32 i = 0; i < TraceTypeEnum->NumEnums(); i++)
			{
				if (TraceTypeEnum->HasMetaData(TEXT("Hidden"), i))
				{
					continue;
				}
				const FString EnumName = TraceTypeEnum->GetNameStringByIndex(i);
				if (EnumName == MyPreferredTraceChannelName)
				{
					MyTraceChannel = static_cast<ETraceTypeQuery>(TraceTypeEnum->GetValueByIndex(i));
					break;
				}
			}
		}
	}

CollisionChannelSearch:
	// 遍历 ECollisionChannel，查找匹配的通道名
	{
		const UEnum* CollisionEnum = StaticEnum<ECollisionChannel>();
		if (CollisionEnum)
		{
			for (int32 i = 0; i < CollisionEnum->NumEnums(); i++)
			{
				if (CollisionEnum->HasMetaData(TEXT("Hidden"), i))
				{
					continue;
				}
				const FString EnumName = CollisionEnum->GetNameStringByIndex(i);
				if (EnumName == MyPreferredTraceChannelName)
				{
					MyCollisionChannel = static_cast<ECollisionChannel>(CollisionEnum->GetValueByIndex(i));
					break;
				}
			}
		}
	}

	// 追踪通道已设置完毕
	MyTraceChannelsSet = true;
}

void UMyNinjaLiveComponent::MySceneCapCameraVSInputMaterials()
{
	// 判断：有输入材质 且 场景捕捉相机无效 → 使用输入材质
	MyUseInputMaterials = (MyInputMaterials.Num() > 0) && MyInputSceneCaptureCamera.Get() == nullptr;
}

void UMyNinjaLiveComponent::MySetTraceMeshProperties()
{
	AMyNinjaLiveActor* NinjaLive = nullptr;
	if (MyCheckComponentOwner(NinjaLive) && NinjaLive)
	{
		MyTraceMeshIsAlsoInteractionVolume = NinjaLive->MyUseTraceMeshAsInteractionVolume;
		if (NinjaLive->MyActivationVolume)
		{
			NinjaLive->MyActivationVolume->SetGenerateOverlapEvents(NinjaLive->MySimActivatedByPawnProximity);
		}
	}
	else
	{
		MyTraceMeshIsAlsoInteractionVolume = false;
	}

	UStaticMeshComponent* TraceMesh = MyTraceMeshComponent.Get();
	if (!IsValid(TraceMesh))
	{
		return;
	}

	TraceMesh->SetTranslucentSortPriority(MyTraceMeshTranslucentSortPrio);
	if (!bMyTraceMeshInitialRotationCaptured)
	{
		MyTraceMeshInitialRotation = TraceMesh->GetComponentRotation();
		bMyTraceMeshInitialRotationCaptured = true;
	}

	TraceMesh->SetGenerateOverlapEvents(MyTraceMeshIsAlsoInteractionVolume);
	TraceMesh->CanCharacterStepUpOn = ECB_No;
	TraceMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TraceMesh->SetCollisionObjectType(MyTraceMeshIsAlsoInteractionVolume ? ECC_WorldDynamic : ECC_WorldStatic);
	TraceMesh->SetCollisionResponseToAllChannels(
		MyTraceMeshIsAlsoInteractionVolume ? ECR_Overlap : ECR_Ignore);
	TraceMesh->SetCollisionResponseToChannel(MyCollisionChannel, ECR_Block);

	const double SizeInMeters = FMath::Max3(TraceMesh->Bounds.BoxExtent.X, TraceMesh->Bounds.BoxExtent.Y, TraceMesh->Bounds.BoxExtent.Z) * 0.001;
	MyTraceMeshSizeCoeff = MyBrushScaledInverselyByTraceMeshSize
		? (SizeInMeters == 0.0 ? 0.0 : (1.0 / SizeInMeters))
		: 1.0;
}

FVector UMyNinjaLiveComponent::MyDefineLineTracingSource() const
{
	// 默认描线源：玩家相机位置；不可用时回退到世界原点。
	FVector TraceSource = FVector::ZeroVector;
	if (const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		TraceSource = CameraManager->K2_GetActorLocation();
	}

	if (MyUseCustomTraceSource)
	{
		// 自定义源：Owner 变换将 CustomTraceSourcePosition 转到世界空间；
		// 任一轴为 0（未设置）时偏移 (100,100,100)，避免与世界原点重叠。
		const AActor* OwnerActor = GetOwner();
		const FTransform OwnerTransform = IsValid(OwnerActor) ? OwnerActor->GetTransform() : FTransform::Identity;
		FVector CustomSourceWorld = OwnerTransform.TransformPosition(MyCustomTraceSourcePosition);
		if (MyCustomTraceSourcePosition.X == 0.0 || MyCustomTraceSourcePosition.Y == 0.0 || MyCustomTraceSourcePosition.Z == 0.0)
		{
			CustomSourceWorld += FVector(100.0, 100.0, 100.0);
		}
		TraceSource = CustomSourceWorld;
	}

	return TraceSource;
}

void UMyNinjaLiveComponent::MyOverlapArtifactWorkaround2(FVector In)
{
	// 保存上一帧追踪位置（此时 TracePositionTemp 仍是旧值），再更新为本帧输入。
	MyLastTracePositionTemp = MyTracePositionTemp;
	MyTracePositionTemp = In;

	// 越界判定：追踪位置未变（物体停在边缘）、物体自身在移动、且非持续交互模式时，
	// FluidTrace 无法生成有效 UV，静音画笔避免伪影。
	const bool bTraceNotMoving = MyTracePositionTemp.Equals(MyLastTracePositionTemp, 0.1f);
	const bool bObjectMoving = !MyPosition1_3D.Equals(MyLastPosition1_3D, 0.1f);
	const bool bNotContinuousInteraction = !MyContinuousInteractionWithOwnerActor;
	const bool bMuteBrush = bTraceNotMoving && bObjectMoving && bNotContinuousInteraction;

	MyBrushStrengthTemp1 = bMuteBrush ? 0.0 : MyBrushStrength;
}

void UMyNinjaLiveComponent::MyTraceObjects2(FVector Start, FLinearColor& HitUV, bool& ThenExec, bool& NoHitExec)
{
	ThenExec = false;
	NoHitExec = false;

	// 从 Start 到物体位置做追踪；命中输出 UV 并走 then 分支，否则走 NoHit 分支。
	FVector TracePosition = FVector::ZeroVector;
	bool HitValid = false;
	TArray<AActor*> TraceExclude;
	TraceExclude.Reserve(MyNinjaLiveTraceExclude.Num());
	for (const TObjectPtr<AActor>& Excluded : MyNinjaLiveTraceExclude)
	{
		if (Excluded)
		{
			TraceExclude.Add(Excluded);
		}
	}
	UMyNinjaLiveFunctions::MyTraceOverlap(
		this, Start, MyPosition1_3D, 1.5, MyTraceChannel,
		TraceExclude, MyUsePAINTER_V2_ToTrackObjects,
		HitUV, TracePosition, HitValid);

	if (!HitValid)
	{
		NoHitExec = true;
		return;
	}

	// 命中：Painter v2 直接使用画笔强度；否则走越界修复（可能静音画笔）。
	if (MyUsePAINTER_V2_ToTrackObjects)
	{
		MyBrushStrengthTemp1 = MyBrushStrength;
	}
	else
	{
		MyOverlapArtifactWorkaround2(TracePosition);
	}

	MyTimeSinceLastCollision = 0.0;
	MyHitValid = true;
	ThenExec = true;
}

void UMyNinjaLiveComponent::MyTraceObjects1(FVector Start, FLinearColor& HitUV)
{
	HitUV = FLinearColor::Black;

	// 3D 位置是否静止：与上一帧相同，或上一帧为零（初始帧）。
	MyPosition1_3D_Static =
		MyPosition1_3D.Equals(MyLastPosition1_3D, 0.001f)
		|| MyLastPosition1_3D.Equals(FVector::ZeroVector, 0.001f);

	MyTimeSinceLastCollision = 0.0;

	// 持续交互时不做重叠检查但仍须追踪物体位置；两种交互方式任一成立才执行追踪。
	if (MyContinuousInteractionWithOwnerActor || MyOverlap1)
	{
		FVector TracePosition = FVector::ZeroVector;
		bool HitValid = false;
		TArray<AActor*> TraceExclude;
		TraceExclude.Reserve(MyNinjaLiveTraceExclude.Num());
		for (const TObjectPtr<AActor>& Excluded : MyNinjaLiveTraceExclude)
		{
			if (Excluded)
			{
				TraceExclude.Add(Excluded);
			}
		}
		UMyNinjaLiveFunctions::MyTraceOverlap(
			this, Start, MyPosition1_3D, 1.5, MyTraceChannel,
			TraceExclude, false,
			HitUV, TracePosition, HitValid);

		// 保存上一帧追踪位置（此时 MyTracePositionTemp 仍是旧值），再更新为本帧追踪位置。
		MyLastTracePositionTemp = MyTracePositionTemp;
		MyTracePositionTemp = TracePosition;

		// 物体跨出模拟平面边缘仍保持重叠时 FluidTrace 失效、无法生成有效 UV，静音画笔避免伪影。
		const bool bTraceNotMoving = MyTracePositionTemp.Equals(MyLastTracePositionTemp, 0.1f);
		const bool bObjectMoving = !MyPosition1_3D.Equals(MyLastPosition1_3D, 0.001f);
		const bool bNotContinuousInteraction = !MyContinuousInteractionWithOwnerActor;
		MyBrushStrengthTemp1 = (bTraceNotMoving && bObjectMoving && bNotContinuousInteraction) ? 0.0 : MyBrushStrength;
	}
}

void UMyNinjaLiveComponent::MyTraceObj2()
{
	FLinearColor HitUV = FLinearColor::Black;
	bool bHit = false;
	bool bNoHit = false;
	MyTraceObjects2(MyDefineLineTracingSource(), HitUV, bHit, bNoHit);

	// ExecutionSequence 的 then_0：仅命中时更新点画笔和本帧 Painter v2 数据。
	if (bHit)
	{
		MyPosition1_2D = MyBrushRnd3(HitUV);
		MySetBrushDensityParams3(MyBrushSizeCoEff());

		// 条件为真时先写入画笔尺寸；两条分支随后汇合到速度计算。
		const bool bUsePainterV2Arrays = MyUsePAINTER_V2_ToTrackObjects && !MySingleTargetMode_LEGACY;
		if (bUsePainterV2Arrays)
		{
			MyBrushSizeArray.Add(static_cast<float>(MyBrushSizeCoEff()));
		}

		FLinearColor Velocity = FLinearColor::Black;
		MyMultiObjectVelocity(Velocity);

		if (bUsePainterV2Arrays)
		{
			MyVelocityArray.Add(Velocity);
		}

		MyFinalDealRTAndBrush();
	}

	// ExecutionSequence 的 then_1：追踪失败时进入线条绘制冷却，不继续画笔收尾流程。
	if (bNoHit)
	{
		MyTemporarilySwitchOffLineDrawingIFTracerFails();
	}
}

void UMyNinjaLiveComponent::MyMultiObjectProcessorCycle3()
{
	// 蓝图分别取 Map 的 Keys/Values 并以相同索引配对；直接遍历 TMap 可保留每个临时数组索引与组件的对应关系。
	for (const TPair<int32, TObjectPtr<UPrimitiveComponent>>& SkeletalMeshTempArrayPair : MySkeletalMeshTempArrayPairs)
	{
		const TArray<FName>& Bones = MyGetTempArray(SkeletalMeshTempArrayPair.Key);
		for (const FName Bone : Bones)
		{
			MyOverlappingBone = Bone;
			MyOverlappingSkeletalMesh = SkeletalMeshTempArrayPair.Value;
			MyBuildOverlapSKMArray(MyOverlappingSkeletalMesh.Get());
			MyCalcPos5();
			MyTraceObj2();
		}
	}

	// 外层 ForEachLoop Completed：每轮多物体追踪结束后仅推进一次流体模拟。
	MyFluidCoreStep();
}

void UMyNinjaLiveComponent::MyForLoopOverlapping()
{
	const bool bBuildPrimitiveArray = MyUsePAINTER_V2_ToTrackObjects
		&& !MySingleTargetMode_LEGACY
		&& MyPV2_Connect_TrackpointsWithLines;

	for (const TObjectPtr<UPrimitiveComponent>& OverlappingComponent : MyOverlappingComponents)
	{
		MyOverlappingComponent = OverlappingComponent;
		if (bBuildPrimitiveArray)
		{
			MyPrimitivesArray.Add(MyOverlappingComponent);
		}

		// 条件的 true/false 分支均在此汇合，始终继续位置计算和追踪。
		MyCalcPos3();
		MyTraceObj2();
	}

	// 外层 ForEachLoop Completed：继续处理骨骼重叠组件，后者负责推进流体核心步骤。
	MyMultiObjectProcessorCycle3();
}

void UMyNinjaLiveComponent::MyNoInteraction()
{
	// ExecutionSequence 的 then_0：清零两种画笔材质的强度。
	for (UMaterialInstanceDynamic* PainterMaterial : { MyMICollisionPainterLine.Get(), MyMICollisionPainterDot.Get() })
	{
		if (IsValid(PainterMaterial))
		{
			PainterMaterial->SetScalarParameterValue(TEXT("BrushStrength"), 0.0f);
		}
	}

	// 未达到空闲 Canvas 停用阈值时，仍需完成本帧画笔收尾。
	if (!MyBrushFadeOutTimer())
	{
		MyFinalDealRTAndBrush();
	}

	// ExecutionSequence 的 then_1：无论画笔是否收尾，均推进流体核心步骤。
	MyFluidCoreStep();
}

void UMyNinjaLiveComponent::MyTemporarilySwitchOffLineDrawingIFTracerFails()
{
	// 仅 Painter v2 追踪、非旧版单目标且连接追踪点画线时，暂停线条绘制。
	const bool bShouldSwitchOff = MyUsePAINTER_V2_ToTrackObjects
		&& !MySingleTargetMode_LEGACY
		&& MyPV2_Connect_TrackpointsWithLines;
	if (!bShouldSwitchOff)
	{
		return;
	}

	MyHitValid = false;

	// 冷却期后恢复线条绘制（对应蓝图 RetriggerableDelay：每次触发重置计时）。
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MyLineDrawingFailCooldownTimer);
		World->GetTimerManager().SetTimer(
			MyLineDrawingFailCooldownTimer,
			this,
			&UMyNinjaLiveComponent::MyRestoreLineDrawingAfterCooldown,
			static_cast<float>(MyPV2LineDrawingFailCooldownTime),
			false);
	}
}

void UMyNinjaLiveComponent::MyRestoreLineDrawingAfterCooldown()
{
	MyHitValid = true;
}

void UMyNinjaLiveComponent::MyFPSPrecisionResolution()
{
	// 分辨率过小会导致流体材质采样越界，蓝图用 256x256 兜底。
	if (MyResolutionX < 8 || MyResolutionY < 8)
	{
		MyResolutionX = 256;
		MyResolutionY = 256;
	}

	// 采样频率直接取上限，并将其转换为每次 Tick 的时间间隔。
	MySamplingFPS = MyMaxSamplingFPS;
	MyTickRateCustom = MyMaxSamplingFPS > 0
		? 1.0 / static_cast<double>(MyMaxSamplingFPS)
		: 0.0;

	MySimPrecisionIndex = (MySimPrecision == EMySimPrecision::Bit32) ? 1 : 0;

	// Painter v2 的速度生成和轨迹连线共用插值开关。
	MyPV2_Interpolation = MyPV2_Connect_TrackpointsWithLines || MyPV2_GenerateVelocity;
	MyPV2_Connect_TrackpointsWithLines = MyPV2_Interpolation;
}

void UMyNinjaLiveComponent::MyInitPainterV2()
{
	// 不支持 Painter v2 的配置会回退到常规追踪流程。
	if (!MyUsePAINTER_V2_ToTrackObjects || MySingleTargetMode_LEGACY)
	{
		MyUsePAINTER_V2_ToTrackObjects = false;
		return;
	}

	const int32 SystemIndex = MyPV2_Connect_TrackpointsWithLines ? 1 : 0;
	// 系统资源按“是否连接追踪点”选择，缺失资源时禁止继续创建空组件。
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

	// 每次初始化创建独立 Niagara 实例，避免旧的运行时参数残留。
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

	// 输出参数声明为 RenderTarget 类型，必须使用对应 Niagara 数据接口写入。
	const TObjectPtr<UTextureRenderTarget2D>* PainterTarget = MyRenderTargetsMap.Find(TEXT("RT_Painter"));
	UTextureRenderTarget2D* PainterRenderTarget = PainterTarget ? PainterTarget->Get() : nullptr;
	MyNiagaraBasedPainter->SetVariableTextureRenderTarget(TEXT("User.PaintbufferOutput"), PainterRenderTarget);
	// 初始化阶段关闭位置插值，待线条绘制冷却后再恢复最终配置。
	MyNiagaraBasedPainter->SetVariableBool(TEXT("User.PosInterpol"), false);
	// 两条初始化路径都会执行这组共享参数；此处先完成首帧配置。
	MyApplyPainterV2SharedParameters();

	if (UWorld* World = GetWorld())
	{
		// 输入缓冲与冷却后的最终参数分别独立调度，互不等待。
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
		// 没有 World 时不能调度 latent 分支，保留立即参数链以便编辑器预览。
		MySetPainterV2PaintbufferInput();
		MyFinalizePainterV2Setup();
	}
}

void UMyNinjaLiveComponent::MyForwardScalarParamsToNiagara()
{
	if (!MyUsePAINTER_V2_ToTrackObjects || MySingleTargetMode_LEGACY ||
		!IsValid(MyMICollisionPainterDot) || !IsValid(MyNiagaraBasedPainter))
	{
		return;
	}

	TArray<FMaterialParameterInfo> ParameterInfos;
	TArray<FGuid> ParameterIds;
	MyMICollisionPainterDot->GetAllScalarParameterInfo(ParameterInfos, ParameterIds);

	for (const FMaterialParameterInfo& ParameterInfo : ParameterInfos)
	{
		// 蓝图仅排除 BrushSize；该值由 Painter v2 自身的轨迹数据驱动。
		if (ParameterInfo.Name == TEXT("BrushSize"))
		{
			continue;
		}

		float ParameterValue = 0.0f;
		if (MyMICollisionPainterDot->GetScalarParameterValue(
			FHashedMaterialParameterInfo(ParameterInfo), ParameterValue))
		{
			MyNiagaraBasedPainter->SetVariableFloat(ParameterInfo.Name, ParameterValue);
		}
	}
}

void UMyNinjaLiveComponent::MySetPosVelocityScaleArraysToPainterV2()
{
	if (!MyUsePAINTER_V2_ToTrackObjects || MySingleTargetMode_LEGACY || !IsValid(MyNiagaraBasedPainter))
	{
		return;
	}

	if (MyPV2_Connect_TrackpointsWithLines)
	{
		const bool bPositionArraysMatch = MyLastPositionArray.Num() == MyPositionArray.Num();
		const bool bTracePositionUnchanged = FVector2D(MyTraceMeshPos) == FVector2D(MyTraceMeshLastPos);
		const bool bCanInterpolate =
			(MyQuantizerStepSize < 1 || bTracePositionUnchanged) && bPositionArraysMatch;
		const bool bEnableInterpolation = bCanInterpolate && MyPV2_Interpolation &&
			MyMaxSamplingFPS == MySamplingFPS && MyHitValid;
		MyNiagaraBasedPainter->SetVariableBool(TEXT("User.PosInterpol"), bEnableInterpolation);

		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector2D(
			MyNiagaraBasedPainter, TEXT("User.PositionArray2D"), MyPositionArray);

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

		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector2D(
			MyNiagaraBasedPainter, TEXT("User.LastPositionArray2D"), MyLastPositionArray);
	}
	else
	{
		MyNiagaraBasedPainter->SetVariableBool(TEXT("User.PosInterpol"), false);
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector2D(
			MyNiagaraBasedPainter, TEXT("User.PositionArray2D"), MyPositionArray);
	}

	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayColor(
		MyNiagaraBasedPainter, TEXT("User.VelocityArray"), MyVelocityArray);

	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(
		MyNiagaraBasedPainter, TEXT("User.BrushSizeArray"), MyBrushSizeArray);
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
	// Painter v2 且非单目标模式：把当前画笔位置追加到位置数组。
	if (MyUsePAINTER_V2_ToTrackObjects && !MySingleTargetMode_LEGACY)
	{
		MyPositionArray.Add(FVector2D(MyPosition1_2D.R, MyPosition1_2D.G));
	}
}

void UMyNinjaLiveComponent::MyFinalDealRTAndBrush()
{
	// 条件为 false（非 Painter v2 追踪或旧版单目标模式）：先把点画笔材质绘制到 RT_Painter。
	if (!(MyUsePAINTER_V2_ToTrackObjects && !MySingleTargetMode_LEGACY))
	{
		const TObjectPtr<UTextureRenderTarget2D>* PainterRT = MyRenderTargetsMap.Find(TEXT("RT_Painter"));
		if (PainterRT && IsValid(PainterRT->Get()) && IsValid(MyMICollisionPainterDot))
		{
			UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, PainterRT->Get(), MyMICollisionPainterDot);
		}
	}

	// 两条分支汇合：设置 Multitarget 参数为 1，再构建画笔位置数组。
	if (IsValid(MyMICollisionPainterDot))
	{
		MyMICollisionPainterDot->SetScalarParameterValue(TEXT("Multitarget"), 1.0f);
	}
	MyBuildBrushPositionArray();
}

void UMyNinjaLiveComponent::MyBuildOverlapSKMArray(UPrimitiveComponent* In)
{
	// 仅 Painter v2 追踪、非旧版单目标且连接追踪点画线时，把重叠组件加入 SK 网格数组。
	if (MyUsePAINTER_V2_ToTrackObjects && !MySingleTargetMode_LEGACY && MyPV2_Connect_TrackpointsWithLines)
	{
		MySKmeshesArray.Add(In);
	}
}

double UMyNinjaLiveComponent::MyBrushSizeCoEff() const
{
	return MyBrushSize * MyTraceMeshSizeCoeff * MyOverlappingMeshSizeCoeff * MyGlobalBrushScale * 0.5;
}

void UMyNinjaLiveComponent::MyCalculateBrushSizeCoEffFromBoneDistance(FVector In, double BrushScaleMult, double& Out)
{
	// 重叠骨骼网格无效时保持 Out 默认（对应蓝图 Cast Failed 无连接分支）。
	Out = 0.0;
	USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(MyOverlappingSkeletalMesh.Get());
	if (!IsValid(SkeletalMesh))
	{
		return;
	}

	// 距离 = |In - 父骨骼 Socket 位置|，×0.01 转米，×BrushScaleMult。
	const FName ParentBone = SkeletalMesh->GetParentBone(MyOverlappingBone);
	const FVector ParentBoneWorldPos = SkeletalMesh->GetSocketLocation(ParentBone);
	const double BoneDistance = FMath::Abs((In - ParentBoneWorldPos).Size());
	Out = BoneDistance * 0.01 * BrushScaleMult;
}

void UMyNinjaLiveComponent::MyCalcPos5()
{
	// 保存上一帧位置，再刷新为重叠骨骼的 Socket 位置。
	MyLastPosition1_3D = MyPosition1_3D;
	MyPosition1_3D = IsValid(MyOverlappingSkeletalMesh.Get())
		? MyOverlappingSkeletalMesh->GetSocketLocation(MyOverlappingBone)
		: FVector::ZeroVector;

	// 按交互物体尺寸缩放画笔：开则按骨骼距离计算，关则直接用骨骼画笔缩放系数。
	if (MyBrushScaledByInteractingObjSize)
	{
		double Out = 0.0;
		MyCalculateBrushSizeCoEffFromBoneDistance(MyPosition1_3D, MySkeletalMeshBrushScale, Out);
		MyOverlappingMeshSizeCoeff = Out;
	}
	else
	{
		MyOverlappingMeshSizeCoeff = MySkeletalMeshBrushScale;
	}

	MyPosDataType = 1;
}

void UMyNinjaLiveComponent::MyCalcPos3()
{
	// 保存上一帧位置，再刷新为重叠组件的世界位置。
	MyLastPosition1_3D = MyPosition1_3D;
	MyPosition1_3D = IsValid(MyOverlappingComponent.Get())
		? MyOverlappingComponent->K2_GetComponentLocation()
		: FVector::ZeroVector;

	// 按重叠物体边界或缩放计算画笔尺寸系数。
	MyOverlappingMeshSizeCoeff = MyCalculateBrushSizeCoFromBounds1(MyOverlappingComponent.Get());

	MyPosDataType = 0;
}

void UMyNinjaLiveComponent::MyCalcPos2(UObject* In)
{
	// 输入对象无效时不更新任何数据（IsValid 的 Is Not Valid 分支在蓝图中未连线）。
	if (!IsValid(In))
	{
		return;
	}

	// 保存上一帧位置，再刷新为重叠组件的世界位置。
	MyLastPosition1_3D = MyPosition1_3D;
	MyPosition1_3D = IsValid(MyOverlappingComponent.Get())
		? MyOverlappingComponent->K2_GetComponentLocation()
		: FVector::ZeroVector;

	// 更新按重叠物体边界/缩放计算的画笔尺寸系数（与 CalcPos3 共用计算）。
	MyOverlappingMeshSizeCoeff = MyCalculateBrushSizeCoFromBounds1(MyOverlappingComponent.Get());
}

double UMyNinjaLiveComponent::MyCalculateBrushSizeCoFromBounds1(USceneComponent* Component) const
{
	// 未按交互物体尺寸缩放时恒为 1.0（SelectFloat 的 B 分支）。
	if (!MyBrushScaledByInteractingObjSize)
	{
		return 1.0;
	}

	// 数据源：用包围盒范围或组件缩放×50（SelectVector 的 B/A 分支）。
	FVector DataSource = FVector::ZeroVector;
	if (IsValid(Component))
	{
		if (MyUseObjBoundsInsteadOfSize)
		{
			FVector Origin = FVector::ZeroVector;
			FVector BoxExtent = FVector::ZeroVector;
			float SphereRadius = 0.0f;
			UKismetSystemLibrary::GetComponentBounds(Component, Origin, BoxExtent, SphereRadius);
			DataSource = BoxExtent;
		}
		else
		{
			DataSource = Component->K2_GetComponentScale() * 50.0;
		}
	}

	// 最小分量 × PrimitiveObjBrushScale × 0.01。
	const double MinElement = FMath::Min(DataSource.X, FMath::Min(DataSource.Y, DataSource.Z));
	return MinElement * MyPrimitiveObjBrushScale * 0.01;
}

void UMyNinjaLiveComponent::MyDrawInternalRenderTargetToExternal()
{
	if (!MyDrawInternalRenderTargetToExternalEnabled)
	{
		return;
	}

	// 蓝图 DoOnce：仅首轮校验数组配对与目标有效性，失败后 Gate 永久关闭。
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
		// 延后绑定输入缓冲，确保 Niagara 系统实例已经完成创建。
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
	// 冷却结束后写入稳定状态，并在生成速度时启用对应 Niagara 分支。
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

	// 以下参数定义 Painter v2 的采样空间、速度阈值和画笔强度。
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
		// 没有有效噪声纹理时保留 Niagara 资源中的默认绑定。
		MyNiagaraBasedPainter->SetVariableTexture(TEXT("User.BrushNoiseVeloTexture"), MyPainterV2BrushVeloNoiseTexture);
	}
	if (IsValid(MyTraceMeshComponent))
	{
		// TraceMesh 最大轴向缩放决定 Niagara 画笔的空间范围。
		const FVector Scale = MyTraceMeshComponent->GetRelativeScale3D();
		MyNiagaraBasedPainter->SetVariableFloat(TEXT("User.TraceMeshMaxExtent"), FMath::Max3(Scale.X, Scale.Y, Scale.Z));
	}

	// 新一轮 Painter 初始化不复用上一轮的追踪历史。
	MyPositionArray.Reset();
	MyLastPositionArray.Reset();
	MyVelocityArray.Reset();
	MyBrushSizeArray.Reset();

	if (MyForceMaxSamplingFPSToNiagara && MyMaxSamplingFPS > 0)
	{
		// Solo 模式使 Niagara 以流体模拟指定的采样频率独立更新。
		MyNiagaraBasedPainter->SetForceSolo(true);
		MyNiagaraBasedPainter->SetComponentTickInterval(1.0 / static_cast<double>(MyMaxSamplingFPS));
		MyNiagaraBasedPainter->ReinitializeSystem();
	}
}

void UMyNinjaLiveComponent::MyManageContinuousInteractions()
{
	if (!MyContinuousInteractionWithOwnerActor)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return;
	}

	MyOverlappingComponents.Reset();
	MyContinuousInteractionSkeletalComponent.Reset();
	MySkeletalMeshTempArrayPairs.Reset();
	MyListOfAvailableTempArrays.Init(false, 40);

	TArray<UPrimitiveComponent*> OwnerComponents;
	OwnerActor->GetComponents<UPrimitiveComponent>(OwnerComponents);
	for (UPrimitiveComponent* PrimitiveComponent : OwnerComponents)
	{
		if (!IsValid(PrimitiveComponent))
		{
			continue;
		}

		const EObjectTypeQuery ObjectType = UEngineTypes::ConvertToObjectType(PrimitiveComponent->GetCollisionObjectType());
		const bool bAllowedObjectType = MyContinuousInteractionInclusiveObjType.IsEmpty() ||
			MyContinuousInteractionInclusiveObjType.Contains(ObjectType);
		const bool bAllowedName = MyContinuousInteractionComponentNamesExact.IsEmpty() ||
			MyContinuousInteractionComponentNamesExact.Contains(PrimitiveComponent->GetFName());
		if (bAllowedObjectType && bAllowedName)
		{
			MyOverlappingComponents.Add(PrimitiveComponent);
		}
	}

	TArray<USkeletalMeshComponent*> SkeletalComponents;
	OwnerActor->GetComponents<USkeletalMeshComponent>(SkeletalComponents);
	MyContinuousInteractionSkeletalComponent.Append(SkeletalComponents);
	for (USkeletalMeshComponent* SkeletalComponent : SkeletalComponents)
	{
		if (!IsValid(SkeletalComponent) ||
			(!MyOverlappingComponents.Contains(SkeletalComponent)))
		{
			continue;
		}

		MyContinuousInteractionBoneNamesExactTemp = MyContinuousInteractionBoneNamesExact;
		if (MyContinuousInteractionBoneNamesExactTemp.IsEmpty())
		{
			continue;
		}

		TArray<FName> MatchedBones;
		for (int32 BoneIndex = 0; BoneIndex < SkeletalComponent->GetNumBones(); ++BoneIndex)
		{
			const FName BoneName = SkeletalComponent->GetBoneName(BoneIndex);
			if (MyContinuousInteractionBoneNamesExactTemp.RemoveSingle(BoneName) > 0)
			{
				MatchedBones.Add(BoneName);
			}
		}

		if (MatchedBones.IsEmpty())
		{
			continue;
		}

		const int32 TempArrayIndex = MyListOfAvailableTempArrays.IndexOfByPredicate([](bool bOccupied)
		{
			return !bOccupied;
		});
		if (TempArrayIndex == INDEX_NONE)
		{
			break;
		}

		MyClearTempArray(TempArrayIndex);
		MyAppendToTempArray(TempArrayIndex, MatchedBones);
		MyListOfAvailableTempArrays[TempArrayIndex] = true;
		MySkeletalMeshTempArrayPairs.Add(TempArrayIndex, SkeletalComponent);
	}

	MyContinuousInteractionBoneNamesExactTemp2 = MyContinuousInteractionBoneNamesExactTemp;
}

void UMyNinjaLiveComponent::MyCheckValidity2(UPrimitiveComponent*& SingleTarget, bool& ThenExec)
{
	ThenExec = false;
	SingleTarget = nullptr;

	// 无重叠组件时没有有效目标（蓝图 IfThenElse 的 else 分支未连线）。
	if (MyOverlappingComponents.Num() == 0)
	{
		return;
	}

	// 未启用精确组件名筛选时，直接取第一个重叠组件。
	if (MyContinuousInteractionComponentNamesExact.Num() == 0)
	{
		SingleTarget = MyOverlappingComponents[0].Get();
		ThenExec = true;
		return;
	}

	// 遍历重叠组件，取对象名匹配第一个精确名称的组件；与蓝图 ForEachLoop 一致，每个匹配都会输出。
	const FName FirstExactName = MyContinuousInteractionComponentNamesExact[0];
	for (const TObjectPtr<UPrimitiveComponent>& Component : MyOverlappingComponents)
	{
		if (Component && Component->GetFName() == FirstExactName)
		{
			SingleTarget = Component.Get();
			ThenExec = true;
		}
	}
}

void UMyNinjaLiveComponent::MyCreateOrAcquireRenderTargets()
{
	MyRenderTargetsMap.Empty();
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

	if (MyRenderTargetsMap.Num() == 6 &&
		(MyMake1stOutputAvailableFor2ndOutput || MyMake1stOutputAvailableForNiagara))
	{
		const int32 OutputMultiplier = MyForce2xResolutionOutputBuffer ? 2 : 1;
		const ETextureRenderTargetFormat OutputFormat = MyForce8bitOutputBuffer ? RTF_RGBA8 : RGBAFormat;
		AddRenderTarget(TEXT("RT_Output"), FullWidth * OutputMultiplier, FullHeight * OutputMultiplier,
			OutputFormat, MySimAreaClamp);
	}
}

void UMyNinjaLiveComponent::MyCreateDynamicMaterialInstances()
{
	// CoreSimMaterials 的索引由原蓝图固定定义；压力材质还取决于求解器与移动端翻转选项。
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

	// 蓝图的 Simple Painter 分支只创建两个 Painter、Null 和 Painter Offset MID。
	// 模拟的五个 MID 位于 If 的 false 分支，不能在此模式下提前创建。
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

	if (UMaterialInterface* NullMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/FluidNinjaLive/Core/Materials/M_SolidColor.M_SolidColor")))
	{
		MyMINull = UMaterialInstanceDynamic::Create(NullMaterial, this);
	}
	else
	{
		MyMINull = nullptr;
	}

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
		// 蓝图即使输入为空也会写入参数；跳过空纹理会意外保留旧 MID 的参数值。
		if (IsValid(Material))
		{
			Material->SetTextureParameterValue(Parameter, Texture);
		}
	};

	if (!MySimplePainterMode)
	{
		// 按蓝图每个 MID 的参数名和 RT 连线绑定，不能按材质阶段泛化。
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
			// 蓝图此处先 Cast To TextureRenderTarget2D；转换失败时不会执行 TextureAdd2 节点。
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
		// 蓝图只有在遮罩有效且不是默认遮罩时才写 true；不在此处回写 false。
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
	// 保持与蓝图相同的存在判定及循环次数计算。
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
	// 蓝图先按 DisableComponent 和 TraceMeshInvisible 选择 TraceMesh 的显示材质。
	if (IsValid(MyTraceMeshComponent))
	{
		UMaterialInterface* TraceMaterial = MyTraceMeshInvisible
			? MyNullMaterial.Get()
			: MyMIOutput.Get();
		TraceMaterial = MyDisableComponent ? MyInactiveGrayMaterial.Get() : TraceMaterial;
		if (IsValid(TraceMaterial))
		{
			MyTraceMeshComponent->SetMaterial(0, TraceMaterial);
		}
	}

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
			// 蓝图循环结束后使用此轮数组的最后一个 Actor 与当前索引设置体积云材质。
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
				NiagaraComponent->SetComponentTickInterval(1.0 / static_cast<double>(MyMaxSamplingFPS));
				NiagaraComponent->ReinitializeSystem();
			}
		}
	}
}

void UMyNinjaLiveComponent::MyAfterCreateRT()
{
	MyCreateDynamicMaterialInstances();

	UClass* NinjaLiveInterfaceClass = LoadClass<UInterface>(
		nullptr,
		TEXT("/Game/FluidNinjaLive/Core/NinjaLiveInterface.NinjaLiveInterface_C"));
	MyNinjaLiveTraceExclude.Reset();
	if (NinjaLiveInterfaceClass != nullptr)
	{
		TArray<AActor*> InterfaceActors;
		UGameplayStatics::GetAllActorsWithInterface(this, NinjaLiveInterfaceClass, InterfaceActors);
		for (AActor* InterfaceActor : InterfaceActors)
		{
			MyNinjaLiveTraceExclude.Add(InterfaceActor);
		}
	}
	MyNinjaLiveTraceExclude.Remove(GetOwner());

	MyManageContinuousInteractions();
	MyAlternativeInputsFedToCompositeDensityInput();
	MyCreateOutputMaterialAndSetItOnTargetsStep01();
	MyCreateOutputMaterialAndSetItOnTargetsStep02();
	MyCreateOutputMaterialAndSetItOnTargetsStep03();

	if (MySupressUE51TextureSmearing)
	{
		UKismetSystemLibrary::ExecuteConsoleCommand(this, TEXT("r.TSR.ShadingRejection.Flickering 0"));
		UKismetSystemLibrary::ExecuteConsoleCommand(this, TEXT("r.TSR.ShadingRejection.Flickering.Period 0"));
	}

	MyInitPainterV2();
	MyInitDone = true;
	MyMaterialInstacesDone = true;
	if (IsValid(MyTraceMeshComponent))
	{
		MyTraceMeshComponent->SetVisibility(true, false);
	}

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
		UE_LOG(LogTemp, Display, TEXT("[FluidSim][Preset] %s = %.17g"),
			*Key,
			MyPresetMap.FindRef(Key));
	}
	MyParsePresetMapAndSetVariables(MyPresetMap);
	MyLoadTextures();
}

void UMyNinjaLiveComponent::MyUpdateCollisionMaskIsNonDefault()
{
	// 蓝图逻辑：遮罩有效且显示名不是默认 T_maskframe_256 时，视为自定义遮罩。
	MyCollisionMaskIsNonDefault = IsValid(MyCollisionMask) &&
		UKismetSystemLibrary::GetDisplayName(MyCollisionMask) != TEXT("T_maskframe_256");
}

void UMyNinjaLiveComponent::MyAlternativeInputsFedToCompositeDensityInput()
{
	// 场景捕捉优先写入密度输入 RT，并禁用输入材质分支。
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

	// 蓝图依次执行 SetMediaPlayer、OpenUrl、Play；缺少任一媒体对象时不启动该分支。
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
	// 蓝图在 RenderTarget 输入模式下直接继续后续流程，不覆盖当前密度纹理。
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

void UMyNinjaLiveComponent::MyLODDistaceStepsPrecalc()
{
	// 蓝图始终先用 LOD-Steps 初始化当前等级；禁用两个降级选项时不触碰既有阈值数据。
	MyLODLevel = MyLODSteps;
	if (!MyLOD1ReduceSimQuality && !MyLOD2ReduceSamplingFPS)
	{
		return;
	}

	const int32 LastIndex = MyLODSteps - 1;
	if (LastIndex < 0)
	{
		MyLODStepsArray.Reset();
		return;
	}

	// 原蓝图以 (Max(FarBound, NearBound) - 1) / (LOD-Steps - 1) 计算步长。
	// 单步没有可定义的分段范围，因此保留数组为空，避免原图的零除未定义结果。
	if (LastIndex == 0)
	{
		MyLODStepsArray.Reset();
		return;
	}

	MyLODStepRange = (FMath::Max(MyLODFarBound, MyLODNearBound) - 1.0) /
		static_cast<double>(LastIndex);
	MyLODStepsArray.Reset();
	MyLODStepsArray.Reserve(MyLODSteps);
	for (int32 Index = 0; Index <= LastIndex; ++Index)
	{
		const double Distance = MyLODNearBound + static_cast<double>(Index) * MyLODStepRange;
		MyLODStepsArray.Add(static_cast<double>(FMath::TruncToInt(Distance)));
	}
}

void UMyNinjaLiveComponent::MyAfterBind()
{
	MyLightDirectionProviderCheck();
	MyTraceChannelAutoFind();
	MySceneCapCameraVSInputMaterials();
	MyLODDistaceStepsPrecalc();
	MySetTraceMeshProperties();
	MyFPSPrecisionResolution();
	MyEnableOwnerInput();
	MyCreateOrAcquireRenderTargets();
	MyAfterCreateRT();
}

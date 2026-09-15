

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

	PrimaryComponentTick.bCanEverTick = true;
	MyRenderTargetsList = {
		TEXT("RT_Composite"),
		TEXT("RT_Advection"),
		TEXT("RT_Painter"),
		TEXT("RT_PressureDivergence"),
		TEXT("RT_PressureDivergenceTemp"),
		TEXT("RT_DensityInputMaterial"),
		TEXT("RT_Output")
	};


	MyPosition3_2D.Init(FLinearColor::Transparent, MyTouchSlotCount);
	MyLastPosition3_2D.Init(FLinearColor::Transparent, MyTouchSlotCount);
	MyListOfAvailableTempArrays.Init(true, MyTempArrayCount);
}

void UMyNinjaLiveComponent::BeginPlay()
{
	Super::BeginPlay();

	MyNativeTickLimiterDoOnceClosed = false;
	MyCustomTickLoopStarted = false;
	MyLODDoOnceClosed = false;
	MyAfterTickDelayDeactivateDoOnceClosed = false;
	MyAfterTickDelayRearmDoOnceClosed = false;
	bMyExternalRenderTargetExportValidated = false;
	bMyExternalRenderTargetExportGateOpen = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MyTimerCheckReady);
		World->GetTimerManager().SetTimer(MyTimerCheckReady, this,
			&UMyNinjaLiveComponent::MyCheckReady, 0.2f, true, 0.0f);
	}
}

void UMyNinjaLiveComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(MyTimerCheckReady);
		TimerManager.ClearTimer(MyTimerSimSpeedAdjustment);
		TimerManager.ClearTimer(MyInputMediaLoopTimer);
		TimerManager.ClearTimer(MyLoadTexturesTimer);
		TimerManager.ClearTimer(MyNiagaraPainterV2SafetyTimer);
		TimerManager.ClearTimer(MyNiagaraPainterV2CooldownTimer);
		TimerManager.ClearTimer(MyLineDrawingFailCooldownTimer);
		TimerManager.ClearTimer(MyLODCheckTimer);
		TimerManager.ClearTimer(MyCustomTickLoopTimer);
	}

	MyDestroyPainterV2();
	MyComponentRePlayEvent.Clear();
	MyWorldSpaceOffset.Clear();
	MyRenderTargetsMap.Reset();
	MyNiagaraSystemsToDrive.Reset();
	MyNinjaLiveTraceExclude.Reset();
	MyNinjaLiveTraceExcludeRaw.Reset();
	MyTraceExcludeRefreshFrame = MAX_uint64;
	MyCompositeScalarParameterIndices.Reset();
	MyDivergenceScalarParameterIndices.Reset();
	MyRDGOutputComparisonTarget = nullptr;
	MyRDGOutputTargetCreatedForValidation = false;
	MyRDGAdvectionComparisonTarget = nullptr;
	MyRDGDivergenceComparisonTarget = nullptr;
	MyRDGPressureComparisonTarget = nullptr;
	MyRDGPressureTempComparisonTarget = nullptr;
	MyMIPressureCycle1Comparison = nullptr;
	MyMIPressureCycle2Comparison = nullptr;

	MyMICompositeAndGradient = nullptr;
	MyMIAdvection = nullptr;
	MyMIDivergence = nullptr;
	MyMIPressureCycle1 = nullptr;
	MyMIPressureCycle2 = nullptr;
	MyMICollisionPainterLine = nullptr;
	MyMICollisionPainterDot = nullptr;
	MyMICollisionPainterOffset = nullptr;
	MyMINull = nullptr;
	MyMIOutput = nullptr;
	MyMISecondaryOutput = nullptr;
	MyMITertiaryOutput = nullptr;

	Super::EndPlay(EndPlayReason);
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
	MyComponentRePlayEvent.AddUniqueDynamic(this, &UMyNinjaLiveComponent::MyRePlay);
	MyProximityActivationMasterVarsQuantizerOutMat();
	MyAfterBind();
}

void UMyNinjaLiveComponent::MyAfterReadyCheck()
{
	MyRefreshTraceExcludeActors();
	MyLOD();
	MyMuteBrush();
	MyCameraFacing();


	const double InputFeedback = FMath::Min(MyInputFeedback, MyInputFeedbackInterface);
	if (IsValid(MyMICollisionPainterLine))
	{
		MyMICollisionPainterLine->SetScalarParameterValue(TEXT("InputFeedback"), InputFeedback);
		MyMICollisionPainterLine->SetScalarParameterValue(TEXT("Multitarget"), 0.0f);
	}
	if (IsValid(MyMICollisionPainterDot))
	{
		MyMICollisionPainterDot->SetScalarParameterValue(TEXT("InputFeedback"), InputFeedback);
		MyMICollisionPainterDot->SetScalarParameterValue(TEXT("Multitarget"), 0.0f);
	}

	MyClearPosVelocityScaleArraysPainterV2();
	MyCheckTouchOptions();

	if (MyMousePressed)
	{

		MyMousePassTrue();
		MyMousePassFalse();
	}
	else
	{
		MyMousePassFalse();
	}
}

void UMyNinjaLiveComponent::MyRePlay()
{
	AMyNinjaLiveActor* NinjaLiveOwner = nullptr;
	if (MyCheckComponentOwner(NinjaLiveOwner) && IsValid(NinjaLiveOwner))
	{
		NinjaLiveOwner->MyOverlappingActors.Reset();
		NinjaLiveOwner->MyOverlappingActorsInitial.Reset();
	}
	MyOverlappingComponents.Reset();
	MyOverlap1 = false;
	MyResetTempArraySlots();
	MyProximityActivationMasterVarsQuantizerOutMatFromOwner();
	MyLODDoOnceClosed = false;
	MyAfterBind();
	MyApplySimulationTickRate();

	if (IsValid(NinjaLiveOwner) && NinjaLiveOwner->MyOverlapBasedInteraction)
	{
		NinjaLiveOwner->MyInitialOverlapCheck();
		NinjaLiveOwner->MyBeginOverlapDetection();
		NinjaLiveOwner->MyEndOverlapDetection();
	}
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


		if (!MyAfterTickDelayRearmDoOnceClosed)
		{
			MyAfterTickDelayRearmDoOnceClosed = true;
			MyAfterTickDelayDeactivateDoOnceClosed = false;
			if (MyUsePAINTER_V2_ToTrackObjects && IsValid(MyNiagaraBasedPainter)
				&& !MyNiagaraBasedPainter->IsActive())
			{
				MyLastForwardedPainterScalarValues.Reset();
				bMyPainterArraysSent = false;
				bMyLastSentPosInterpolValid = false;
				MyNiagaraBasedPainter->Activate(false);
			}
		}
		return bCollisionTimerUpdated;
	}


	if (!MyAfterTickDelayDeactivateDoOnceClosed)
	{
		MyAfterTickDelayDeactivateDoOnceClosed = true;
		if (MyUsePAINTER_V2_ToTrackObjects && IsValid(MyNiagaraBasedPainter))
		{
			MyNiagaraBasedPainter->Deactivate();

			MyAfterTickDelayRearmDoOnceClosed = false;
		}
	}

	return false;
}

void UMyNinjaLiveComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (MyDisableComponent)
	{
		return;
	}

	if (MyUseUnrealNativeEventTick)
	{

		if (!MyNativeTickLimiterDoOnceClosed)
		{
			MyNativeTickLimiterDoOnceClosed = true;
			MyApplySimulationTickRate();
		}
		MyDeltaSeconds = static_cast<double>(DeltaTime);
		if (MyAfterTickDelay(MyDeltaSeconds))
		{
			MyAfterReadyCheck();
		}
		return;
	}


	if (!MyCustomTickLoopStarted)
	{
		MyCustomTickLoopStarted = true;
		if (GetWorld())
		{
			MyNormalizeTimingParameters();
			MyApplySimulationTickRate();
		}
	}
}

void UMyNinjaLiveComponent::MyCustomTick()
{
	MyDeltaSeconds = MyTickRateCustom;
	if (MyAfterTickDelay(MyDeltaSeconds))
	{
		MyAfterReadyCheck();
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

	const int32 SafeMaxSamplingFPS = FMath::Max(MyMaxSamplingFPS, 1);
	const int32 SafeSamplingFPS = FMath::Max(MySamplingFPS, 1);
	const double BaseTexelSizeMultiplier = FMath::Max(
		static_cast<double>(UKismetMathLibrary::Divide_IntInt(SafeMaxSamplingFPS, SafeSamplingFPS)) * 0.5, 1.0) * MySpeed;
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
					const int32 SafeMaxFPS = FMath::Max(MyMaxSamplingFPS, 1);
					const int32 SafeSamplingFPS = FMath::Max(MySamplingFPS, 1);
					const double CurrentMultiplier = MyHalfResPressureAndDivergenceBuffers
						? 1.0
						: FMath::Max(static_cast<double>(UKismetMathLibrary::Divide_IntInt(
							SafeMaxFPS, SafeSamplingFPS)) * 0.5, 1.0) * MySpeed;
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

bool UMyNinjaLiveComponent::MyCheckComponentOwner(AMyNinjaLiveActor*& AsNinjaLive) const
{

	AsNinjaLive = nullptr;

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->IsA<AMyNinjaLiveActor>())
	{
		return false;
	}

	AsNinjaLive = Cast<AMyNinjaLiveActor>(OwnerActor);
	return true;
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

void UMyNinjaLiveComponent::MyProximityActivationMasterVarsQuantizerOutMat()
{


	if ((int32)MyTraceMeshMovingInWorldSpace > 3 && MyCameraFacingTraceMesh)
	{
		MyTraceMeshMovingInWorldSpace = EMyQuantizerMode::NoQuantizerTextureOffsetAutomatic;
	}


	if (MyUsePAINTER_V2_ToTrackObjects && !MySingleTargetMode_LEGACY)
	{
		MyEnablePainterDoubleBuffering = true;
	}


	MyInitDone = false;
	MyMaterialInstacesDone = false;


	MyPresetNameFilterCriteria = FName(TEXT("NinjaLive"));


	MyUE5EAFLAG = !FEngineVersion::Current().ToString().Contains(TEXT("EarlyAccess"));


	MyQuantizerStepSize = MyQuantizerValues(MyTraceMeshMovingInWorldSpace);


	if (MyOutputMaterials.Num() == 0)
	{
		MyOutputMaterials.Add(nullptr);
	}
}

void UMyNinjaLiveComponent::MyProximityActivationMasterVarsQuantizerOutMatFromOwner()
{



	AMyNinjaLiveActor* NinjaLive = nullptr;
	if (MyCheckComponentOwner(NinjaLive))
	{

		MyDisableComponent = NinjaLive->MyDisableBlueprint;
		MyComponentActivatedByPawnProximity = NinjaLive->MySimActivatedByPawnProximity;


		if (MyDisableComponent)
		{
			return;
		}

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

	MyProximityActivationMasterVarsQuantizerOutMat();
}

void UMyNinjaLiveComponent::MyLightDirectionProviderCheck()
{


	if (!MyEnableRayMarching)
	{
		return;
	}


	if (IsValid(MyLightDirectionProvider))
	{
		return;
	}


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

	if (!IsValid(MyMIOutput) || !IsValid(MyTraceMeshComponent.Get()))
	{
		return;
	}


	FVector CameraPos = FVector::ZeroVector;
	if (const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		CameraPos = CameraManager->GetCameraLocation();
	}
	const FVector TraceMeshPos = MyTraceMeshComponent->GetComponentLocation();
	const FVector ViewNormal = (TraceMeshPos - CameraPos).GetSafeNormal();


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


	const double BlendAlpha = FMath::Pow(FMath::Max(MyFacing, 0.0), MyTwoSideBlendPow);
	const double ZMask = MyTwoSidedShading
		? FMath::Lerp(-1.0, 1.0, BlendAlpha)
		: 1.0;
	const FLinearColor LightDirectionColor(FVector(LightDir.X * 1.0, LightDir.Y * 1.0, LightDir.Z * ZMask));


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


	MyFacing = FVector::DotProduct(
		FRotationMatrix(MyTraceMeshComponent->GetComponentRotation()).GetUnitAxis(EAxis::Z), ViewNormal) * -1.0;
}

void UMyNinjaLiveComponent::MyTraceChannelAutoFind()
{

	MyTraceChannelsSet = false;


	if (MyPreferredTraceChannelName.IsEmpty())
	{
		MyPreferredTraceChannelName = TEXT("FluidTrace");
	}


	{
		const UEnum* TraceTypeEnum = StaticEnum<ETraceTypeQuery>();
		if (TraceTypeEnum)
		{
			const FString CurrentName = TraceTypeEnum->GetNameStringByValue(MyTraceChannel);
			if (CurrentName == MyPreferredTraceChannelName)
			{

				goto CollisionChannelSearch;
			}
		}
	}


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


	MyTraceChannelsSet = true;
}

void UMyNinjaLiveComponent::MySceneCapCameraVSInputMaterials()
{

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

void UMyNinjaLiveComponent::MyFPSPrecisionResolution()
{

	if (MyResolutionX < 8 || MyResolutionY < 8)
	{
		MyResolutionX = 256;
		MyResolutionY = 256;
	}

	MyNormalizeTimingParameters();


	MySamplingFPS = MyMaxSamplingFPS;
	MyTickRateCustom = 1.0 / static_cast<double>(MyMaxSamplingFPS);

	MySimPrecisionIndex = (MySimPrecision == EMySimPrecision::Bit32) ? 1 : 0;


	MyPV2_Interpolation = MyPV2_Connect_TrackpointsWithLines || MyPV2_GenerateVelocity;
	MyPV2_Connect_TrackpointsWithLines = MyPV2_Interpolation;
}

void UMyNinjaLiveComponent::MyNormalizeTimingParameters()
{
	MyMaxSamplingFPS = FMath::Max(MyMaxSamplingFPS, 1);
	MyMinSamplingFPS = FMath::Clamp(MyMinSamplingFPS, 1, MyMaxSamplingFPS);
	MySamplingFPS = FMath::Clamp(MySamplingFPS, MyMinSamplingFPS, MyMaxSamplingFPS);
	MyLODSteps = FMath::Max(MyLODSteps, 1);
	MyLODCheckFrequency = FMath::Max(MyLODCheckFrequency, static_cast<double>(KINDA_SMALL_NUMBER));
	MyTickRateCustom = FMath::Max(MyTickRateCustom, static_cast<double>(KINDA_SMALL_NUMBER));
}

void UMyNinjaLiveComponent::MyApplySimulationTickRate()
{
	MyNormalizeTimingParameters();

	if (MyUseUnrealNativeEventTick)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(MyCustomTickLoopTimer);
		}
		MyCustomTickLoopStarted = false;

		double EffectiveFPS = MyLimitUnrealNativeEventTick > 0.0
			? MyLimitUnrealNativeEventTick : 0.0;
		if (MyLOD2ReduceSamplingFPS)
		{
			EffectiveFPS = EffectiveFPS > 0.0
				? FMath::Min(EffectiveFPS, static_cast<double>(MySamplingFPS))
				: static_cast<double>(MySamplingFPS);
		}

		const float TickInterval = EffectiveFPS > 0.0
			? static_cast<float>(1.0 / EffectiveFPS) : 0.0f;
		if (!FMath::IsNearlyEqual(GetComponentTickInterval(), TickInterval))
		{
			SetComponentTickInterval(TickInterval);
		}
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		MyCustomTickLoopStarted = false;
		return;
	}

	MyCustomTickLoopStarted = true;
	FTimerManager& TimerManager = World->GetTimerManager();
	const float TickInterval = FMath::Max(static_cast<float>(MyTickRateCustom), KINDA_SMALL_NUMBER);
	const float CurrentRate = TimerManager.GetTimerRate(MyCustomTickLoopTimer);
	if (!TimerManager.IsTimerActive(MyCustomTickLoopTimer)
		|| !FMath::IsNearlyEqual(CurrentRate, TickInterval))
	{
		TimerManager.SetTimer(MyCustomTickLoopTimer, this,
			&UMyNinjaLiveComponent::MyCustomTick, TickInterval, true);
	}
}

void UMyNinjaLiveComponent::MyLODDistaceStepsPrecalc()
{
	MyNormalizeTimingParameters();


	MyLODLevel = MyLODSteps;
	if (!MyLOD1ReduceSimQuality && !MyLOD2ReduceSamplingFPS)
	{
		return;
	}

	const int32 LastIndex = MyLODSteps - 1;



	if (LastIndex == 0)
	{
		MyLODStepsArray.Reset();
		MyLODStepRange = 0.0;
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

void UMyNinjaLiveComponent::MyLOD()
{

	if (!MyLOD1ReduceSimQuality && !MyLOD2ReduceSamplingFPS)
	{
		return;
	}


	if (!MyLODDoOnceClosed)
	{
		MyLODDoOnceClosed = true;
		MyCheckLODLevel();
	}

	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		if (!TimerManager.IsTimerActive(MyLODCheckTimer))
		{
			TimerManager.SetTimer(MyLODCheckTimer, this,
				&UMyNinjaLiveComponent::MyCheckLODLevel,
				FMath::Max(static_cast<float>(MyLODCheckFrequency), KINDA_SMALL_NUMBER), false);
		}
	}
}

void UMyNinjaLiveComponent::MyCheckLODLevel()
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}


	MyNormalizeTimingParameters();

	const double Distance = Owner->GetDistanceTo(UGameplayStatics::GetPlayerCameraManager(this, 0));
	const double LODStepsD = static_cast<double>(MyLODSteps);

	if (Distance < MyLODNearBound)
	{

		MyLODLevel = MyLODSteps;
		MyFluidSolver1Iterations = MyPressureSolver1MaxIterations;
	}
	else if (Distance > MyLODFarBound)
	{

		MyLODLevel = 1;
		MyFluidSolver1Iterations = 1;
	}
	else
	{

		for (int32 Index = 0; Index < MyLODStepsArray.Num(); ++Index)
		{
			const double Step = MyLODStepsArray[Index];
			if (Distance >= Step && Distance <= Step + MyLODStepRange)
			{
				MyLODLevel = MyLODSteps - (Index + 1);
				MyFluidSolver1Iterations = FMath::Max(
					FMath::RoundToInt(MyPressureSolver1MaxIterations * (MyLODLevel / LODStepsD)), 1);
			}
		}
	}

	if (MyLOD2ReduceSamplingFPS)
	{

		const double SamplingBase = FMath::Max(
			static_cast<double>(MyMinSamplingFPS),
			MyMaxSamplingFPS * (MyLODLevel / LODStepsD));
		MySamplingFPS = FMath::TruncToInt(SamplingBase);
		MyTickRateCustom = 1.0 / SamplingBase;
		MyApplySimulationTickRate();
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

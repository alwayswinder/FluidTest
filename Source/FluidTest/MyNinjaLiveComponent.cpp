// MyNinjaLiveComponent.cpp — 生命周期、调度与空间状态

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
	// 启用 Tick（对应蓝图 ReceiveTick 事件，逻辑见 TickComponent）
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
	MyLOD();
	MyMuteBrush();
	MyCameraFacing();

	// 蓝图中 SetScalarParameterValue 同时连到线/点两个 Painter 材质（双 target），C++ 分别设置。
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
		// Sequence 的两路输出：先 MousePassTrue 再 MousePassFalse（保留蓝图原连接）。
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
		// 原生分支的 DoOnce：首次 tick 时按 LimitUnrealNativeEventTick 限制组件 Tick 频率。
		if (!MyNativeTickLimiterDoOnceClosed)
		{
			MyNativeTickLimiterDoOnceClosed = true;
			SetComponentTickInterval(MyLimitUnrealNativeEventTick > 0.0
				? static_cast<float>(1.0 / MyLimitUnrealNativeEventTick)
				: 0.0f);
		}
		MyDeltaSeconds = static_cast<double>(DeltaTime);
		if (MyAfterTickDelay(MyDeltaSeconds))
		{
			MyAfterReadyCheck();
		}
		return;
	}

	// 非原生分支：首次 tick 启动自定义循环（间隔 MyTickRateCustom），之后由 MyCustomTick 回调驱动。
	if (!MyCustomTickLoopStarted)
	{
		MyCustomTickLoopStarted = true;
		if (UWorld* World = GetWorld())
		{
			MyNormalizeTimingParameters();
			World->GetTimerManager().SetTimer(MyCustomTickLoopTimer, this,
				&UMyNinjaLiveComponent::MyCustomTick,
				FMath::Max(static_cast<float>(MyTickRateCustom), KINDA_SMALL_NUMBER), true);
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

	// OutMat 数组为空时补一个空占位项，实际材质由后续输出流程填充
	if (MyOutputMaterials.Num() == 0)
	{
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

void UMyNinjaLiveComponent::MyFPSPrecisionResolution()
{
	// 分辨率过小会导致流体材质采样越界，蓝图用 256x256 兜底。
	if (MyResolutionX < 8 || MyResolutionY < 8)
	{
		MyResolutionX = 256;
		MyResolutionY = 256;
	}

	MyNormalizeTimingParameters();

	// 采样频率直接取上限，并将其转换为每次 Tick 的时间间隔。
	MySamplingFPS = MyMaxSamplingFPS;
	MyTickRateCustom = 1.0 / static_cast<double>(MyMaxSamplingFPS);

	MySimPrecisionIndex = (MySimPrecision == EMySimPrecision::Bit32) ? 1 : 0;

	// Painter v2 的速度生成和轨迹连线共用插值开关。
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

void UMyNinjaLiveComponent::MyLODDistaceStepsPrecalc()
{
	MyNormalizeTimingParameters();

	// 蓝图始终先用 LOD-Steps 初始化当前等级；禁用两个降级选项时不触碰既有阈值数据。
	MyLODLevel = MyLODSteps;
	if (!MyLOD1ReduceSimQuality && !MyLOD2ReduceSamplingFPS)
	{
		return;
	}

	const int32 LastIndex = MyLODSteps - 1;

	// 原蓝图以 (Max(FarBound, NearBound) - 1) / (LOD-Steps - 1) 计算步长。
	// 单步没有可定义的分段范围，因此保留数组为空，避免原图的零除未定义结果。
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
	// 两个 LOD 降级选项都关闭时不做任何事（蓝图 if 的 else 未连接）。
	if (!MyLOD1ReduceSimQuality && !MyLOD2ReduceSamplingFPS)
	{
		return;
	}

	// DoOnce：首次进入立即检查一次，之后仅在没有等待中的 Delay 时安排下一次检查。
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

	// 与玩家摄像机的距离（蓝图 GetDistanceTo；相机无效时返回 -1，落入近距离分支）。
	MyNormalizeTimingParameters();

	const double Distance = Owner->GetDistanceTo(UGameplayStatics::GetPlayerCameraManager(this, 0));
	const double LODStepsD = static_cast<double>(MyLODSteps);

	if (Distance < MyLODNearBound)
	{
		// 近距离：取最高等级与压力求解器迭代上限。
		MyLODLevel = MyLODSteps;
		MyFluidSolver1Iterations = MyPressureSolver1MaxIterations;
	}
	else if (Distance > MyLODFarBound)
	{
		// 远距离：等级 1、单次迭代。
		MyLODLevel = 1;
		MyFluidSolver1Iterations = 1;
	}
	else
	{
		// 中间区间：遍历阈值数组，命中分段（距离 ∈ [Step, Step+StepRange]）时更新，最后命中者生效。
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
		// 采样帧率 = Max(MinSamplingFPS, MaxSamplingFPS × LODLevel/LODSteps) 截断，Tick 间隔取倒数。
		const double SamplingBase = FMath::Max(
			static_cast<double>(MyMinSamplingFPS),
			MyMaxSamplingFPS * (MyLODLevel / LODStepsD));
		MySamplingFPS = FMath::TruncToInt(SamplingBase);
		MyTickRateCustom = 1.0 / SamplingBase;
		Owner->SetActorTickInterval(MyTickRateCustom);
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

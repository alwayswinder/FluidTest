

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

void UMyNinjaLiveComponent::MyMuteBrush()
{
	const double BrushActiveValue = (MyOverlap1 || MyMousePressed) ? 1.0 : 0.0;
	MyBrushStrengthTemp1 = (BrushActiveValue * MyBrushStrength) + 0.001;
}

bool UMyNinjaLiveComponent::MyBrushFadeOutTimer() const
{

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

	MySetCompositeEraserSwitch();
}

void UMyNinjaLiveComponent::MySetCompositeEraserSwitch()
{
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

	MySetCompositeEraserSwitch();
}

bool UMyNinjaLiveComponent::MyBrushSwitch2(FLinearColor InLinearColor) const
{

	const auto IsAtCanvasEdge = [](const FLinearColor& Color)
	{
		return Color.R < 0.05 || Color.R > 0.95 || Color.G < 0.05 || Color.G > 0.95;
	};

	return MyPosition1_3D_Static
		|| !(MyContinuousInteractionWithOwnerActor || MyOverlap1)
		|| IsAtCanvasEdge(InLinearColor)
		|| IsAtCanvasEdge(MyLastPosition2_2D);
}

bool UMyNinjaLiveComponent::MyBrushSwitch1(FLinearColor InLinearColor) const
{

	const auto IsAtEdge = [](const FVector& Pos)
	{
		return Pos.X < 0.05 || Pos.X > 0.95 || Pos.Y < 0.05 || Pos.Y > 0.95;
	};


	const FVector CurrentPos1(InLinearColor.R, InLinearColor.G, InLinearColor.B);
	const FLinearColor LastColor = MyLastPosition3_2D.IsValidIndex(MyTouchLookupIndex)
		? MyLastPosition3_2D[MyTouchLookupIndex]
		: FLinearColor::Black;
	const FVector LastPos1(LastColor.R, LastColor.G, LastColor.B);

	return IsAtEdge(CurrentPos1)
		|| IsAtEdge(LastPos1)
		|| CurrentPos1.Equals(LastPos1, 0.001)
		|| LastPos1.Equals(FVector::ZeroVector, 0.001)
		|| MyTimeSinceLastClick < (1.0 / static_cast<double>(FMath::Max(MySamplingFPS, 1))) * 1.5;
}

FLinearColor UMyNinjaLiveComponent::MyBrushRnd3(const FLinearColor InColor) const
{
	if (MyPV2_GenerateVelocity)
	{
		return InColor;
	}


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

	return MyBrushRnd3(InColor);
}

FLinearColor UMyNinjaLiveComponent::MyBrushRnd1(const FLinearColor InColor) const
{

	return MyBrushRnd3(InColor);
}

void UMyNinjaLiveComponent::MyResetTempArrays()
{
	for (TArray<FName>& TempArray : MyTempArrays)
	{
		TempArray.Reset();
	}
}

TArray<FName> UMyNinjaLiveComponent::MyGetTempArray(int32 Index) const
{
	if (Index >= 0 && Index < MyTempArrayCount)
	{
		return MyTempArrays[Index];
	}

	return TArray<FName>();
}

TArray<FName>& UMyNinjaLiveComponent::MyGetTempArrayRef(int32 Index)
{
	if (Index >= 0 && Index < MyTempArrayCount)
	{
		return MyTempArrays[Index];
	}


	MyInvalidTempArray.Reset();
	return MyInvalidTempArray;
}

void UMyNinjaLiveComponent::MyAddToTempArray(int32 ArrayIndex, FName Item)
{
	MyGetTempArrayRef(ArrayIndex).Add(Item);
}

void UMyNinjaLiveComponent::MyClearTempArray(int32 ArrayIndex)
{
	MyGetTempArrayRef(ArrayIndex).Empty();
}

void UMyNinjaLiveComponent::MyAppendToTempArray(int32 ArrayIndex, const TArray<FName>& Items)
{
	MyGetTempArrayRef(ArrayIndex).Append(Items);
}

void UMyNinjaLiveComponent::MyResetTempArraySlots()
{
	MyResetTempArrays();
	MyListOfAvailableTempArrays.Init(true, MyTempArrayCount);
	MySkeletalMeshTempArrayPairs.Reset();
}

int32 UMyNinjaLiveComponent::MyAcquireTempArraySlot()
{
	const int32 ArrayIndex = MyListOfAvailableTempArrays.Find(true);
	if (ArrayIndex != INDEX_NONE)
	{
		MyClearTempArray(ArrayIndex);
		MyListOfAvailableTempArrays[ArrayIndex] = false;
	}
	return ArrayIndex;
}

void UMyNinjaLiveComponent::MyReleaseTempArraySlot(int32 ArrayIndex)
{

	if (!MyListOfAvailableTempArrays.IsValidIndex(ArrayIndex) || MyListOfAvailableTempArrays[ArrayIndex])
	{
		return;
	}

	MyClearTempArray(ArrayIndex);
	MyListOfAvailableTempArrays[ArrayIndex] = true;
	MySkeletalMeshTempArrayPairs.Remove(ArrayIndex);
}

void UMyNinjaLiveComponent::MyVelocityHandlerForSimArea(double CoEff, double& X, double& Y, double& Z) const
{

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
	Y = Velocity.Y * -1.0;
	Z = Velocity.Z;
}

void UMyNinjaLiveComponent::MyEnableOwnerInput()
{

	if (MyUserInputBasedInteraction == EMyUserInput::None)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}


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

FVector UMyNinjaLiveComponent::MyDefineLineTracingSource() const
{

	FVector TraceSource = FVector::ZeroVector;
	if (const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		TraceSource = CameraManager->K2_GetActorLocation();
	}

	if (MyUseCustomTraceSource)
	{


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

void UMyNinjaLiveComponent::MyBuildTraceExcludeActors(TArray<AActor*>& Out) const
{

	Out.Reset(MyNinjaLiveTraceExclude.Num());
	for (const TObjectPtr<AActor>& Excluded : MyNinjaLiveTraceExclude)
	{
		if (IsValid(Excluded))
		{
			Out.Add(Excluded.Get());
		}
	}
}

void UMyNinjaLiveComponent::MyOverlapArtifactWorkaround2(FVector In)
{

	MyApplyTraceArtifactBrushMute(In, 0.1f);
}

void UMyNinjaLiveComponent::MyApplyTraceArtifactBrushMute(FVector TracePosition, float ObjectMoveTolerance)
{

	MyLastTracePositionTemp = MyTracePositionTemp;
	MyTracePositionTemp = TracePosition;



	const bool bTraceNotMoving = MyTracePositionTemp.Equals(MyLastTracePositionTemp, 0.1f);
	const bool bObjectMoving = !MyPosition1_3D.Equals(MyLastPosition1_3D, ObjectMoveTolerance);
	const bool bNotContinuousInteraction = !MyContinuousInteractionWithOwnerActor;
	const bool bMuteBrush = bTraceNotMoving && bObjectMoving && bNotContinuousInteraction;

	MyBrushStrengthTemp1 = bMuteBrush ? 0.0 : MyBrushStrength;
}

void UMyNinjaLiveComponent::MyTraceObjects2(FVector Start, FLinearColor& HitUV, bool& ThenExec, bool& NoHitExec)
{
	ThenExec = false;
	NoHitExec = false;


	FVector TracePosition = FVector::ZeroVector;
	bool HitValid = false;
	TArray<AActor*> TraceExclude;
	MyBuildTraceExcludeActors(TraceExclude);
	UMyNinjaLiveFunctions::MyTraceOverlap(
		this, Start, MyPosition1_3D, 1.5, MyTraceChannel,
		TraceExclude, MyUsePAINTER_V2_ToTrackObjects,
		HitUV, TracePosition, HitValid);

	if (!HitValid)
	{
		NoHitExec = true;
		return;
	}


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


	MyPosition1_3D_Static =
		MyPosition1_3D.Equals(MyLastPosition1_3D, 0.001f)
		|| MyLastPosition1_3D.Equals(FVector::ZeroVector, 0.001f);

	MyTimeSinceLastCollision = 0.0;


	if (MyContinuousInteractionWithOwnerActor || MyOverlap1)
	{
		FVector TracePosition = FVector::ZeroVector;
		bool HitValid = false;
		TArray<AActor*> TraceExclude;
		MyBuildTraceExcludeActors(TraceExclude);
		UMyNinjaLiveFunctions::MyTraceOverlap(
			this, Start, MyPosition1_3D, 1.5, MyTraceChannel,
			TraceExclude, false,
			HitUV, TracePosition, HitValid);


		MyApplyTraceArtifactBrushMute(TracePosition, 0.001f);
	}
}

bool UMyNinjaLiveComponent::MyTraceGestures(FLinearColor& HitUV)
{
	HitUV = FLinearColor::Black;
	TArray<AActor*> TraceExclude;
	MyBuildTraceExcludeActors(TraceExclude);

	auto TraceInput = [this, &HitUV, &TraceExclude](uint8 FingerIndex)
	{
		FLinearColor TraceHitUV = FLinearColor::Black;
		bool bSimHitByMouse = false;
		bool bMouseClickValid = false;
		bool bTouchValid = false;
		UMyNinjaLiveFunctions::MyTraceMouse(
			this,
			MyTraceMeshComponent.Get(),
			MyTouch,
			FingerIndex,
			MyTraceChannel,
			TraceExclude,
			TraceHitUV,
			bSimHitByMouse,
			bMouseClickValid,
			bTouchValid);
		if (bSimHitByMouse)
		{
			HitUV = TraceHitUV;
		}
		return bSimHitByMouse;
	};

	if (MySingleInput)
	{
		return TraceInput(0);
	}

	AMyNinjaLiveActor* NinjaLive = nullptr;
	if (!MyCheckComponentOwner(NinjaLive) || !IsValid(NinjaLive))
	{
		return false;
	}

	bool bReachedOut = false;
	for (int32 Index = 0; Index < NinjaLive->MyMultipleTouchLookup.Num(); ++Index)
	{
		MyTouchLookupIndex = Index;
		if (NinjaLive->MyMultipleTouchLookup[Index])
		{
			bReachedOut |= TraceInput(static_cast<uint8>(Index));
		}
	}

	return bReachedOut;
}

void UMyNinjaLiveComponent::MyMousePassTrue()
{
	MyMousePass = true;

	FLinearColor HitUV;
	if (!MyTraceGestures(HitUV))
	{
		return;
	}

	MyOverlappingMeshSizeCoeff = MyUserInputBrushScale;



	if (MyUsePAINTER_V2_ToTrackObjects && !MySingleTargetMode_LEGACY)
	{
		const FLinearColor RandomColor = MyBrushRnd1(HitUV);
		MyPositionArray.Add(FVector2D(RandomColor.R, RandomColor.G));

		const FLinearColor Gradient(RandomColor.R - MyLastMouseHitUV_2D.R,
			RandomColor.G - MyLastMouseHitUV_2D.G, 0.0f, 0.0f);
		MyVelocityArray.Add(Gradient * 15.0f);
		MyBrushSizeArray.Add(static_cast<float>(MyBrushSizeCoEff()));
		MyLastMouseHitUV_2D = RandomColor;
		return;
	}

	if (!MyPosition3_2D.IsValidIndex(MyTouchLookupIndex) || !MyLastPosition3_2D.IsValidIndex(MyTouchLookupIndex))
	{
		return;
	}


	const FLinearColor RandomColor = MyBrushRnd1(HitUV);
	MyLastPosition3_2D[MyTouchLookupIndex] = FMath::Lerp(
		MyPosition3_2D[MyTouchLookupIndex],
		RandomColor,
		MyBrushSwitch1(HitUV) ? 1.0f : 0.0f);
	MyPosition3_2D[MyTouchLookupIndex] = RandomColor;
	MyLastMouseHitUV_2D = RandomColor;

	MyPaintLine();
}

void UMyNinjaLiveComponent::MyMousePassFalse()
{
	MyMousePass = false;

	if (!MyOverlapBasedInteraction && !MyContinuousInteractionWithOwnerActor)
	{
		MyNoInteraction();
		return;
	}

	const int32 OverlapCount = MyOverlappingComponents.Num() + MySkeletalMeshTempArrayPairs.Num();
	if (OverlapCount == 0)
	{
		MyNoInteraction();
		return;
	}

	if (MySingleTargetMode_LEGACY)
	{
		MySingleTargetMode();
	}
	else
	{
		MyForLoopOverlapping();
	}
}

void UMyNinjaLiveComponent::MySingleTargetMode()
{
	auto TraceSingleTarget = [this]()
	{
		FLinearColor HitUV = FLinearColor::Black;
		MyTraceObjects1(MyDefineLineTracingSource(), HitUV);


		const FLinearColor LastPositionCandidate = MyBrushRnd2(HitUV);
		MyLastPosition2_2D = FMath::Lerp(
			MyPosition2_2D,
			LastPositionCandidate,
			MyBrushSwitch2(HitUV) ? 1.0f : 0.0f);
		MyPosition2_2D = MyBrushRnd2(HitUV);
		MyPaintLine();
	};
	const auto DidCalcPos1ReachOutput = [this]()
	{
		if (!MyContinuousInteractionWithOwnerActor)
		{
			AMyNinjaLiveActor* NinjaLive = nullptr;
			if (!MyCheckComponentOwner(NinjaLive) || !IsValid(NinjaLive))
			{
				return false;
			}
		}

		if (!MyBrushScaledByInteractingObjSize)
		{
			return true;
		}

		TArray<TObjectPtr<UPrimitiveComponent>> SkeletalMeshValues;
		MySkeletalMeshTempArrayPairs.GenerateValueArray(SkeletalMeshValues);
		return SkeletalMeshValues.IsValidIndex(0) &&
			IsValid(Cast<USkeletalMeshComponent>(SkeletalMeshValues[0]));
	};

	if (MySingleTargetType_LEGACY == EMySingleObjectType::SkeletalMeshBone)
	{

		if (!MySkeletalMeshTempArrayPairs.IsEmpty())
		{
			TArray<TObjectPtr<UPrimitiveComponent>> SkeletalMeshValues;
			MySkeletalMeshTempArrayPairs.GenerateValueArray(SkeletalMeshValues);
			if (SkeletalMeshValues.IsValidIndex(MySingleTargetModeSkeletalMeshIndex_LEGACY) &&
				IsValid(SkeletalMeshValues[MySingleTargetModeSkeletalMeshIndex_LEGACY]))
			{
				MyCalcPos1(SkeletalMeshValues[MySingleTargetModeSkeletalMeshIndex_LEGACY]);

				if (DidCalcPos1ReachOutput())
				{
					TraceSingleTarget();
				}
			}
		}
	}
	else
	{
		UPrimitiveComponent* SingleTarget = nullptr;
		bool bTargetValid = false;
		MyCheckValidity2(SingleTarget, bTargetValid);
		if (bTargetValid)
		{
			MyOverlappingComponent = SingleTarget;
			MyCalcPos2(MyOverlappingComponent.Get());
			TraceSingleTarget();
		}
	}


	MyFluidCoreStep();
}

void UMyNinjaLiveComponent::MyTraceObj2()
{
	FLinearColor HitUV = FLinearColor::Black;
	bool bHit = false;
	bool bNoHit = false;
	MyTraceObjects2(MyDefineLineTracingSource(), HitUV, bHit, bNoHit);


	if (bHit)
	{
		MyPosition1_2D = MyBrushRnd3(HitUV);
		MySetBrushDensityParams3(MyBrushSizeCoEff());


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


	if (bNoHit)
	{
		MyTemporarilySwitchOffLineDrawingIFTracerFails();
	}
}

void UMyNinjaLiveComponent::MyMultiObjectProcessorCycle3()
{

	for (const TPair<int32, TObjectPtr<UPrimitiveComponent>>& SkeletalMeshTempArrayPair : MySkeletalMeshTempArrayPairs)
	{
		const TArray<FName>& Bones = MyGetTempArrayRef(SkeletalMeshTempArrayPair.Key);
		for (const FName Bone : Bones)
		{
			MyOverlappingBone = Bone;
			MyOverlappingSkeletalMesh = SkeletalMeshTempArrayPair.Value;
			MyBuildOverlapSKMArray(MyOverlappingSkeletalMesh.Get());
			MyCalcPos5();
			MyTraceObj2();
		}
	}


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


		MyCalcPos3();
		MyTraceObj2();
	}


	MyMultiObjectProcessorCycle3();
}

void UMyNinjaLiveComponent::MyNoInteraction()
{

	for (UMaterialInstanceDynamic* PainterMaterial : { MyMICollisionPainterLine.Get(), MyMICollisionPainterDot.Get() })
	{
		if (IsValid(PainterMaterial))
		{
			PainterMaterial->SetScalarParameterValue(TEXT("BrushStrength"), 0.0f);
		}
	}


	if (!MyBrushFadeOutTimer())
	{
		MyFinalDealRTAndBrush();
	}


	MyFluidCoreStep();
}

void UMyNinjaLiveComponent::MyTemporarilySwitchOffLineDrawingIFTracerFails()
{

	const bool bShouldSwitchOff = MyUsePAINTER_V2_ToTrackObjects
		&& !MySingleTargetMode_LEGACY
		&& MyPV2_Connect_TrackpointsWithLines;
	if (!bShouldSwitchOff)
	{
		return;
	}

	MyHitValid = false;


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

void UMyNinjaLiveComponent::MyBuildOverlapSKMArray(UPrimitiveComponent* In)
{

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

	Out = 0.0;
	USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(MyOverlappingSkeletalMesh.Get());
	if (!IsValid(SkeletalMesh))
	{
		return;
	}


	const FName ParentBone = SkeletalMesh->GetParentBone(MyOverlappingBone);
	const FVector ParentBoneWorldPos = SkeletalMesh->GetSocketLocation(ParentBone);
	const double BoneDistance = FMath::Abs((In - ParentBoneWorldPos).Size());
	Out = BoneDistance * 0.01 * BrushScaleMult;
}

void UMyNinjaLiveComponent::MyCalcPos5()
{

	MyLastPosition1_3D = MyPosition1_3D;
	MyPosition1_3D = IsValid(MyOverlappingSkeletalMesh.Get())
		? MyOverlappingSkeletalMesh->GetSocketLocation(MyOverlappingBone)
		: FVector::ZeroVector;


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

	MyLastPosition1_3D = MyPosition1_3D;
	MyPosition1_3D = IsValid(MyOverlappingComponent.Get())
		? MyOverlappingComponent->K2_GetComponentLocation()
		: FVector::ZeroVector;


	MyOverlappingMeshSizeCoeff = MyCalculateBrushSizeCoFromBounds1(MyOverlappingComponent.Get());

	MyPosDataType = 0;
}

void UMyNinjaLiveComponent::MyCalcPos2(UObject* In)
{

	if (!IsValid(In))
	{
		return;
	}


	MyLastPosition1_3D = MyPosition1_3D;
	MyPosition1_3D = IsValid(MyOverlappingComponent.Get())
		? MyOverlappingComponent->K2_GetComponentLocation()
		: FVector::ZeroVector;


	MyOverlappingMeshSizeCoeff = MyCalculateBrushSizeCoFromBounds1(MyOverlappingComponent.Get());
}

void UMyNinjaLiveComponent::MyCalcPos1(USceneComponent* Component)
{
	if (!IsValid(Component))
	{
		return;
	}

	const TArray<FName>* BoneNames = &MyContinuousInteractionBoneNamesExact;
	if (!MyContinuousInteractionWithOwnerActor)
	{
		AMyNinjaLiveActor* NinjaLive = nullptr;
		if (!MyCheckComponentOwner(NinjaLive) || !IsValid(NinjaLive))
		{
			return;
		}

		BoneNames = &NinjaLive->MyOverlapFilterInclusiveBoneNameExact;
	}


	const FName BoneName = BoneNames->IsValidIndex(0) ? (*BoneNames)[0] : NAME_None;
	MyLastPosition1_3D = MyPosition1_3D;
	MyPosition1_3D = Component->GetSocketLocation(BoneName);

	if (!MyBrushScaledByInteractingObjSize)
	{
		MyOverlappingMeshSizeCoeff = 1.0;
		return;
	}


	TArray<TObjectPtr<UPrimitiveComponent>> SkeletalMeshValues;
	MySkeletalMeshTempArrayPairs.GenerateValueArray(SkeletalMeshValues);
	USkeletalMeshComponent* SkeletalMesh = SkeletalMeshValues.IsValidIndex(0)
		? Cast<USkeletalMeshComponent>(SkeletalMeshValues[0])
		: nullptr;
	if (!IsValid(SkeletalMesh))
	{
		return;
	}

	const FVector BoneLocation = SkeletalMesh->GetSocketLocation(BoneName);
	const FVector ParentBoneLocation = SkeletalMesh->GetSocketLocation(SkeletalMesh->GetParentBone(BoneName));
	MyOverlappingMeshSizeCoeff = FVector::Distance(BoneLocation, ParentBoneLocation) * 0.01 *
		MySkeletalMeshBrushScale;
}

double UMyNinjaLiveComponent::MyCalculateBrushSizeCoFromBounds1(USceneComponent* Component) const
{

	if (!MyBrushScaledByInteractingObjSize)
	{
		return 1.0;
	}


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


	const double MinElement = FMath::Min(DataSource.X, FMath::Min(DataSource.Y, DataSource.Z));
	return MinElement * MyPrimitiveObjBrushScale * 0.01;
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
	MyResetTempArraySlots();

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

		const int32 TempArrayIndex = MyAcquireTempArraySlot();
		if (TempArrayIndex == INDEX_NONE)
		{
			break;
		}

		MyAppendToTempArray(TempArrayIndex, MatchedBones);
		MySkeletalMeshTempArrayPairs.Add(TempArrayIndex, SkeletalComponent);
	}

	MyContinuousInteractionBoneNamesExactTemp2 = MyContinuousInteractionBoneNamesExactTemp;
}

void UMyNinjaLiveComponent::MyCheckValidity2(UPrimitiveComponent*& SingleTarget, bool& ThenExec)
{
	ThenExec = false;
	SingleTarget = nullptr;


	if (MyOverlappingComponents.Num() == 0)
	{
		return;
	}


	if (MyContinuousInteractionComponentNamesExact.Num() == 0)
	{
		SingleTarget = MyOverlappingComponents[0].Get();
		ThenExec = true;
		return;
	}


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

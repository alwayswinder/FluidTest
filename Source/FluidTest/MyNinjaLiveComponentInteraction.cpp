// MyNinjaLiveComponentInteraction.cpp — 输入、追踪与画笔交互

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

bool UMyNinjaLiveComponent::MyBrushSwitch1(FLinearColor InLinearColor) const
{
	// 位置是否落在画布边缘（X/Y 接近 0 或 1）。
	const auto IsAtEdge = [](const FVector& Pos)
	{
		return Pos.X < 0.05 || Pos.X > 0.95 || Pos.Y < 0.05 || Pos.Y > 0.95;
	};

	// LinearColor 按 R/G/B 转 Vector（与蓝图 Conv_LinearColorToVector 一致）；索引越界时按零点处理。
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
	// 对应 BrushRnd2：随机抖动逻辑与 BrushRnd3 相同，仅蓝图节点不同。
	return MyBrushRnd3(InColor);
}

FLinearColor UMyNinjaLiveComponent::MyBrushRnd1(const FLinearColor InColor) const
{
	// 对应 BrushRnd1：随机抖动逻辑与 BrushRnd3 相同，仅蓝图节点不同。
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

	// 越界访问保持原蓝图兼容行为：返回一个可写但不会进入有效槽位的空占位数组。
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
	// 越界或槽位本就空闲时直接返回，避免重复释放误删其他占用者的骨骼网格映射。
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

void UMyNinjaLiveComponent::MyBuildTraceExcludeActors(TArray<AActor*>& Out) const
{
	// 排除同类 NinjaLive 实例，避免流体彼此追踪自身。
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
	// 越界修复入口：用 0.1 的容差判定物体是否在移动。
	MyApplyTraceArtifactBrushMute(In, 0.1f);
}

void UMyNinjaLiveComponent::MyApplyTraceArtifactBrushMute(FVector TracePosition, float ObjectMoveTolerance)
{
	// 保存上一帧追踪位置（此时 TracePositionTemp 仍是旧值），再更新为本帧输入。
	MyLastTracePositionTemp = MyTracePositionTemp;
	MyTracePositionTemp = TracePosition;

	// 越界判定：追踪位置未变（物体停在边缘）、物体自身在移动、且非持续交互模式时，
	// FluidTrace 无法生成有效 UV，静音画笔避免伪影。
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

	// 从 Start 到物体位置做追踪；命中输出 UV 并走 then 分支，否则走 NoHit 分支。
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
		MyBuildTraceExcludeActors(TraceExclude);
		UMyNinjaLiveFunctions::MyTraceOverlap(
			this, Start, MyPosition1_3D, 1.5, MyTraceChannel,
			TraceExclude, false,
			HitUV, TracePosition, HitValid);

		// 物体跨出模拟平面边缘仍保持重叠时 FluidTrace 失效、无法生成有效 UV，静音画笔避免伪影（此处用 0.001 容差）。
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

	// Painter v2 且非旧版单目标：鼠标命中点追加进 Niagara 追踪数组，与人物共用通道，
	// 避免用 Line 画笔整幅覆盖 RT_Painter 清掉人物笔刷（原蓝图序列两路并行的意图）。
	if (MyUsePAINTER_V2_ToTrackObjects && !MySingleTargetMode_LEGACY)
	{
		const FLinearColor RandomColor = MyBrushRnd1(HitUV);
		MyPositionArray.Add(FVector2D(RandomColor.R, RandomColor.G));
		// 速度沿用上一帧鼠标位置差分（比例与 MySingleTargetVelocity 一致）。
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

	// 随机化色只求值一次（蓝图 MyBrushRnd1 单节点输出同时用于 Lerp.B 与 Position3_2D 写回）。
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

		// 保留蓝图中两个独立 BrushRnd2 节点的随机采样。
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
		// Map_Length != 0 后才读取 Map_Values[Index]；越界索引和无效对象均不进入追踪分支。
		if (!MySkeletalMeshTempArrayPairs.IsEmpty())
		{
			TArray<TObjectPtr<UPrimitiveComponent>> SkeletalMeshValues;
			MySkeletalMeshTempArrayPairs.GenerateValueArray(SkeletalMeshValues);
			if (SkeletalMeshValues.IsValidIndex(MySingleTargetModeSkeletalMeshIndex_LEGACY) &&
				IsValid(SkeletalMeshValues[MySingleTargetModeSkeletalMeshIndex_LEGACY]))
			{
				MyCalcPos1(SkeletalMeshValues[MySingleTargetModeSkeletalMeshIndex_LEGACY]);
				// CalcPos1 的 Owner/Cast Failed 分支没有连接输出执行引脚。
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

	// ExecutionSequence 的 then_1：无论单目标分支是否得到有效对象，都推进一次流体模拟。
	MyFluidCoreStep();
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

	// 蓝图 Get(Array Item 0) 在数组为空时返回 None，SocketLocation 随之回退到组件位置。
	const FName BoneName = BoneNames->IsValidIndex(0) ? (*BoneNames)[0] : NAME_None;
	MyLastPosition1_3D = MyPosition1_3D;
	MyPosition1_3D = Component->GetSocketLocation(BoneName);

	if (!MyBrushScaledByInteractingObjSize)
	{
		MyOverlappingMeshSizeCoeff = 1.0;
		return;
	}

	// Map Values[0] 与蓝图 Map_Values 接入 Get(Array Item 0) 保持一致。
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

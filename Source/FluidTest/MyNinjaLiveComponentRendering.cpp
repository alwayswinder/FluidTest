// MyNinjaLiveComponentRendering.cpp — RenderTarget、材质与 Niagara

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

void UMyNinjaLiveComponent::MyInitPainterV2()
{
	MyDestroyPainterV2();

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

	// 前一轮实例已清理；每次初始化创建独立 Niagara 实例，避免运行时参数残留。
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

	if (MyForceMaxSamplingFPSToNiagara)
	{
		// Solo 模式使 Niagara 以流体模拟指定的采样频率独立更新。
		MyNiagaraBasedPainter->SetForceSolo(true);
		MyNiagaraBasedPainter->SetComponentTickInterval(1.0 / static_cast<double>(FMath::Max(MyMaxSamplingFPS, 1)));
		MyNiagaraBasedPainter->ReinitializeSystem();
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
	MyApplyOutputMaterialToTraceMesh();
	MyApplyOutputMaterialsToTaggedActors();
}

void UMyNinjaLiveComponent::MyApplyOutputMaterialToTraceMesh()
{
	// 蓝图先按 DisableComponent 和 TraceMeshInvisible 选择 TraceMesh 的显示材质。
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
				NiagaraComponent->SetComponentTickInterval(1.0 / static_cast<double>(FMath::Max(MyMaxSamplingFPS, 1)));
				NiagaraComponent->ReinitializeSystem();
			}
		}
	}
}

void UMyNinjaLiveComponent::MyAfterCreateRT()
{
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

	// 原蓝图接口只标记 NinjaLive 自身；按 Owner 的生成类收集同类实例，避免加载蓝图接口资产。
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

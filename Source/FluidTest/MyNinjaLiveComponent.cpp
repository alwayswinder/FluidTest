// MyNinjaLiveComponent.cpp — UMyNinjaLiveComponent 实现

#include "MyNinjaLiveComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/EngineVersion.h"
#include "MyNinjaLiveActor.h"
#include "FluidTest/MyNinjaLiveFunctions.h"
#include "MyNinjaLiveMemoryPoolManager.h"

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



// MyNinjaLiveFunctions.cpp — UMyNinjaLiveFunctions 实现

#include "MyNinjaLiveFunctions.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/UnrealType.h"

UTextureRenderTarget2D* UMyNinjaLiveFunctions::MyCreateRenderTarget(
	UObject* WorldContextObject,
	int32 Width,
	int32 Height,
	TEnumAsByte<ETextureRenderTargetFormat> Format,
	bool Clamping,
	TEnumAsByte<TextureGroup> LODgroup,
	TEnumAsByte<TextureFilter> Filter)
{
	UTextureRenderTarget2D* RTout = UKismetRenderingLibrary::CreateRenderTarget2D(
		WorldContextObject,
		Width,
		Height,
		Format,
		FLinearColor::Black,
		false,
		false);

	if (!IsValid(RTout))
	{
		return nullptr;
	}

	RTout->AddressX = Clamping ? TA_Clamp : TA_Wrap;
	RTout->AddressY = Clamping ? TA_Clamp : TA_Wrap;
	RTout->Filter = Filter;
	RTout->LODGroup = LODgroup;
	return RTout;
}

void UMyNinjaLiveFunctions::MyTemplateLoader(
	UObject* WorldContextObject,
	FName TemplateDefinition,
	UDataTable* LoadedDataTable,
	const FString& LoadedDatatablePath,
	bool& LoadFailed,
	UObject*& LoadedTemplateObject,
	FString& LoadedTmpFullPath,
	FString& LoadedTemplateNameOnly,
	bool& UsesAbsolutePath)
{
	LoadFailed = true;
	LoadedTemplateObject = nullptr;
	LoadedTmpFullPath.Reset();
	LoadedTemplateNameOnly.Reset();
	UsesAbsolutePath = false;

	if (!IsValid(LoadedDataTable) || TemplateDefinition.IsNone())
	{
		return;
	}

	const uint8* RowData = LoadedDataTable->GetRowMap().FindRef(TemplateDefinition);
	if (RowData == nullptr || LoadedDataTable->RowStruct == nullptr)
	{
		return;
	}

	const FStrProperty* SourceStringProperty = nullptr;
	for (TFieldIterator<FProperty> It(LoadedDataTable->RowStruct); It; ++It)
	{
		if (It->GetFName().ToString().StartsWith(TEXT("SourceString")))
		{
			SourceStringProperty = CastField<FStrProperty>(*It);
			break;
		}
	}

	if (SourceStringProperty == nullptr)
	{
		return;
	}

	const FString& SourceString = SourceStringProperty->GetPropertyValue_InContainer(RowData);
	if (SourceString.IsEmpty())
	{
		return;
	}

	const bool bSourceUsesAbsolutePath = SourceString.StartsWith(TEXT("/"), ESearchCase::IgnoreCase);
	LoadedTmpFullPath = bSourceUsesAbsolutePath
		? SourceString
		: FString::Printf(TEXT("%s/%s"), *LoadedDatatablePath, *SourceString);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> AssetData;
	AssetRegistryModule.Get().GetAssetsByPackageName(FName(*LoadedTmpFullPath), AssetData, false, true);

	if (!AssetData.IsEmpty())
	{
		// 蓝图 ForEachLoop 的首个循环体即进入函数返回节点，只处理第一个资产。
		LoadedTemplateObject = AssetData[0].GetAsset();
	}

	if (!IsValid(LoadedTemplateObject))
	{
		LoadedTemplateObject = nullptr;
		return;
	}

	const int32 LastSeparator = LoadedTmpFullPath.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	const FString PackageDirectory = LastSeparator != INDEX_NONE
		? LoadedTmpFullPath.Left(LastSeparator + 1)
		: FString();
	UsesAbsolutePath = bSourceUsesAbsolutePath && PackageDirectory != LoadedDatatablePath;
	LoadedTemplateNameOnly = UsesAbsolutePath && LastSeparator != INDEX_NONE
		? LoadedTmpFullPath.Mid(LastSeparator + 1)
		: LoadedTmpFullPath;
	LoadFailed = false;
}

void UMyNinjaLiveFunctions::MyPresetLoader(
	UObject* WorldContextObject,
	const FString& PresetName,
	const TArray<FName>& AssetPath,
	FName AssetTrimmedName,
	bool ForcePreferredPreset,
	UDataTable* PreferredPreset,
	UDataTable*& LoadedDataTable,
	FString& LoadedDataTablePath,
	TMap<FString, double>& PresetMap)
{
	LoadedDataTable = nullptr;
	LoadedDataTablePath.Reset();
	PresetMap.Reset();

	if (ForcePreferredPreset)
	{
		LoadedDataTable = PreferredPreset;
	}
	else
	{
		const FString ExpectedAssetName = FString::Printf(TEXT("DT_%s_%s"), *AssetTrimmedName.ToString(), *PresetName);
		FARFilter Filter;
		Filter.PackagePaths = AssetPath;
		Filter.ClassPaths.Add(UDataTable::StaticClass()->GetClassPathName());
		Filter.bRecursivePaths = true;

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		TArray<FAssetData> Assets;
		AssetRegistryModule.Get().GetAssets(Filter, Assets);

		for (const FAssetData& Asset : Assets)
		{
			if (Asset.AssetName.ToString().Contains(ExpectedAssetName))
			{
				LoadedDataTable = Cast<UDataTable>(Asset.GetAsset());
				if (IsValid(LoadedDataTable))
				{
					break;
				}
			}
		}
	}

	if (!IsValid(LoadedDataTable))
	{
		LoadedDataTable = nullptr;
		return;
	}

	const FAssetData LoadedAssetData(LoadedDataTable);
	LoadedDataTablePath = LoadedAssetData.PackagePath.ToString();

	if (LoadedDataTable->RowStruct == nullptr)
	{
		return;
	}

	const FStrProperty* SourceStringProperty = nullptr;
	for (TFieldIterator<FProperty> It(LoadedDataTable->RowStruct); It; ++It)
	{
		if (It->GetFName().ToString().StartsWith(TEXT("SourceString")))
		{
			SourceStringProperty = CastField<FStrProperty>(*It);
			break;
		}
	}

	if (SourceStringProperty == nullptr)
	{
		return;
	}

	FNumberFormattingOptions NumberFormat;
	NumberFormat.SetRoundingMode(ERoundingMode::HalfToEven);
	NumberFormat.SetUseGrouping(false);
	NumberFormat.SetMinimumIntegralDigits(1);
	NumberFormat.SetMaximumIntegralDigits(16);
	NumberFormat.SetMinimumFractionalDigits(0);
	NumberFormat.SetMaximumFractionalDigits(2);

	for (const TPair<FName, uint8*>& RowPair : LoadedDataTable->GetRowMap())
	{
		const FString& SourceString = SourceStringProperty->GetPropertyValue_InContainer(RowPair.Value);
		const double ParsedValue = FCString::Atod(*SourceString);
		const FString RoundedValue = FText::AsNumber(ParsedValue, &NumberFormat).ToString().Replace(TEXT(","), TEXT("."));
		PresetMap.Add(RowPair.Key.ToString(), FCString::Atod(*RoundedValue));
	}
}

void UMyNinjaLiveFunctions::MyCameraFacing(
	UObject* WorldContextObject,
	USceneComponent* InMesh,
	bool UseLegacyFacing,
	bool LockY,
	FRotator TraceMeshInitRot)
{
	if (!IsValid(WorldContextObject) || !IsValid(InMesh))
	{
		return;
	}

	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(WorldContextObject, 0);
	if (!IsValid(CameraManager))
	{
		return;
	}

	const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(
		CameraManager->K2_GetActorLocation(), InMesh->GetComponentLocation());
	const FRotator LegacyFacingRotation = UKismetMathLibrary::ComposeRotators(
		FRotator(0.0, 90.0, 90.0), CameraManager->GetCameraRotation());
	const FRotator LookAtFacingRotation(0.0, LookAtRotation.Yaw + 90.0, LookAtRotation.Pitch + 90.0);
	FRotator FacingRotation = UseLegacyFacing ? LookAtFacingRotation : LegacyFacingRotation;
	if (LockY)
	{
		FacingRotation = UKismetMathLibrary::ComposeRotators(
			FRotator(LookAtRotation.Yaw, 0.0, 0.0), TraceMeshInitRot);
	}

	InMesh->SetWorldRotation(FacingRotation);
}

void UMyNinjaLiveFunctions::MyTraceMouse(
	UObject* WorldContextObject,
	UPrimitiveComponent* HitComponent,
	bool TouchSensitive,
	uint8 FingerIndex,
	TEnumAsByte<ETraceTypeQuery> TraceChannel,
	const TArray<AActor*>& FluidNinjaLIVEActors,
	FLinearColor& HitUV,
	bool& SimHitByMouse,
	bool& MouseClickValid,
	bool& TouchValid)
{
	HitUV = FLinearColor::Black;
	SimHitByMouse = false;
	MouseClickValid = false;
	TouchValid = false;
	if (!IsValid(WorldContextObject) || !IsValid(HitComponent))
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!IsValid(PlayerController))
	{
		return;
	}

	// 原蓝图库始终查询鼠标与指定手指的命中，再按 TouchSensitive 选择其中一项。
	FHitResult MouseHit;
	FHitResult TouchHit;
	MouseClickValid = PlayerController->GetHitResultUnderCursorByChannel(TraceChannel, true, MouseHit);
	TouchValid = PlayerController->GetHitResultUnderFingerByChannel(
		static_cast<ETouchIndex::Type>(FingerIndex), TraceChannel, true, TouchHit);
	const FHitResult& SelectedHit = TouchSensitive ? TouchHit : MouseHit;

	// 仅在所选命中组件就是目标模拟平面时进入原蓝图的追踪分支。
	if (SelectedHit.GetComponent() != HitComponent)
	{
		return;
	}

	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(WorldContextObject, 0);
	if (!IsValid(CameraManager))
	{
		return;
	}

	FHitResult Hit;
	const bool bHit = UKismetSystemLibrary::LineTraceSingle(
		WorldContextObject,
		CameraManager->K2_GetActorLocation(),
		SelectedHit.TraceEnd,
		TraceChannel,
		true,
		FluidNinjaLIVEActors,
		EDrawDebugTrace::None,
		Hit,
		false);
	if (!bHit || !IsValid(Hit.GetActor()))
	{
		return;
	}

	FVector2D UV = FVector2D::ZeroVector;
	UGameplayStatics::FindCollisionUV(Hit, 0, UV);
	HitUV = FLinearColor(UV.X, UV.Y, 0.0f, 1.0f);
	// 到达蓝图库函数返回节点时，原节点输出此比较结果（此处必为 true）。
	SimHitByMouse = true;
}

void UMyNinjaLiveFunctions::MyTraceOverlap(
	UObject* WorldContextObject,
	FVector Start,
	FVector End,
	double TracelineOvershoot,
	TEnumAsByte<ETraceTypeQuery> TraceChannel,
	TArray<AActor*>& FluidNinjaLIVEActors,
	bool PainterV2,
	FLinearColor& HitUV,
	FVector& TracePosition,
	bool& HitValid)
{
	// 输出默认值；无效命中分支会保持这些默认值。
	HitUV = FLinearColor::Black;
	TracePosition = FVector::ZeroVector;
	HitValid = false;

	// 追踪终点沿 Start→End 方向延长 TracelineOvershoot。
	const FVector TraceEnd = End + (End - Start) * TracelineOvershoot;

	// 单次线追踪：复杂碰撞、忽略传入的 Actor 列表、不忽略自身。
	FHitResult Hit;
	const bool bHit = UKismetSystemLibrary::LineTraceSingle(
		WorldContextObject, Start, TraceEnd, TraceChannel,
		true, FluidNinjaLIVEActors, EDrawDebugTrace::None, Hit, false);

	// 命中且 HitActor 有效 → HitValidator=true，否则 false。
	bool HitValidator = false;
	if (bHit && IsValid(Hit.GetActor()))
	{
		HitValidator = true;
	}

	// HitValidator=false 时若 PainterV2=false，蓝图不走 Return 节点（输出保持默认）。
	if (!HitValidator && !PainterV2)
	{
		return;
	}

	// 命中数据：碰撞 UV 转 LinearColor、命中位置、命中有效性。
	FVector2D UV(0.0f, 0.0f);
	UGameplayStatics::FindCollisionUV(Hit, 0, UV);
	HitUV = FLinearColor(UV.X, UV.Y, 0.0f, 1.0f);
	TracePosition = Hit.Location;
	HitValid = HitValidator;
}

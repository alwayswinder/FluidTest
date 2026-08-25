// MyNinjaLiveFunctions.cpp — UMyNinjaLiveFunctions 实现

#include "MyNinjaLiveFunctions.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
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

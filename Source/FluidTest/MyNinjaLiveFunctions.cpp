

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
#include "UObject/SoftObjectPath.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/UnrealType.h"

namespace
{
	const FSoftObjectProperty* MyFindSourceSoftObjectProperty(const UScriptStruct* RowStruct)
	{
		if (RowStruct == nullptr)
		{
			return nullptr;
		}

		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			if (It->GetFName().ToString().StartsWith(TEXT("Source")))
			{
				if (const FSoftObjectProperty* Property = CastField<FSoftObjectProperty>(*It))
				{
					return Property;
				}
			}
		}
		return nullptr;
	}

	const FStrProperty* MyFindSourceStringProperty(const UScriptStruct* RowStruct)
	{
		if (RowStruct == nullptr)
		{
			return nullptr;
		}

		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			if (It->GetFName().ToString().StartsWith(TEXT("SourceString")))
			{
				if (const FStrProperty* Property = CastField<FStrProperty>(*It))
				{
					return Property;
				}
			}
		}
		return nullptr;
	}

	FString MyMakeTemplatePath(const FString& SourcePath, const FString& DataTablePath)
	{
		FString TemplatePath = SourcePath.TrimStartAndEnd();
		if (TemplatePath.Len() >= 2 && TemplatePath.Contains(TEXT("'")) && TemplatePath.EndsWith(TEXT("'")))
		{
			int32 FirstQuote = INDEX_NONE;
			if (TemplatePath.FindChar(TEXT('\''), FirstQuote))
			{
				TemplatePath = TemplatePath.Mid(FirstQuote + 1, TemplatePath.Len() - FirstQuote - 2);
			}
		}

		if (!TemplatePath.StartsWith(TEXT("/"), ESearchCase::IgnoreCase))
		{
			TemplatePath = FString::Printf(TEXT("%s/%s"), *DataTablePath, *TemplatePath);
		}

		return TemplatePath;
	}

	bool MyHasObjectName(const FString& ObjectPath)
	{
		const int32 LastSeparator = ObjectPath.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		const int32 ObjectNameSeparator = ObjectPath.Find(TEXT("."), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		return ObjectNameSeparator > LastSeparator;
	}
}

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

	FSoftObjectPath SourceObjectPath;
	FString SourcePathForDisplay;
	bool bUsesLegacyStringPath = false;
	if (const FSoftObjectProperty* SourceObjectProperty = MyFindSourceSoftObjectProperty(LoadedDataTable->RowStruct))
	{
		const FSoftObjectPtr SourceObject = SourceObjectProperty->GetPropertyValue_InContainer(RowData);
		SourceObjectPath = SourceObject.ToSoftObjectPath();
		SourcePathForDisplay = SourceObjectPath.ToString();
	}

	if (!SourceObjectPath.IsValid())
	{
		bUsesLegacyStringPath = true;
		const FStrProperty* SourceStringProperty = MyFindSourceStringProperty(LoadedDataTable->RowStruct);
		if (SourceStringProperty == nullptr)
		{
			return;
		}

		SourcePathForDisplay = SourceStringProperty->GetPropertyValue_InContainer(RowData);
		if (SourcePathForDisplay.IsEmpty())
		{
			return;
		}
		const FString ResolvedTemplatePath = MyMakeTemplatePath(SourcePathForDisplay, LoadedDatatablePath);
		if (MyHasObjectName(ResolvedTemplatePath))
		{
			SourceObjectPath = FSoftObjectPath(ResolvedTemplatePath);
		}
		else
		{
			FAssetRegistryModule& AssetRegistryModule =
				FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			TArray<FAssetData> Assets;
			AssetRegistryModule.Get().GetAssetsByPackageName(FName(*ResolvedTemplatePath), Assets, false, true);
			if (Assets.Num() == 1)
			{
				SourceObjectPath = Assets[0].ToSoftObjectPath();
			}
			else if (Assets.Num() > 1)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("FluidSim: 模板包 '%s' 包含 %d 个资产；请将 SourceString 改为完整对象路径或 Soft Object Reference。"),
					*ResolvedTemplatePath, Assets.Num());
			}
		}
	}

	if (!SourceObjectPath.IsValid())
	{
		return;
	}

	LoadedTemplateObject = SourceObjectPath.TryLoad();
	const FString ResolvedObjectPath = SourceObjectPath.ToString();
	const int32 ObjectNameSeparator = ResolvedObjectPath.Find(TEXT("."), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	LoadedTmpFullPath = ObjectNameSeparator == INDEX_NONE
		? ResolvedObjectPath
		: ResolvedObjectPath.Left(ObjectNameSeparator);

	if (!IsValid(LoadedTemplateObject))
	{
		LoadedTemplateObject = nullptr;
		return;
	}

	const int32 LastSeparator = LoadedTmpFullPath.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	const FString PackageDirectory = LastSeparator != INDEX_NONE
		? LoadedTmpFullPath.Left(LastSeparator + 1)
		: FString();
	UsesAbsolutePath = !bUsesLegacyStringPath || SourcePathForDisplay.StartsWith(TEXT("/"), ESearchCase::IgnoreCase)
		&& PackageDirectory != LoadedDatatablePath;
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

		TArray<FAssetData> ExactNameMatches;
		for (const FAssetData& Asset : Assets)
		{
			if (Asset.AssetName == FName(*ExpectedAssetName))
			{
				ExactNameMatches.Add(Asset);
			}
		}

		if (ExactNameMatches.Num() == 1)
		{
			LoadedDataTable = Cast<UDataTable>(ExactNameMatches[0].GetAsset());
		}
		else if (ExactNameMatches.Num() > 1)
		{
			UE_LOG(LogTemp, Warning, TEXT("FluidSim: 预设 '%s' 在给定搜索路径中找到 %d 个同名数据表；请改用 PreferredPreset 或收窄路径。"),
				*ExpectedAssetName, ExactNameMatches.Num());
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

	const FStrProperty* SourceStringProperty = MyFindSourceStringProperty(LoadedDataTable->RowStruct);

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

	FHitResult MouseHit;
	FHitResult TouchHit;
	MouseClickValid = PlayerController->GetHitResultUnderCursorByChannel(TraceChannel, true, MouseHit);
	TouchValid = PlayerController->GetHitResultUnderFingerByChannel(
		static_cast<ETouchIndex::Type>(FingerIndex), TraceChannel, true, TouchHit);
	const FHitResult& SelectedHit = TouchSensitive ? TouchHit : MouseHit;

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
	HitUV = FLinearColor::Black;
	TracePosition = FVector::ZeroVector;
	HitValid = false;

	const FVector TraceEnd = End + (End - Start) * TracelineOvershoot;

	FHitResult Hit;
	const bool bHit = UKismetSystemLibrary::LineTraceSingle(
		WorldContextObject, Start, TraceEnd, TraceChannel,
		true, FluidNinjaLIVEActors, EDrawDebugTrace::None, Hit, false);

	bool HitValidator = false;
	if (bHit && IsValid(Hit.GetActor()))
	{
		HitValidator = true;
	}

	if (!HitValidator)
	{
		return;
	}

	FVector2D UV(0.0f, 0.0f);
	UGameplayStatics::FindCollisionUV(Hit, 0, UV);
	HitUV = FLinearColor(UV.X, UV.Y, 0.0f, 1.0f);
	TracePosition = Hit.Location;
	HitValid = HitValidator;
}

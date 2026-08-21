// MyNinjaLiveFunctions.cpp — UMyNinjaLiveFunctions 实现

#include "MyNinjaLiveFunctions.h"

#include "AssetRegistry/AssetRegistryModule.h"
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

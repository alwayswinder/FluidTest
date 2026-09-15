

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "MyNinjaFluidEnums.h"
#include "MyNinjaLiveMemoryPoolManager.generated.h"


USTRUCT(BlueprintType)
struct FLUIDTEST_API FMyRenderTargetListItem
{
	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	TObjectPtr<UTextureRenderTarget2D> MyRT = nullptr;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	bool MyFree = false;
};


UCLASS(Blueprintable, BlueprintType)
class FLUIDTEST_API AMyNinjaLiveMemoryPoolManager : public AActor
{
	GENERATED_BODY()

public:
	AMyNinjaLiveMemoryPoolManager();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	bool MyMMInitFinished = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	bool MyDisableMemoryManager = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	EMySimPrecision MyPrecision = EMySimPrecision::Bit16;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	int32 MyResolutionX = 256;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	int32 MyResolutionY = 256;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	bool MyHalfResPressureAndDivergenceBuffers = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	TArray<FMyRenderTargetListItem> MyRGBA_RenderTargetsList;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	TArray<FMyRenderTargetListItem> MyRG_RenderTargetsList;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	TArray<FMyRenderTargetListItem> MyR_RenderTargetsList;
};

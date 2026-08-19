// MyNinjaLiveMemoryPoolManager.h — NinjaLive_MemoryPoolManager 蓝图占位父类

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "MyNinjaFluidEnums.h"
#include "MyNinjaLiveMemoryPoolManager.generated.h"

/** RenderTargetListItem 蓝图结构的 C++ 对应项。 */
USTRUCT(BlueprintType)
struct FLUIDTEST_API FMyRenderTargetListItem
{
	GENERATED_BODY()

	/** 池条目关联的 RenderTarget。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	TObjectPtr<UTextureRenderTarget2D> MyRT = nullptr;

	/** 条目是否处于空闲状态。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	bool MyFree = false;
};

/** RenderTarget 池管理器蓝图的 C++ 占位父类。 */
UCLASS(Blueprintable, BlueprintType)
class FLUIDTEST_API AMyNinjaLiveMemoryPoolManager : public AActor
{
	GENERATED_BODY()

public:
	AMyNinjaLiveMemoryPoolManager();

	/** 内存池初始化是否完成，组件仅在完成或禁用状态下建立连接。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	bool MyMMInitFinished = false;

	/** 是否禁用内存池管理。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	bool MyDisableMemoryManager = false;

	/** 内存池提供给流体组件的数值精度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	EMySimPrecision MyPrecision = EMySimPrecision::Bit16;

	/** 内存池统一配置的横向分辨率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	int32 MyResolutionX = 256;

	/** 内存池统一配置的纵向分辨率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	int32 MyResolutionY = 256;

	/** 是否使用半分辨率压力和散度缓冲区。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	bool MyHalfResPressureAndDivergenceBuffers = false;

	/** RGBA 格式 RenderTarget 池条目。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	TArray<FMyRenderTargetListItem> MyRGBA_RenderTargetsList;

	/** RG 格式 RenderTarget 池条目。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	TArray<FMyRenderTargetListItem> MyRG_RenderTargetsList;

	/** R 格式 RenderTarget 池条目。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	TArray<FMyRenderTargetListItem> MyR_RenderTargetsList;
};

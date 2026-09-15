


#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyNinjaFluidEnums.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "Materials/MaterialInstance.h"
#include "MyNinjaLiveActor.generated.h"

class UMyNinjaLiveComponent;
class UTextureRenderTarget2D;




UCLASS(Blueprintable, BlueprintType)
class FLUIDTEST_API AMyNinjaLiveActor : public AActor
{
	GENERATED_BODY()

public:
	AMyNinjaLiveActor();


	virtual void BeginPlay() override;


	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


	virtual void Tick(float DeltaSeconds) override;


	UFUNCTION(BlueprintPure, Category = "FluidSim|Component")
	UMyNinjaLiveComponent* GetNinjaLiveComponent() const;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Input")
	EMyUserInput MyUserInputBasedInteraction = EMyUserInput::MouseSingle;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Input")
	TArray<bool> MyMultipleTouchLookup;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	bool MySimActivatedByPawnProximity = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	bool MyDisableBlueprint = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	bool MyUseTraceMeshAsInteractionVolume = false;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Lifecycle")
	bool MyBeginPlaySupressed = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	FVector MyActivationVolumeSize = FVector(4.0f, 4.0f, 2.0f);


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Activation")
	bool MyPawnInsideActivationBounds = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	TObjectPtr<AActor> MyActivator = nullptr;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	TEnumAsByte<ECollisionChannel> MyActivatorType = ECC_Pawn;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation", meta = (ClampMin = "0.001"))
	double MyActivatorProximityCheckFrequency = 0.1;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Timing")
	double MyDeltaSeconds = 0.0;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|RenderTarget")
	TObjectPtr<UTextureRenderTarget2D> MyRTDensityPreview = nullptr;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<FName> MyOverlapFilterInclusiveBoneNameExact;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	TArray<FName> MyOverlapFilterInclusiveBoneNameExactTemp;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	TArray<FName> MyOverlapFilterInclusiveBoneNameExactTemp2;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<FString> MyOverlapFilterInclusiveBoneNamePartial;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	bool MyForceTrackBonesWithSimilarName = false;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	bool MyInitialActorsProcessed = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<TEnumAsByte<EObjectTypeQuery>> MyOverlapFilterInclusiveObjType = {
		EObjectTypeQuery::ObjectTypeQuery1 };


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	bool MyOverlapBasedInteraction = false;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	TArray<TObjectPtr<AActor>> MyNinjaLIVECollisionExclude;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<TObjectPtr<AActor>> MyExcludeSpecificActorsFromOverlap;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	FName MyTrackActorPrimitiveComponentsWithTag = NAME_None;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	FName MyTrackActorSkeletalMeshComponentsWithTag = NAME_None;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	TArray<TObjectPtr<AActor>> MyOverlappingActorsInitial;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TMap<TEnumAsByte<ECollisionChannel>, TEnumAsByte<EObjectTypeQuery>> MyOverlapFilterInclusiveCollisionType;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FluidSim|Activation")
	TObjectPtr<USceneComponent> MyRoot = nullptr;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FluidSim|Activation")
	TObjectPtr<UBoxComponent> MyActivationVolume = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FluidSim|Interaction")
	TObjectPtr<UBoxComponent> MyInteractionVolume = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FluidSim|Trace")
	TObjectPtr<UStaticMeshComponent> MyTraceMesh = nullptr;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	EMyInactiveBehaviour MyTraceMeshInactiveBehaviour = EMyInactiveBehaviour::HoldLastFrameWhenInactive;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	TObjectPtr<UMaterialInstance> MyInactiveGrayMaterial = nullptr;


	UPROPERTY(BlueprintReadWrite, Transient, Category = "FluidSim|Runtime|Interaction")
	TObjectPtr<UPrimitiveComponent> MyInteractionVolumeTemplate = nullptr;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	FVector MyInteractionVolumeSize = FVector(4.0f, 4.0f, 2.0f);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	FVector MyTraceMeshSize = FVector::OneVector;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	TArray<TObjectPtr<AActor>> MyOverlappingActors;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	bool MyForceTrackObjectsWithNocollisionFlag = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	bool MyAutoExcludeLargeOverlappingObjects = false;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Trace")
	void MySetInitialVisibility2();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Interaction")
	void MyEndOverlapDetection();


	UFUNCTION(BlueprintPure, Category = "FluidSim|Interaction")
	bool MyExcludeLargeObjects(const USceneComponent* OverlapComponent) const;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Interaction")
	bool MyCollisionTypeFilter1(const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes,
		const UPrimitiveComponent* OverlapComponent, FString& ObjType,
		TEnumAsByte<ECollisionChannel>& CollisionType) const;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Interaction")
	bool MyCollisionTypeFilter2(const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes,
		const UPrimitiveComponent* OverlapComponent, FString& ObjType,
		TEnumAsByte<ECollisionChannel>& CollisionType) const;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Interaction")
	void MyInitialOverlapCheck();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Interaction")
	void MySetInteractionVolumeCollisionResponse();


	UFUNCTION(BlueprintPure, Category = "FluidSim|Interaction")
	bool MySimContainerCapacityFilter1(const TArray<bool>& TempArrays,
		const TMap<int32, UPrimitiveComponent*>& Pairs,
		const TArray<USkeletalMeshComponent*>& SKmeshComponents) const;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Interaction")
	void MyBeginOverlapDetection();


	UFUNCTION()
	void MyBeginOverlapComponent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);


	UFUNCTION()
	void MyEndOverlapComponent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:

	TWeakObjectPtr<UPrimitiveComponent> MyBoundInteractionVolumeTemplate;


	void MyPrepareInteractionOverlapBindings();


	void MyClearInteractionOverlapBindings();


	FTimerHandle MyInitialOverlapCheckTimer;


	bool MyActivatorSetupDone = false;


	bool MyInactiveShownOnce = false;


	FTimerHandle MyProximityCheckTimer;


	void MyProximityCheck();


	void MyApplyInitialInactiveState(UMyNinjaLiveComponent* NinjaLive);


	void MyInitializeSimulationRuntime(UMyNinjaLiveComponent* NinjaLive);


	void MyProcessOverlapActor(AActor* Actor);


	bool MyReleaseSkeletalSlotsForActor(AActor* Actor);
};

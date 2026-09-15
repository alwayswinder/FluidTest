


#pragma once

#include "CoreMinimal.h"
#include "Containers/StaticArray.h"
#include "Components/ActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameters.h"
#include "MyNinjaFluidEnums.h"
#include "Engine/SceneCapture2D.h"
#include "TimerManager.h"
#include "MyNinjaLiveComponent.generated.h"

class AMyNinjaLiveActor;
class AMyNinjaLiveMemoryPoolManager;
class AActor;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMaterialParameterCollection;
class UDataTable;
class UFileMediaSource;
class UMediaPlayer;
class UMediaTexture;
class UNiagaraComponent;
class UNiagaraSystem;
class UTexture2D;
class UBoxComponent;
class USceneComponent;
enum class EMyNinjaRDGDiffTarget : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMyComponentRePlayEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyWorldSpaceOffsetEvent, FVector, TraceMeshPos);

USTRUCT(BlueprintType)
struct FMyNinjaRDGTextureDiffDiagnostics
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Diagnostics")
	FLinearColor MaxDifference = FLinearColor::Black;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Diagnostics")
	int32 ExceededPixelCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Diagnostics")
	int32 ComparedPixelCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Diagnostics")
	float Tolerance = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Diagnostics")
	int64 SampleId = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Diagnostics")
	bool WithinTolerance = true;
};


UCLASS(Blueprintable, BlueprintType, ClassGroup = (FluidSim), meta = (BlueprintSpawnableComponent))
class FLUIDTEST_API UMyNinjaLiveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMyNinjaLiveComponent();


	UPROPERTY(BlueprintAssignable, Category = "FluidSim|Init")
	FMyComponentRePlayEvent MyComponentRePlayEvent;


	UPROPERTY(BlueprintAssignable, Category = "FluidSim|Trace")
	FMyWorldSpaceOffsetEvent MyWorldSpaceOffset;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Init")
	void MyCheckReady();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Init")
	void MyAfterReadyCheck();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Init")
	void MyRePlay();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Simulation")
	bool MyAfterTickDelay(double DeltaSeconds);


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials|Brush")
	void MyMuteBrush();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	bool MyStopUsingPainterCanvasWhenIdle = false;


	UFUNCTION(BlueprintPure, Category = "FluidSim|Materials|Brush")
	bool MyBrushFadeOutTimer() const;


	UFUNCTION(BlueprintPure, Category = "FluidSim|Simulation")
	FVector MyCorrectExtremes(FVector DeltaPos, FVector Scale, FVector Composite) const;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Simulation")
	void MyDynamicSimspeedAndWorldOffsetAdjustmentFinal();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Simulation")
	void MyDynamicSimspeedAndWorldOffsetAdjustment();


	UFUNCTION(BlueprintPure, Category = "FluidSim|Trace")
	FVector MyLockMovementOnGivenAxis(FVector Pos, EMyQuantizerAxisIgnore LockThisAxis) const;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Trace")
	void MyKillFracOnGivenAxis(FVector Frac, FVector FracInit, EMyQuantizerAxisIgnore QuantizerIgnoresThisAxis,
		FVector& FracOut, FVector& FracInitOut) const;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|RenderTarget")
	TMap<FString, TObjectPtr<UTextureRenderTarget2D>> MyRenderTargetsMap;


	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "FluidSim|Runtime|RenderTarget")
	TArray<FString> MyRenderTargetsList;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|RT")
	bool MySimAreaClamp = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|RT")
	bool MyForce8bitOutputBuffer = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|RT")
	bool MyForce8bitSimplePainterBuffers = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|RT")
	bool MyForce2xResolutionOutputBuffer = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|RT")
	bool MyMake1stOutputAvailableFor2ndOutput = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|RT")
	bool MyMake1stOutputAvailableForNiagara = false;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Diagnostics")
	int32 MyMapLengthTmp = 0;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Diagnostics")
	FLinearColor MyRDGOutputDiffMax = FLinearColor::Black;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Diagnostics")
	int32 MyRDGOutputDiffExceededPixelCount = 0;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Diagnostics")
	int32 MyRDGOutputDiffComparedPixelCount = 0;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Diagnostics")
	float MyRDGOutputDiffTolerance = 0.0f;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Diagnostics")
	int64 MyRDGOutputDiffSampleId = 0;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Diagnostics")
	bool MyRDGOutputDiffWithinTolerance = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Diagnostics")
	FMyNinjaRDGTextureDiffDiagnostics MyRDGAdvectionDiff;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Diagnostics")
	FMyNinjaRDGTextureDiffDiagnostics MyRDGDivergenceDiff;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Diagnostics")
	FMyNinjaRDGTextureDiffDiagnostics MyRDGPressureDiff;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Diagnostics")
	FMyNinjaRDGTextureDiffDiagnostics MyRDGPressureTempDiff;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Diagnostics")
	FMyNinjaRDGTextureDiffDiagnostics MyRDGPainterDiff;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Diagnostics")
	FMyNinjaRDGTextureDiffDiagnostics MyRDGCompositeDiff;





	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	double MyVeloFromSimAreaMotion = 0.0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	double MySimAreaMotionEffectsBrushPuncture = 0.0;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Trace")
	TObjectPtr<UStaticMeshComponent> MyTraceMeshComponent = nullptr;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Trace")
	FVector MyTraceMeshParentPos = FVector::ZeroVector;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Trace")
	FVector MyTraceMeshParentLastPos = FVector::ZeroVector;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Trace")
	FVector MyTraceMeshPosInitialWorld = FVector::ZeroVector;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Trace")
	FVector MyTraceMeshLastPos = FVector::ZeroVector;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Trace")
	FVector MyTraceMeshDeltaPos = FVector::ZeroVector;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Trace")
	FVector MyTraceMeshPosInitialLocal = FVector::ZeroVector;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Trace")
	FVector MyTraceMeshPosInitialFractionalPart = FVector::ZeroVector;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Trace")
	bool MyDynamicSimPositionInitialized = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Quantizer")
	EMyQuantizerAxisIgnore MyMovementNotQuantizedToStepsOnAxis = EMyQuantizerAxisIgnore::None;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	bool MyForceTraceMeshToCustomVerticalPos = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace", meta = (EditCondition = "MyForceTraceMeshToCustomVerticalPos"))
	double MyForceTraceMeshVerticalPosition = 0.0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	double MySimSpeedAdjustmentLatency = 0.0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	double MySingleTargetModeSpeedInfluenceFactor_LEGACY = 0.0;





	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Input")
	EMyUserInput MyUserInputBasedInteraction = EMyUserInput::None;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Input")
	bool MySingleInput = false;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Input")
	bool MyTouch = false;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Input")
	void MyCheckTouchOptions();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Input")
	bool MyShowMouseCursor = true;





	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	bool MyContinuousInteractionWithOwnerActor = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<FName> MyContinuousInteractionComponentNamesExact;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<FName> MyContinuousInteractionBoneNamesExact;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<TEnumAsByte<EObjectTypeQuery>> MyContinuousInteractionInclusiveObjType;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	TArray<TObjectPtr<UPrimitiveComponent>> MyOverlappingComponents;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	TArray<TObjectPtr<USkeletalMeshComponent>> MyContinuousInteractionSkeletalComponent;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	TArray<FName> MyContinuousInteractionBoneNamesExactTemp;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	TArray<FName> MyContinuousInteractionBoneNamesExactTemp2;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	TArray<bool> MyListOfAvailableTempArrays;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	TMap<int32, TObjectPtr<UPrimitiveComponent>> MySkeletalMeshTempArrayPairs;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	bool MyOverlapBasedInteraction = false;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Temp")
	void MyResetTempArrays();


	UFUNCTION(BlueprintPure, Category = "FluidSim|Temp")
	TArray<FName> MyGetTempArray(int32 Index) const;


	TArray<FName>& MyGetTempArrayRef(int32 Index);


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Temp")
	void MyAddToTempArray(int32 ArrayIndex, FName Item);


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Temp")
	void MyClearTempArray(int32 ArrayIndex);


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Temp")
	void MyAppendToTempArray(int32 ArrayIndex, const TArray<FName>& Items);


	void MyResetTempArraySlots();


	int32 MyAcquireTempArraySlot();


	void MyReleaseTempArraySlot(int32 ArrayIndex);


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Interaction")
	void MyManageContinuousInteractions();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Interaction")
	void MyCheckValidity2(UPrimitiveComponent*& SingleTarget, bool& ThenExec);


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Preset")
	void MyParsePresetMapAndSetVariables(const TMap<FString, double>& PresetMap);


	UFUNCTION(BlueprintPure, Category = "FluidSim|Temp")
	void MyCompareMapLength(int32 FirstIndex, int32 LastIndex, int32& MapLength, bool& Equal, int32& Added) const;


	UFUNCTION(BlueprintPure, Category = "FluidSim|Velocity")
	void MyVelocityHandlerForSimArea(double CoEff, double& X, double& Y, double& Z) const;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Input")
	bool MyCheckComponentOwner(AMyNinjaLiveActor*& AsNinjaLive) const;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Input")
	void MyEnableOwnerInput();


	UFUNCTION(BlueprintPure, Category = "FluidSim|Quantizer")
	int32 MyQuantizerValues(EMyQuantizerMode InQuantizerMode) const;





	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	bool MyComponentActivatedByPawnProximity = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	bool MyDisableComponent = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	bool MyUseUnrealNativeEventTick = true;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	double MyLimitUnrealNativeEventTick = 0.0;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Timing")
	double MyDeltaSeconds = 0.0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|System")
	bool MyBeginPlaySupressed = false;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Lifecycle")
	bool MyInitDone = false;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Lifecycle")
	bool MyMaterialInstacesDone = false;


	UPROPERTY(BlueprintReadWrite, Transient, Category = "FluidSim|Runtime|Timing")
	bool MyTickBlocker = false;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Activation")
	bool MyPawnInsideActivationBounds = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	bool MyPauseSimWhenNotVisible = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation", meta = (ClampMin = "0.0"))
	float MyWaitBeforePause = 0.2f;


	UPROPERTY(BlueprintReadWrite, Transient, Category = "FluidSim|Runtime|Timing")
	double MyTimeSinceLastClick = 0.0;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Timing")
	double MyTimeSinceLastCollision = 0.0;


	UPROPERTY(Transient)
	bool MyAfterTickDelayDeactivateDoOnceClosed = false;


	UPROPERTY(Transient)
	bool MyAfterTickDelayRearmDoOnceClosed = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|System")
	FName MyPresetNameFilterCriteria = NAME_None;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	TObjectPtr<UDataTable> MyDefaultPreset = nullptr;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	bool MyForceAutoLoadPreset = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	FString MyActualPreset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	TArray<FName> MyPresetSearchPaths;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Preset")
	TMap<FString, double> MyPresetMap;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Trace")
	TArray<TObjectPtr<AActor>> MyNinjaLiveTraceExclude;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MySupressUE51TextureSmearing = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|System")
	bool MyUE5EAFLAG = false;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Quantizer")
	int32 MyQuantizerStepSize = 0;


	FTimerHandle MyTimerSimSpeedAdjustment;


	bool MySimSpeedAdjustmentPending = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Quantizer")
	bool MyEnablePainterDoubleBuffering = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Quantizer")
	EMyQuantizerMode MyTraceMeshMovingInWorldSpace = EMyQuantizerMode::NoQuantizerTextureOffsetAutomatic;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TArray<TObjectPtr<UMaterialInterface>> MyOutputMaterials;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TArray<TObjectPtr<UMaterialInterface>> MySecondaryOutputMaterials;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TArray<TObjectPtr<UMaterialInterface>> MyTertiaryOutputMaterials;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	int32 MyOutputMaterialSelected = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	int32 MySecondaryOutputMaterialSelected = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	int32 MyTertiaryOutputMaterialSelected = 0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TObjectPtr<UMaterialParameterCollection> MySetInternalParamsToMaterialParamCollection = nullptr;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MyUsePAINTER_V2_ToTrackObjects = true;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MyCameraFacingTraceMesh = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MyUseLegacyCameraFacing = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MyCameraFacingLockYAxis = false;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Compatibility")
	void MyCameraFacing();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MySingleTargetMode_LEGACY = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	EMySingleObjectType MySingleTargetType_LEGACY = EMySingleObjectType::SkeletalMeshBone;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	int32 MySingleTargetModeSkeletalMeshIndex_LEGACY = 0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MySingleTargetModeSetSimSpeed_LEGACY = false;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	double MySpeedTemp = 0.0;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Input")
	bool MyMousePass = false;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Input")
	int32 MyTouchLookupIndex = 0;


	bool MyCheckTouchOptionsDoOnceClosed = false;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	FLinearColor MyPosition1_2D = FLinearColor::Black;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	FLinearColor MyPosition2_2D = FLinearColor::Black;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	FLinearColor MyLastPosition2_2D = FLinearColor::Black;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	TArray<FLinearColor> MyPosition3_2D;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	TArray<FLinearColor> MyLastPosition3_2D;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Input")
	FLinearColor MyLastMouseHitUV_2D = FLinearColor::Black;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Velocity")
	void MySingleTargetVelocity();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	double MyBrushVelocityClamp = 0.0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	bool MyDampenIgnoresStaticMeshes = false;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	TObjectPtr<UPrimitiveComponent> MyOverlappingComponent = nullptr;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	TObjectPtr<UPrimitiveComponent> MyOverlappingSkeletalMesh = nullptr;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	FName MyOverlappingBone = NAME_None;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	int32 MyPosDataType = 0;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Velocity")
	void MyMultiObjectVelocity(FLinearColor& Velocity);







	UFUNCTION(BlueprintCallable, Category = "FluidSim|Init")
	void MyProximityActivationMasterVarsQuantizerOutMat();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Init")
	void MyProximityActivationMasterVarsQuantizerOutMatFromOwner();






	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Light")
	bool MyEnableRayMarching = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Light")
	TObjectPtr<AActor> MyLightDirectionProvider = nullptr;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Light")
	bool MyLightDirectionSourceIsRotation_NOT_Pos = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Light")
	double MySunLatitude = 0.0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Light")
	double MySunLongitude = 0.0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Light")
	double MySunHeight = 0.0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Light")
	bool MyForceManualSunPosition = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Light")
	double MyTwoSideBlendPow = 0.0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Light")
	bool MyTwoSidedShading = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Light")
	bool MyDistanceBasedLightAttenuation = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Light")
	double MyAttenuationPower = 0.0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Light")
	double MyPointLightMovementMultiplier = 0.0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Light")
	FVector MyOffsetLightVector = FVector::ZeroVector;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Light")
	double MyFacing = 0.0;



	UFUNCTION(BlueprintCallable, Category = "FluidSim|Light")
	void MyLightDirectionProviderCheck();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Light")
	void MyRaymarchBasedLightingOPs();





	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	TEnumAsByte<ETraceTypeQuery> MyTraceChannel = TraceTypeQuery1;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	TEnumAsByte<ECollisionChannel> MyCollisionChannel = ECC_WorldStatic;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	FString MyPreferredTraceChannelName = TEXT("FluidTrace");


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Trace")
	bool MyTraceChannelsSet = false;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Trace")
	void MyTraceChannelAutoFind();




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TArray<TObjectPtr<UMaterialInterface>> MyInputMaterials;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TObjectPtr<ASceneCapture2D> MyInputSceneCaptureCamera = nullptr;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Materials")
	bool MyUseInputMaterials = false;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials")
	void MySceneCapCameraVSInputMaterials();





	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Trace")
	double MyTraceMeshSizeCoeff = 0.0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	bool MyBrushScaledInverselyByTraceMeshSize = false;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Trace")
	FRotator MyTraceMeshInitialRotation = FRotator::ZeroRotator;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	int32 MyTraceMeshTranslucentSortPrio = 0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	bool MyTraceMeshIsAlsoInteractionVolume = false;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Trace")
	void MySetTraceMeshProperties();





	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	bool MyUseCustomTraceSource = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	FVector MyCustomTraceSourcePosition = FVector::ZeroVector;


	UFUNCTION(BlueprintPure, Category = "FluidSim|Trace")
	FVector MyDefineLineTracingSource() const;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Trace")
	FVector MyTracePositionTemp = FVector::ZeroVector;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Trace")
	FVector MyLastTracePositionTemp = FVector::ZeroVector;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	FVector MyPosition1_3D = FVector::ZeroVector;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	FVector MyLastPosition1_3D = FVector::ZeroVector;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	bool MyPosition1_3D_Static = false;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Trace")
	void MyOverlapArtifactWorkaround2(FVector In);


	void MyApplyTraceArtifactBrushMute(FVector TracePosition, float ObjectMoveTolerance);


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Trace")
	void MyTraceObjects2(FVector Start, FLinearColor& HitUV, bool& ThenExec, bool& NoHitExec);


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Trace")
	void MyTraceObjects1(FVector Start, FLinearColor& HitUV);


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Trace")
	bool MyTraceGestures(FLinearColor& HitUV);


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Trace")
	void MyTraceObj2();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Interaction")
	void MyMousePassTrue();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Interaction")
	void MyMousePassFalse();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Interaction")
	void MySingleTargetMode();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Interaction")
	void MyMultiObjectProcessorCycle3();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Interaction")
	void MyForLoopOverlapping();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Interaction")
	void MyNoInteraction();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Trace")
	void MyTemporarilySwitchOffLineDrawingIFTracerFails();





	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	int32 MyResolutionX = 256;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	int32 MyResolutionY = 256;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation", meta = (ClampMin = "1"))
	int32 MyMaxSamplingFPS = 60;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|LOD")
	int32 MySamplingFPS = 60;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation", meta = (ClampMin = "1"))
	int32 MyMinSamplingFPS = 60;





	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|LOD", meta = (ClampMin = "1"))
	int32 MyLODSteps = 1;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|LOD")
	double MyLODNearBound = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|LOD")
	double MyLODFarBound = 0.0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|LOD")
	bool MyLOD1ReduceSimQuality = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|LOD")
	bool MyLOD2ReduceSamplingFPS = false;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|LOD")
	int32 MyLODLevel = 0;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|LOD")
	double MyLODStepRange = 0.0;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|LOD")
	TArray<double> MyLODStepsArray;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|LOD", meta = (ClampMin = "0.001"))
	double MyLODCheckFrequency = 0.2;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|LOD")
	void MyLODDistaceStepsPrecalc();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|LOD")
	void MyLOD();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Init")
	void MyAfterBind();


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Timing")
	double MyTickRateCustom = 1.0 / 60.0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	EMySimPrecision MySimPrecision = EMySimPrecision::Bit16;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Simulation")
	int32 MySimPrecisionIndex = 0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	bool MyHalfResPressureAndDivergenceBuffers = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|LOD")
	int32 MyFluidSolver1Iterations = 5;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	int32 MyPressureSolver1MaxIterations = 0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	int32 MyPressureSolver2MaxIterations = 0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	double MyPressureSolver2KernelReduction = 0.0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	int32 MyInputMaterialSelected = 0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	EMyQuantizerAxisIgnore MyMovementIsLockedOnThisAxis = EMyQuantizerAxisIgnore::X;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	TObjectPtr<UBoxComponent> MyInteractionVolume = nullptr;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Trace")
	bool MyInteractionVolumeIsPresent = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	bool MyLWCAvoidNiagaraWarnings = false;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Simulation")
	void MyCoreFluidsimOPs(bool& ThenExec, bool& PainterV2Exec);


	void MyApplyRDGOutputDiffResult(
		int64 SampleId,
		FLinearColor MaxDifference,
		int32 ExceededPixelCount,
		int32 ComparedPixelCount,
		float Tolerance);
	void MyApplyRDGCoreDiffResult(
		EMyNinjaRDGDiffTarget DiffTarget,
		int64 SampleId,
		FLinearColor MaxDifference,
		int32 ExceededPixelCount,
		int32 ComparedPixelCount,
		float Tolerance);


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Simulation")
	void MyFluidCoreStep();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials|Solver")
	void MySetAdditionalFluidsimParams();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MyPV2_Connect_TrackpointsWithLines = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Compatibility")
	bool MyPV2_GenerateVelocity = false;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Niagara")
	bool MyPV2_Interpolation = false;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Simulation")
	void MyFPSPrecisionResolution();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Niagara")
	void MyInitPainterV2();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Niagara")
	void MyForwardScalarParamsToNiagara();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Niagara")
	void MySetPosVelocityScaleArraysToPainterV2();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Niagara")
	void MyClearPosVelocityScaleArraysPainterV2();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Niagara")
	void MyBuildBrushPositionArray();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Niagara")
	void MyFinalDealRTAndBrush();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|RenderTarget|Export")
	bool MyDrawInternalRenderTargetToExternalEnabled = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|RenderTarget|Export")
	TArray<TObjectPtr<UTextureRenderTarget2D>> MyExternalRenderTargets;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|RenderTarget|Export")
	TArray<EMyRenderTargetList> MyInternalRenderTargetsToExport;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|RenderTarget|Export")
	void MyDrawInternalRenderTargetToExternal();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	TArray<TObjectPtr<UNiagaraSystem>> MyCoreNiagaraSystems;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Niagara")
	TObjectPtr<UNiagaraComponent> MyNiagaraBasedPainter = nullptr;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Niagara")
	TArray<FVector2D> MyPositionArray;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Niagara")
	TArray<FVector2D> MyLastPositionArray;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Niagara")
	TArray<FLinearColor> MyVelocityArray;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Niagara")
	TArray<float> MyBrushSizeArray;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Niagara")
	TArray<TObjectPtr<UPrimitiveComponent>> MyPrimitivesArray;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Niagara")
	TArray<TObjectPtr<UPrimitiveComponent>> MyLastPrimitivesArray;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Niagara")
	TArray<TObjectPtr<UPrimitiveComponent>> MySKmeshesArray;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Niagara")
	TArray<TObjectPtr<UPrimitiveComponent>> MyLastSKmeshesArray;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Niagara")
	void MyBuildOverlapSKMArray(UPrimitiveComponent* In);


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Niagara")
	bool MyHitValid = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	float MyPV2StopLineDrawingAboveThisVelocity = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	float MyAdjustPainterV2BrushStrength = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	float MyAdjustPainterV2BrushVeloNoise = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara", meta = (ClampMin = "0.0"))
	double MyPV2LineDrawingFailCooldownTime = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	TObjectPtr<UTexture> MyPainterV2BrushVeloNoiseTexture = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara", meta = (ClampMin = "0.0"))
	double MyNiagaraVariableSetSafetyDelay = 0.0;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|RenderTarget")
	void MyCreateOrAcquireRenderTargets();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TArray<TObjectPtr<UMaterialInterface>> MyCoreSimMaterials;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	bool MyFlipRenderTargetsForMobile = false;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MyBrushDensityNoiseScale = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MyBrushDensityNoiseFreq = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MyBrushVelocityNoiseScale = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MyBrushVelocityNoiseFreq = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MyBrushVelocityPow = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	bool MyBrushNoiseInWorldSpace = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MyDampenBrushBelowThisVelocity = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MyDampenBrushFactor = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	bool MyAllowAbsoluteBlackDensity = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MyAdjustPainterV2EdgeMask = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MySpeed = 0.0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloOffsetX = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloOffsetY = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloFromBrushMotion = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyOffsetFromSimAreaMotion = 1.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloStrength = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloRotate = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloAmpNoise = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloDirNoise = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyInputFeedback = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyInputFeedbackInterface = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyBrushSize = 0.0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MyGlobalBrushScale = 0.0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MyOverlappingMeshSizeCoeff = 0.0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MyUserInputBrushScale = 0.0;


	UFUNCTION(BlueprintPure, Category = "FluidSim|Materials|Brush")
	double MyBrushSizeCoEff() const;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials|Brush")
	void MyCalculateBrushSizeCoEffFromBoneDistance(FVector In, double BrushScaleMult, double& Out);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	bool MyBrushScaledByInteractingObjSize = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MySkeletalMeshBrushScale = 0.0;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Trace")
	void MyCalcPos5();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Trace")
	void MyCalcPos3();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Trace")
	void MyCalcPos2(UObject* In);


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Trace")
	void MyCalcPos1(USceneComponent* Component);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	bool MyUseObjBoundsInsteadOfSize = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Brush")
	double MyPrimitiveObjBrushScale = 0.0;


	UFUNCTION(BlueprintPure, Category = "FluidSim|Materials|Brush")
	double MyCalculateBrushSizeCoFromBounds1(USceneComponent* Component) const;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyBrushStrength = 0.0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Interaction")
	bool MyOverlap1 = false;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "FluidSim|Runtime|Input")
	bool MyMousePressed = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Brush")
	double MyBrushStrengthTemp1 = 0.001;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Brush")
	double MyBrushStrengthTemp2 = 1.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyBrushHardness = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyBrushPuncture = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	bool MyEraserMode = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyDensityTxtMult = 1.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyFadeDensityAtSimEdge = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MySimEdgeBouncyness = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloDirNoiseSize = 1.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloDirNoiseSpeed = 1.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyEdgeMaskWidth = 0.25;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyDensityTxtScale = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyDensityTxtOffsetX = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyDensityTxtOffsetY = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyBrushNoise = 0.0;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials|Brush")
	void MySetBrushDensityParams1(double Value);


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials|Brush")
	void MyPaintLine();


	UFUNCTION(BlueprintPure, Category = "FluidSim|Materials|Brush")
	bool MyBrushSwitch2(FLinearColor InLinearColor) const;


	UFUNCTION(BlueprintPure, Category = "FluidSim|Materials|Brush")
	bool MyBrushSwitch1(FLinearColor InLinearColor) const;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials|Brush")
	void MySetBrushDensityParams3(double Value);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloInputTile = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyVeloInputOffsetSpeed = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyDensityInputNoiseAmp = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyDensityInputNoiseOffset = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyDensityInputNoiseTile = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Preset")
	double MyBrushRnd = 0.0;


	UFUNCTION(BlueprintPure, Category = "FluidSim|Materials|Brush")
	FLinearColor MyBrushRnd3(const FLinearColor InColor) const;


	UFUNCTION(BlueprintPure, Category = "FluidSim|Materials|Brush")
	FLinearColor MyBrushRnd2(const FLinearColor InColor) const;


	UFUNCTION(BlueprintPure, Category = "FluidSim|Materials|Brush")
	FLinearColor MyBrushRnd1(const FLinearColor InColor) const;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Solver")
	double MyFlowFeedback = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Solver")
	double MyDivergence = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Solver")
	double MyPressureEdgeMasking = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Solver")
	double MyExperimentalPressureFeedback = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Solver")
	double MyExpPressureFeedbackComponent = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Solver")
	double MyExpDivergenceFeedbackComponent = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Solver")
	int32 MyExperimentalPSolver2KernelIndexOffset = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Solver")
	bool MyUsePressureSolver1DefaultIs2 = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	TObjectPtr<UTexture> MyDensityInput = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	TObjectPtr<UTexture> MyVelocityInput = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	TObjectPtr<UDataTable> MyLoadedDataTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	FString MyLoadedDataTablePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	TObjectPtr<UTexture2D> MyOverwritePresetVelocityInput = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	TObjectPtr<UTexture2D> MyOverwritePresetDensityInput = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	TObjectPtr<UTexture> MyCollisionMask = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	TObjectPtr<UTexture> MyInputRenderTarget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	TObjectPtr<UMediaTexture> MyMediaTexture = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	TObjectPtr<UMediaPlayer> MyInputMediaPlayer = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	TObjectPtr<UFileMediaSource> MyInputMediaSource = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input", meta = (ClampMin = "0.0"))
	double MyInputMediaLoopLength = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	bool MyUseRenderTargetAsInput = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	bool MyRandomizeNoiseOffsets = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	bool MyRandomizeDensityTextureOffset = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	bool MyCollisionMaskIsNonDefault = false;


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials|Input")
	void MyUpdateCollisionMaskIsNonDefault();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials|Input")
	void MyAlternativeInputsFedToCompositeDensityInput();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials|Input")
	void MyLoadVelocityInputTexture();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials|Input")
	void MyLoadDensityInputTexture();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials|Input")
	void MyLoadTextures();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials")
	void MyCreateDynamicMaterialInstances();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Init")
	void MyAfterCreateRT();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials")
	void MyCreateOutputMaterialAndSetItOnTargetsStep01();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Materials")
	void MyCreateOutputMaterialAndSetItOnTargetsStep02();


	UFUNCTION(BlueprintCallable, Category = "FluidSim|Niagara")
	void MyCreateOutputMaterialAndSetItOnTargetsStep03();


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMICompositeAndGradient = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMIAdvection = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMIDivergence = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMIPressureCycle1 = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMIPressureCycle2 = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMICollisionPainterLine = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMICollisionPainterDot = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMICollisionPainterOffset = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMINull = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMIOutput = nullptr;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMISecondaryOutput = nullptr;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Materials")
	TObjectPtr<UMaterialInstanceDynamic> MyMITertiaryOutput = nullptr;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Materials")
	bool MySecondaryMaterialsPresent = false;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Materials")
	bool MyTertiaryMaterialsPresent = false;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Materials")
	bool MyMaterialCollectionPresent = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TObjectPtr<UMaterialInterface> MyInactiveGrayMaterial = nullptr;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials")
	TObjectPtr<UMaterialInterface> MyNullMaterial = nullptr;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	bool MyTraceMeshInvisible = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Output")
	FName MyApply1stOutMatToActorsWithTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Output")
	FName MyApply2ndOutMatToActorsWithTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Output")
	FName MyApply3rdOutMatToActorsWithTag = NAME_None;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Output")
	FName MyApply1stOutMatToComponentsWithTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Output")
	FName MyApply2ndOutMatToComponentsWithTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Output")
	FName MyApply3rdOutMatToComponentsWithTag = NAME_None;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	FName MyFeedTaggedActorNiagaraComponent = NAME_None;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Niagara")
	TArray<TObjectPtr<UNiagaraComponent>> MyNiagaraSystemsToDrive;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Niagara")
	bool MyNiagaraSystemsPresent = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	bool MyMakePressureAvailableForNiagara = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	bool MyForceMaxSamplingFPSToNiagara = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Niagara")
	bool MyLWCSupport = false;


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "FluidSim|Runtime|Trace")
	FVector MyTraceMeshPos = FVector::ZeroVector;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|MemoryPool")
	bool MySimplePainterMode = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Materials|Input")
	bool MyRGBInputMaterial = false;

private:

	static constexpr int32 MyTempArrayCount = 40;
	TStaticArray<TArray<FName>, MyTempArrayCount> MyTempArrays;


	static constexpr int32 MyTouchSlotCount = 10;


	TArray<FName> MyInvalidTempArray;


	void MyApplyOutputMaterialToTraceMesh();


	void MyApplyOutputMaterialsToTaggedActors();


	void MyBuildTraceExcludeList();


	void MyApplyPlatformCompatibilityOptions();


	void MyApplyPresetAndInputTextures();


	void MySetCompositeEraserSwitch();
	void MyRefreshTraceExcludeActors();

	void MySetScalarParameterByCachedIndex(UMaterialInstanceDynamic* Material,
		TMap<FName, int32>& ParameterIndices, FName ParameterName, float Value);

	TArray<AActor*> MyNinjaLiveTraceExcludeRaw;
	uint64 MyTraceExcludeRefreshFrame = MAX_uint64;
	TMap<FName, int32> MyCompositeScalarParameterIndices;
	TMap<FName, int32> MyDivergenceScalarParameterIndices;
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> MyRDGOutputComparisonTarget = nullptr;
	uint64 MyRDGOutputFrameIndex = 0;
	bool MyRDGOutputTargetCreatedForValidation = false;
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> MyRDGAdvectionComparisonTarget = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> MyRDGDivergenceComparisonTarget = nullptr;
	uint64 MyRDGCoreFrameIndex = 0;
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> MyRDGPressureComparisonTarget = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> MyRDGPressureTempComparisonTarget = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MyMIPressureCycle1Comparison = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MyMIPressureCycle2Comparison = nullptr;
	uint64 MyRDGPressureFrameIndex = 0;
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> MyRDGPainterComparisonTarget = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> MyRDGCompositeComparisonTarget = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MyMICollisionPainterOffsetFirstPass = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MyMICollisionPainterOffsetComparisonFirstPass = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MyMICollisionPainterOffsetComparisonSecondPass = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MyMICompositeAndGradientComparison = nullptr;
	uint64 MyRDGPainterFrameIndex = 0;


	FTimerHandle MyTimerCheckReady;


	FTimerHandle MyInputMediaLoopTimer;
	FTimerHandle MyLoadTexturesTimer;
	FTimerHandle MyNiagaraPainterV2SafetyTimer;
	FTimerHandle MyNiagaraPainterV2CooldownTimer;


	void MyDestroyPainterV2();


	void MyNormalizeTimingParameters();


	void MyApplySimulationTickRate();


	FTimerHandle MyLineDrawingFailCooldownTimer;


	FTimerHandle MyLODCheckTimer;
	UPROPERTY(Transient)
	bool MyLODDoOnceClosed = false;


	void MyCheckLODLevel();


	UPROPERTY(Transient)
	bool MyNativeTickLimiterDoOnceClosed = false;


	FTimerHandle MyCustomTickLoopTimer;
	UPROPERTY(Transient)
	bool MyCustomTickLoopStarted = false;


	void MyCustomTick();


	void MyRestartInputMedia();


	void MyRestoreLineDrawingAfterCooldown();

protected:

	virtual void BeginPlay() override;


	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;


	void MySetPainterV2PaintbufferInput();


	void MyFinalizePainterV2Setup();


	void MyApplyPainterV2SharedParameters();


	UPROPERTY(Transient)
	bool bMyTraceMeshInitialRotationCaptured = false;


	UPROPERTY(Transient)
	bool bMyExternalRenderTargetExportValidated = false;


	UPROPERTY(Transient)
	bool bMyExternalRenderTargetExportGateOpen = false;


	TArray<FMaterialParameterInfo> MyPainterScalarParameterInfos;
	TMap<FName, float> MyLastForwardedPainterScalarValues;
	bool bMyPainterScalarParameterCacheInitialized = false;


	TArray<FVector2D> MyLastSentPositionArray;
	TArray<FVector2D> MyLastSentLastPositionArray;
	TArray<FLinearColor> MyLastSentVelocityArray;
	TArray<float> MyLastSentBrushSizeArray;
	bool bMyPainterArraysSent = false;
	bool bMyLastSentPosInterpolValid = false;
	bool bMyLastSentPosInterpol = false;

};

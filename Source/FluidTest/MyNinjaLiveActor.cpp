

#include "MyNinjaLiveActor.h"

#include "MyNinjaLiveComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/Texture.h"
#include "Engine/TextureRenderTarget2D.h"
#include "FluidTest/MyNinjaLiveFunctions.h"
#include "TimerManager.h"

AMyNinjaLiveActor::AMyNinjaLiveActor()
{

	PrimaryActorTick.bCanEverTick = true;


	MyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = MyRoot;


	MyActivationVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("MyActivationVolume"));
	MyActivationVolume->SetupAttachment(MyRoot);
	MyActivationVolume->SetBoxExtent(FVector(200.0f, 200.0f, 100.0f));
	MyActivationVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MyActivationVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	MyActivationVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	MyActivationVolume->SetGenerateOverlapEvents(true);
	MyActivationVolume->SetHiddenInGame(true);


	MyInteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("MyInteractionVolume"));
	MyInteractionVolume->SetupAttachment(MyRoot);
	MyInteractionVolume->SetBoxExtent(FVector(200.0f, 200.0f, 100.0f));
	MyInteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MyInteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	MyInteractionVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	MyInteractionVolume->SetGenerateOverlapEvents(true);
	MyInteractionVolume->SetHiddenInGame(true);


	MyTraceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MyTraceMesh"));
	MyTraceMesh->SetupAttachment(MyRoot);
	MyTraceMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MyTraceMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	MyTraceMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	MyTraceMesh->SetGenerateOverlapEvents(false);
	MyTraceMesh->SetHiddenInGame(true);
}

UMyNinjaLiveComponent* AMyNinjaLiveActor::GetNinjaLiveComponent() const
{
	return FindComponentByClass<UMyNinjaLiveComponent>();
}

void AMyNinjaLiveActor::MyPrepareInteractionOverlapBindings()
{
	if (MyBoundInteractionVolumeTemplate.Get() == MyInteractionVolumeTemplate.Get())
	{
		return;
	}

	MyClearInteractionOverlapBindings();
	MyBoundInteractionVolumeTemplate = MyInteractionVolumeTemplate;
}

void AMyNinjaLiveActor::MyClearInteractionOverlapBindings()
{
	if (UPrimitiveComponent* BoundVolume = MyBoundInteractionVolumeTemplate.Get())
	{
		BoundVolume->OnComponentBeginOverlap.RemoveDynamic(this, &AMyNinjaLiveActor::MyBeginOverlapComponent);
		BoundVolume->OnComponentEndOverlap.RemoveDynamic(this, &AMyNinjaLiveActor::MyEndOverlapComponent);
	}
	MyBoundInteractionVolumeTemplate.Reset();
}

void AMyNinjaLiveActor::BeginPlay()
{
	Super::BeginPlay();


	if (MyDisableBlueprint)
	{
		MySetInitialVisibility2();
		return;
	}


	if (MySimActivatedByPawnProximity)
	{
		MyBeginPlaySupressed = true;
		if (IsValid(MyActivationVolume))
		{
			MyActivationVolume->SetBoxExtent(MyActivationVolumeSize * 50.0f);
			MyActivationVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
		return;
	}


	if (IsValid(MyActivationVolume))
	{
		MyActivationVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	MyInitializeSimulationRuntime(GetNinjaLiveComponent());
}

void AMyNinjaLiveActor::MyInitializeSimulationRuntime(UMyNinjaLiveComponent* NinjaLive)
{
	if (!IsValid(NinjaLive))
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MyInitialOverlapCheckTimer);
	}
	MyInitialOverlapCheckTimer.Invalidate();


	if (IsValid(MyTraceMesh))
	{
		MyTraceMesh->SetWorldScale3D(MyTraceMeshSize);
	}

	NinjaLive->MyTraceMeshComponent = MyTraceMesh;
	NinjaLive->MyUserInputBasedInteraction = MyUserInputBasedInteraction;
	NinjaLive->MyOverlapBasedInteraction = MyOverlapBasedInteraction;
	NinjaLive->MyDisableComponent = MyDisableBlueprint;
	NinjaLive->MyComponentActivatedByPawnProximity = MySimActivatedByPawnProximity;
	NinjaLive->MyPawnInsideActivationBounds = MyPawnInsideActivationBounds;


	if (!MyOverlapBasedInteraction)
	{
		return;
	}


	MyInteractionVolumeTemplate = MyUseTraceMeshAsInteractionVolume
		? static_cast<UPrimitiveComponent*>(MyTraceMesh.Get())
		: static_cast<UPrimitiveComponent*>(MyInteractionVolume.Get());

	if (IsValid(MyInteractionVolume))
	{
		MyInteractionVolume->SetBoxExtent(MyInteractionVolumeSize * 50.0f);
	}


	MyOverlappingActors.Reset();
	NinjaLive->MyOverlappingComponents.Reset();
	NinjaLive->MyResetTempArraySlots();
	MyOverlappingActorsInitial.Reset();


	TArray<AActor*> SameClassActors;
	UGameplayStatics::GetAllActorsOfClass(this, GetClass(), SameClassActors);
	SameClassActors.Remove(this);
	MyNinjaLIVECollisionExclude.Reset();
	for (AActor* Actor : SameClassActors)
	{
		MyNinjaLIVECollisionExclude.Add(Actor);
	}
	for (AActor* Actor : MyNinjaLIVECollisionExclude)
	{
		MyExcludeSpecificActorsFromOverlap.AddUnique(Actor);
	}


	MyInitialOverlapCheck();
	MyBeginOverlapDetection();
	MyEndOverlapDetection();
}

void AMyNinjaLiveActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	MyClearInteractionOverlapBindings();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MyProximityCheckTimer);
		World->GetTimerManager().ClearTimer(MyInitialOverlapCheckTimer);
	}
	MyProximityCheckTimer.Invalidate();
	MyInitialOverlapCheckTimer.Invalidate();

	Super::EndPlay(EndPlayReason);
}

void AMyNinjaLiveActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);


	if (MyDisableBlueprint)
	{
		return;
	}
	MyDeltaSeconds = DeltaSeconds;


	if (MySimActivatedByPawnProximity)
	{
		if (UWorld* World = GetWorld();
			World != nullptr && !World->GetTimerManager().IsTimerActive(MyProximityCheckTimer))
		{
			World->GetTimerManager().SetTimer(MyProximityCheckTimer, this,
				&AMyNinjaLiveActor::MyProximityCheck,
				FMath::Max(static_cast<float>(MyActivatorProximityCheckFrequency), KINDA_SMALL_NUMBER), false);
		}
	}


	if (!MyActivatorSetupDone)
	{
		MyActivatorSetupDone = true;
		if (MySimActivatedByPawnProximity && IsValid(MyActivationVolume))
		{
			MyActivationVolume->SetGenerateOverlapEvents(true);
			MyActivationVolume->SetCollisionResponseToChannel(MyActivatorType.GetValue(), ECR_Overlap);
		}
	}
}

void AMyNinjaLiveActor::MyProximityCheck()
{
	UMyNinjaLiveComponent* NinjaLive = GetNinjaLiveComponent();
	if (!IsValid(NinjaLive) || !IsValid(MyActivationVolume))
	{
		return;
	}


	AActor* Target = IsValid(MyActivator) ? MyActivator.Get() : UGameplayStatics::GetPlayerPawn(this, 0);


	const bool bIsInside = IsValid(Target) ? MyActivationVolume->IsOverlappingActor(Target) : false;
	if (bIsInside == MyPawnInsideActivationBounds)
	{

		if (!MyPawnInsideActivationBounds && !MyInactiveShownOnce)
		{
			MyInactiveShownOnce = true;
			NinjaLive->MyPawnInsideActivationBounds = false;
			MyApplyInitialInactiveState(NinjaLive);
		}
		return;
	}

	MyPawnInsideActivationBounds = bIsInside;
	NinjaLive->MyPawnInsideActivationBounds = bIsInside;
	if (bIsInside)
	{
		const bool bWasDeferredInitialization = MyBeginPlaySupressed;
		MyBeginPlaySupressed = false;
		MyInitializeSimulationRuntime(NinjaLive);
		if (bWasDeferredInitialization && !NinjaLive->MyInitDone)
		{
			NinjaLive->MyCheckReady();
			if (NinjaLive->MyTraceChannelsSet)
			{
				MyInitializeSimulationRuntime(NinjaLive);
			}
		}
	}

	if (!bIsInside)
	{

		switch (MyTraceMeshInactiveBehaviour)
		{
		case EMyInactiveBehaviour::HoldLastFrameWhenInactive:

			MyRTDensityPreview = UMyNinjaLiveFunctions::MyCreateRenderTarget(this, 64, 64,
				RTF_RGBA16f, false, TEXTUREGROUP_RenderTarget, TF_Bilinear);
			if (IsValid(MyRTDensityPreview) && IsValid(NinjaLive->MyMICompositeAndGradient))
			{
				UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, MyRTDensityPreview,
					NinjaLive->MyMICompositeAndGradient.Get());
			}
			if (IsValid(NinjaLive->MyMIOutput))
			{
				NinjaLive->MyMIOutput->SetTextureParameterValue(TEXT("DensityBuffer"), MyRTDensityPreview.Get());
			}
			break;
		case EMyInactiveBehaviour::GrayWhenInactive:
			if (IsValid(MyTraceMesh))
			{
				if (IsValid(NinjaLive->MyInactiveGrayMaterial))
				{
					MyTraceMesh->SetMaterial(0, NinjaLive->MyInactiveGrayMaterial.Get());
				}
				MyTraceMesh->SetVisibility(true);
			}
			break;
		case EMyInactiveBehaviour::HiddenWhenInactive:
			if (IsValid(MyTraceMesh))
			{
				MyTraceMesh->SetVisibility(false);
			}
			break;
		}


		if (NinjaLive->MyInitDone
			&& MyOverlapFilterInclusiveCollisionType.Contains(TEnumAsByte<ECollisionChannel>(ECC_Pawn)))
		{
			NinjaLive->MyResetTempArraySlots();
		}
		return;
	}


	switch (MyTraceMeshInactiveBehaviour)
	{
	case EMyInactiveBehaviour::GrayWhenInactive:
		if (IsValid(MyTraceMesh) && IsValid(NinjaLive->MyMIOutput))
		{
			MyTraceMesh->SetMaterial(0, NinjaLive->MyMIOutput.Get());
		}
		break;
	case EMyInactiveBehaviour::HiddenWhenInactive:

		if (IsValid(MyTraceMesh))
		{
			MyTraceMesh->SetVisibility(true);
		}
		break;
	case EMyInactiveBehaviour::HoldLastFrameWhenInactive:
	default:
		break;
	}
}

void AMyNinjaLiveActor::MyApplyInitialInactiveState(UMyNinjaLiveComponent* NinjaLive)
{
	if (!IsValid(MyTraceMesh))
	{
		return;
	}

	switch (MyTraceMeshInactiveBehaviour)
	{
	case EMyInactiveBehaviour::HoldLastFrameWhenInactive:
	case EMyInactiveBehaviour::GrayWhenInactive:

		if (IsValid(NinjaLive->MyInactiveGrayMaterial))
		{
			MyTraceMesh->SetMaterial(0, NinjaLive->MyInactiveGrayMaterial.Get());
		}
		MyTraceMesh->SetVisibility(true);
		break;
	case EMyInactiveBehaviour::HiddenWhenInactive:
		MyTraceMesh->SetVisibility(false);
		break;
	}
}

void AMyNinjaLiveActor::MySetInitialVisibility2()
{
	switch (MyTraceMeshInactiveBehaviour)
	{
	case EMyInactiveBehaviour::HoldLastFrameWhenInactive:
	case EMyInactiveBehaviour::GrayWhenInactive:

		if (IsValid(MyTraceMesh) && IsValid(MyInactiveGrayMaterial))
		{
			MyTraceMesh->SetMaterial(0, MyInactiveGrayMaterial);
		}
		break;
	case EMyInactiveBehaviour::HiddenWhenInactive:
		if (IsValid(MyTraceMesh))
		{
			MyTraceMesh->SetVisibility(false, false);
		}
		break;
	}
}

void AMyNinjaLiveActor::MyEndOverlapDetection()
{
	if (IsValid(MyInteractionVolumeTemplate))
	{
		MyPrepareInteractionOverlapBindings();
		MyInteractionVolumeTemplate->OnComponentEndOverlap.AddUniqueDynamic(
			this, &AMyNinjaLiveActor::MyEndOverlapComponent);
	}
}

bool AMyNinjaLiveActor::MyExcludeLargeObjects(const USceneComponent* OverlapComponent) const
{
	if (!MyAutoExcludeLargeOverlappingObjects)
	{
		return true;
	}

	FVector InteractionOrigin = FVector::ZeroVector;
	FVector InteractionBoxExtent = FVector::ZeroVector;
	float InteractionSphereRadius = 0.0f;
	UKismetSystemLibrary::GetComponentBounds(
		MyInteractionVolumeTemplate, InteractionOrigin, InteractionBoxExtent, InteractionSphereRadius);

	FVector OverlapOrigin = FVector::ZeroVector;
	FVector OverlapBoxExtent = FVector::ZeroVector;
	float OverlapSphereRadius = 0.0f;
	UKismetSystemLibrary::GetComponentBounds(
		OverlapComponent, OverlapOrigin, OverlapBoxExtent, OverlapSphereRadius);

	return InteractionBoxExtent.GetMax() < OverlapBoxExtent.GetMax();
}

bool AMyNinjaLiveActor::MyCollisionTypeFilter1(const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes,
	const UPrimitiveComponent* OverlapComponent, FString& ObjType,
	TEnumAsByte<ECollisionChannel>& CollisionType) const
{
	ObjType.Reset();
	CollisionType = ECC_WorldStatic;

	const UMyNinjaLiveComponent* NinjaLive = GetNinjaLiveComponent();
	if (!IsValid(OverlapComponent) || !IsValid(NinjaLive)
		|| OverlapComponent->GetCollisionResponseToChannel(NinjaLive->MyCollisionChannel) == ECR_Block)
	{
		return false;
	}

	const ECollisionChannel ComponentCollisionType = OverlapComponent->GetCollisionObjectType();
	const TEnumAsByte<EObjectTypeQuery>* MappedObjectType =
		MyOverlapFilterInclusiveCollisionType.Find(TEnumAsByte<ECollisionChannel>(ComponentCollisionType));
	const TEnumAsByte<EObjectTypeQuery> FilterObjectType = MappedObjectType != nullptr
		? *MappedObjectType
		: TEnumAsByte<EObjectTypeQuery>(EObjectTypeQuery::ObjectTypeQuery1);

	const UEnum* ObjectTypeEnum = StaticEnum<EObjectTypeQuery>();
	for (const TEnumAsByte<EObjectTypeQuery> ObjectType : ObjectTypes)
	{
		if (ObjectType != FilterObjectType)
		{
			continue;
		}

		ObjType = ObjectTypeEnum != nullptr
			? ObjectTypeEnum->GetNameStringByValue(ObjectType.GetValue())
			: FString();
		CollisionType = ComponentCollisionType;
		return true;
	}

	return false;
}

bool AMyNinjaLiveActor::MyCollisionTypeFilter2(const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes,
	const UPrimitiveComponent* OverlapComponent, FString& ObjType,
	TEnumAsByte<ECollisionChannel>& CollisionType) const
{

	return MyCollisionTypeFilter1(ObjectTypes, OverlapComponent, ObjType, CollisionType);
}

void AMyNinjaLiveActor::MyInitialOverlapCheck()
{
	UMyNinjaLiveComponent* NinjaLive = GetNinjaLiveComponent();
	if (!IsValid(NinjaLive))
	{
		return;
	}

	if (!NinjaLive->MyTraceChannelsSet)
	{
		if (UWorld* World = GetWorld(); World != nullptr
			&& !World->GetTimerManager().IsTimerActive(MyInitialOverlapCheckTimer))
		{
			World->GetTimerManager().SetTimer(MyInitialOverlapCheckTimer, this,
				&AMyNinjaLiveActor::MyInitialOverlapCheck, 0.01f, false);
		}
		return;
	}

	if (!IsValid(MyInteractionVolumeTemplate))
	{
		NinjaLive->MyOverlap1 = false;
		if (MyOverlappingActorsInitial.IsEmpty())
		{
			MyOverlappingActorsInitial.Add(this);
		}
		return;
	}

	TArray<UPrimitiveComponent*> ExistingOverlaps;
	MyInteractionVolumeTemplate->GetOverlappingComponents(ExistingOverlaps);
	for (UPrimitiveComponent* OverlapComponent : ExistingOverlaps)
	{
		if (!IsValid(OverlapComponent))
		{
			continue;
		}

		if (OverlapComponent->ComponentTags.Contains(MyTrackActorPrimitiveComponentsWithTag))
		{
			NinjaLive->MyOverlappingComponents.Add(OverlapComponent);
		}

		AActor* OverlapOwner = OverlapComponent->GetOwner();
		if (IsValid(OverlapOwner)
			&& OverlapOwner->GetComponentsByTag(USkeletalMeshComponent::StaticClass(),
				MyTrackActorSkeletalMeshComponentsWithTag).Num() != 0)
		{
			MyOverlappingActorsInitial.Add(OverlapOwner);
		}
	}

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Reserve(MyExcludeSpecificActorsFromOverlap.Num());
	for (const TObjectPtr<AActor>& ExcludedActor : MyExcludeSpecificActorsFromOverlap)
	{
		ActorsToIgnore.Add(ExcludedActor.Get());
	}

	TArray<UPrimitiveComponent*> FilteredOverlaps;
	UKismetSystemLibrary::ComponentOverlapComponents(MyInteractionVolumeTemplate,
		MyInteractionVolumeTemplate->GetComponentTransform(), MyOverlapFilterInclusiveObjType,
		nullptr, ActorsToIgnore, FilteredOverlaps);
	FilteredOverlaps.Remove(MyActivationVolume.Get());
	FilteredOverlaps.Remove(MyTraceMesh.Get());

	if (FilteredOverlaps.IsEmpty())
	{
		NinjaLive->MyOverlap1 = false;
	}
	else
	{
		NinjaLive->MyOverlap1 = true;
		for (UPrimitiveComponent* OverlapComponent : FilteredOverlaps)
		{
			if (!IsValid(OverlapComponent))
			{
				continue;
			}

			FString ObjType;
			TEnumAsByte<ECollisionChannel> CollisionType = ECC_WorldStatic;
			if (!MyCollisionTypeFilter1(MyOverlapFilterInclusiveObjType, OverlapComponent,
				ObjType, CollisionType))
			{
				continue;
			}

			if (OverlapComponent->GetCollisionObjectType() == ECC_Pawn)
			{
				AActor* OverlapOwner = OverlapComponent->GetOwner();
				if (!MyOverlappingActorsInitial.Contains(OverlapOwner))
				{
					MyOverlappingActorsInitial.Add(OverlapOwner);
				}
			}
			else if (MyExcludeLargeObjects(OverlapComponent)
				&& !NinjaLive->MyOverlappingComponents.Contains(OverlapComponent))
			{
				NinjaLive->MyOverlappingComponents.Add(OverlapComponent);
			}
		}
	}

	if (MyOverlappingActorsInitial.IsEmpty())
	{
		MyOverlappingActorsInitial.Add(this);
	}
}

void AMyNinjaLiveActor::MySetInteractionVolumeCollisionResponse()
{

	if (!UKismetSystemLibrary::GetEngineVersion().StartsWith(TEXT("5"), ESearchCase::IgnoreCase))
	{
		return;
	}


	const UMyNinjaLiveComponent* NinjaLive = GetNinjaLiveComponent();
	const int32 QuantizerStepSize = IsValid(NinjaLive) ? NinjaLive->MyQuantizerStepSize : 0;
	const int32 AxisLocked = IsValid(NinjaLive)
		? static_cast<int32>(NinjaLive->MyMovementIsLockedOnThisAxis) : 0;
	if (QuantizerStepSize <= 0 && AxisLocked == 4)
	{
		return;
	}

	if (!IsValid(MyInteractionVolumeTemplate))
	{
		return;
	}


	struct FChannelFilterEntry
	{
		EObjectTypeQuery ObjectType;
		ECollisionChannel Channel;
	};
	const FChannelFilterEntry ChannelFilters[] = {
		{ EObjectTypeQuery::ObjectTypeQuery6, ECC_Destructible },
		{ EObjectTypeQuery::ObjectTypeQuery5, ECC_Vehicle },
		{ EObjectTypeQuery::ObjectTypeQuery4, ECC_PhysicsBody },
		{ EObjectTypeQuery::ObjectTypeQuery3, ECC_Pawn },
		{ EObjectTypeQuery::ObjectTypeQuery2, ECC_WorldDynamic },
		{ EObjectTypeQuery::ObjectTypeQuery1, ECC_WorldStatic },
	};
	for (const FChannelFilterEntry& Entry : ChannelFilters)
	{
		if (!MyOverlapFilterInclusiveObjType.Contains(TEnumAsByte<EObjectTypeQuery>(Entry.ObjectType)))
		{
			MyInteractionVolumeTemplate->SetCollisionResponseToChannel(Entry.Channel, ECR_Ignore);
		}
	}
}

bool AMyNinjaLiveActor::MySimContainerCapacityFilter1(const TArray<bool>& TempArrays,
	const TMap<int32, UPrimitiveComponent*>& Pairs,
	const TArray<USkeletalMeshComponent*>& SKmeshComponents) const
{

	if (!TempArrays.Contains(true))
	{
		return false;
	}



	if (SKmeshComponents.Num() > 1)
	{
		return (TempArrays.Num() - Pairs.Num()) >= SKmeshComponents.Num();
	}

	return true;
}

void AMyNinjaLiveActor::MyBeginOverlapDetection()
{

	if (IsValid(MyInteractionVolumeTemplate))
	{
		MyPrepareInteractionOverlapBindings();
		MyInteractionVolumeTemplate->OnComponentBeginOverlap.AddUniqueDynamic(
			this, &AMyNinjaLiveActor::MyBeginOverlapComponent);
	}


	MySetInteractionVolumeCollisionResponse();


	UMyNinjaLiveComponent* NinjaLive = GetNinjaLiveComponent();
	for (const TObjectPtr<AActor>& Actor : MyOverlappingActorsInitial)
	{
		if (!IsValid(Actor) || !IsValid(NinjaLive))
		{
			continue;
		}

		MyInitialActorsProcessed = false;
		MyProcessOverlapActor(Actor.Get());
	}
	MyInitialActorsProcessed = true;
}

void AMyNinjaLiveActor::MyBeginOverlapComponent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UMyNinjaLiveComponent* NinjaLive = GetNinjaLiveComponent();
	if (!IsValid(NinjaLive) || !IsValid(OtherActor) || !IsValid(OtherComp))
	{
		return;
	}


	if (MyExcludeSpecificActorsFromOverlap.Contains(OtherActor))
	{
		return;
	}


	if (!OtherComp->ComponentTags.Contains(MyTrackActorPrimitiveComponentsWithTag))
	{

		bool bHasTaggedSkeletalMesh = false;
		TArray<USkeletalMeshComponent*> ActorSkeletalMeshes;
		OtherActor->GetComponents<USkeletalMeshComponent>(ActorSkeletalMeshes);
		for (USkeletalMeshComponent* SkeletalMesh : ActorSkeletalMeshes)
		{
			if (IsValid(SkeletalMesh) && SkeletalMesh->ComponentTags.Contains(MyTrackActorSkeletalMeshComponentsWithTag))
			{
				bHasTaggedSkeletalMesh = true;
				break;
			}
		}
		if (bHasTaggedSkeletalMesh)
		{
			MyProcessOverlapActor(OtherActor);
			return;
		}


		FString ObjType;
		TEnumAsByte<ECollisionChannel> CollisionType = ECC_WorldStatic;
		if (!MyCollisionTypeFilter2(MyOverlapFilterInclusiveObjType, OtherComp, ObjType, CollisionType))
		{
			return;
		}


		if (CollisionType == ECC_Pawn)
		{
			MyProcessOverlapActor(OtherActor);
			return;
		}
	}


	AActor* CompOwner = OtherComp->GetOwner();
	if (IsValid(CompOwner) && CompOwner->GetClass() == GetClass())
	{
		return;
	}


	if (NinjaLive->MyOverlappingComponents.Contains(OtherComp))
	{
		return;
	}


	if (!MyExcludeLargeObjects(OtherComp))
	{
		return;
	}
	NinjaLive->MyOverlappingComponents.Add(OtherComp);
	NinjaLive->MyOverlap1 = true;
}

void AMyNinjaLiveActor::MyProcessOverlapActor(AActor* Actor)
{
	UMyNinjaLiveComponent* NinjaLive = GetNinjaLiveComponent();
	if (!IsValid(NinjaLive) || !IsValid(Actor))
	{
		return;
	}


	if (MyOverlappingActors.Contains(Actor))
	{
		return;
	}


	MyOverlappingActors.Add(Actor);


	TArray<USkeletalMeshComponent*> SkeletalMeshes;
	{
		TArray<USkeletalMeshComponent*> AllSkeletalMeshes;
		Actor->GetComponents<USkeletalMeshComponent>(AllSkeletalMeshes);
		for (USkeletalMeshComponent* SkeletalMesh : AllSkeletalMeshes)
		{
			if (IsValid(SkeletalMesh) && SkeletalMesh->ComponentTags.Contains(MyTrackActorSkeletalMeshComponentsWithTag))
			{
				SkeletalMeshes.Add(SkeletalMesh);
			}
		}
		if (SkeletalMeshes.Num() == 0)
		{
			SkeletalMeshes = AllSkeletalMeshes;
		}
	}


	TMap<int32, UPrimitiveComponent*> Pairs;
	for (const TPair<int32, TObjectPtr<UPrimitiveComponent>>& Pair : NinjaLive->MySkeletalMeshTempArrayPairs)
	{
		Pairs.Add(Pair.Key, Pair.Value.Get());
	}
	if (!MySimContainerCapacityFilter1(NinjaLive->MyListOfAvailableTempArrays, Pairs, SkeletalMeshes))
	{
		return;
	}


	MyOverlapFilterInclusiveBoneNameExactTemp2 = MyOverlapFilterInclusiveBoneNameExact;


	for (USkeletalMeshComponent* SkeletalMesh : SkeletalMeshes)
	{
		if (!IsValid(SkeletalMesh))
		{
			continue;
		}

		const int32 ArrayIndex = NinjaLive->MyAcquireTempArraySlot();
		if (ArrayIndex == INDEX_NONE)
		{
			break;
		}

		const int32 NumBones = SkeletalMesh->GetNumBones();
		if (MyOverlapFilterInclusiveBoneNameExact.Num() != 0)
		{

			MyOverlapFilterInclusiveBoneNameExactTemp.Reset();
			for (int32 Index = 0; Index < NumBones; ++Index)
			{
				const FName BoneName = SkeletalMesh->GetBoneName(Index);
				if (MyOverlapFilterInclusiveBoneNameExactTemp2.Contains(BoneName))
				{
					MyOverlapFilterInclusiveBoneNameExactTemp.Add(BoneName);

					if (!MyForceTrackBonesWithSimilarName)
					{
						MyOverlapFilterInclusiveBoneNameExactTemp2.Remove(BoneName);
					}
				}
			}

			NinjaLive->MyAppendToTempArray(ArrayIndex, MyOverlapFilterInclusiveBoneNameExactTemp);
		}
		else
		{

			for (int32 Index = 0; Index < NumBones; ++Index)
			{
				const FName BoneName = SkeletalMesh->GetBoneName(Index);
				if (MyOverlapFilterInclusiveBoneNamePartial.Num() == 0)
				{
					NinjaLive->MyAddToTempArray(ArrayIndex, BoneName);
					continue;
				}

				const FString BoneNameString = BoneName.ToString();
				for (const FString& Partial : MyOverlapFilterInclusiveBoneNamePartial)
				{
					if (BoneNameString.Contains(Partial, ESearchCase::IgnoreCase))
					{
						NinjaLive->MyAddToTempArray(ArrayIndex, BoneName);
						break;
					}
				}
			}
		}

		if (NinjaLive->MyGetTempArrayRef(ArrayIndex).IsEmpty())
		{
			NinjaLive->MyReleaseTempArraySlot(ArrayIndex);
			continue;
		}
		NinjaLive->MySkeletalMeshTempArrayPairs.Emplace(ArrayIndex, SkeletalMesh);
	}


	NinjaLive->MyOverlap1 = true;
}

bool AMyNinjaLiveActor::MyReleaseSkeletalSlotsForActor(AActor* Actor)
{
	UMyNinjaLiveComponent* NinjaLive = GetNinjaLiveComponent();
	if (!IsValid(NinjaLive) || !IsValid(Actor))
	{
		return false;
	}

	TArray<int32> SlotsToRelease;
	for (const TPair<int32, TObjectPtr<UPrimitiveComponent>>& Pair : NinjaLive->MySkeletalMeshTempArrayPairs)
	{
		if (IsValid(Pair.Value) && Pair.Value->GetOwner() == Actor)
		{
			SlotsToRelease.Add(Pair.Key);
		}
	}

	for (const int32 Slot : SlotsToRelease)
	{
		NinjaLive->MyReleaseTempArraySlot(Slot);
	}
	return !SlotsToRelease.IsEmpty();
}

void AMyNinjaLiveActor::MyEndOverlapComponent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!IsValid(OtherComp))
	{

		return;
	}


	UMyNinjaLiveComponent* NinjaLive = GetNinjaLiveComponent();
	if (!IsValid(NinjaLive))
	{
		return;
	}

	const bool bActorStillOverlapping = IsValid(MyInteractionVolumeTemplate) && IsValid(OtherActor)
		&& MyInteractionVolumeTemplate->IsOverlappingActor(OtherActor);
	if (!bActorStillOverlapping && MyOverlappingActors.Contains(OtherActor))
	{
		MyOverlappingActors.Remove(OtherActor);
		MyReleaseSkeletalSlotsForActor(OtherActor);
	}

	NinjaLive->MyOverlappingComponents.Remove(OtherComp);
	NinjaLive->MyOverlap1 = !NinjaLive->MySkeletalMeshTempArrayPairs.IsEmpty()
		|| !NinjaLive->MyOverlappingComponents.IsEmpty();
}

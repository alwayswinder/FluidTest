// MyNinjaLiveActor.cpp — AMyNinjaLiveActor 实现

#include "MyNinjaLiveActor.h"

#include "MyNinjaLiveComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"

AMyNinjaLiveActor::AMyNinjaLiveActor()
{
	// 不启用 Tick
	PrimaryActorTick.bCanEverTick = false;

	// 创建根 SceneComponent
	MyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = MyRoot;

	// 创建激活体积（BoxCollision），挂载在 Root 下，默认隐藏
	MyActivationVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("MyActivationVolume"));
	MyActivationVolume->SetupAttachment(MyRoot);
	MyActivationVolume->SetBoxExtent(FVector(200.0f, 200.0f, 100.0f));
	MyActivationVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MyActivationVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	MyActivationVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	MyActivationVolume->SetGenerateOverlapEvents(true);
	MyActivationVolume->SetHiddenInGame(true);

	// 创建交互体积（BoxCollision），挂载在 Root 下，默认隐藏
	MyInteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("MyInteractionVolume"));
	MyInteractionVolume->SetupAttachment(MyRoot);
	MyInteractionVolume->SetBoxExtent(FVector(200.0f, 200.0f, 100.0f));
	MyInteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MyInteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	MyInteractionVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	MyInteractionVolume->SetGenerateOverlapEvents(true);
	MyInteractionVolume->SetHiddenInGame(true);

	// 创建追踪网格（StaticMesh，用于射线检测交互，MyUseTraceMeshAsInteractionVolume 时替代交互体积）
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

void AMyNinjaLiveActor::MySetInitialVisibility2()
{
	switch (MyTraceMeshInactiveBehaviour)
	{
	case EMyInactiveBehaviour::HoldLastFrameWhenInactive:
	case EMyInactiveBehaviour::GrayWhenInactive:
		// 蓝图 0/1 两个分支共用：把 TraceMesh 材质替换为 InactiveGrayMaterial。
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
		MyInteractionVolumeTemplate->OnComponentEndOverlap.AddDynamic(
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
	// 与 CollisionTypeFilter1 同构：非阻塞响应时在过滤数组中查找对象类型映射，首个命中输出 ObjType/CollisionType。
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
	// 蓝图：引擎版本字符串（GetEngineVersion）不以 “5” 开头（忽略大小写）时直接返回，不做任何设置。
	if (!UKismetSystemLibrary::GetEngineVersion().StartsWith(TEXT("5"), ESearchCase::IgnoreCase))
	{
		return;
	}

	// 蓝图：量化步长 > 0 或锁轴枚举值 != 4（EMyQuantizerAxisIgnore::None）时才执行通道设置。
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

	// 蓝图链式六段：MyOverlapFilterInclusiveObjType 未包含对应对象类型时，把该碰撞通道响应设为 Ignore。
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
	// 蓝图 IfThenElse_2：临时数组槽位列表中没有任何可用槽位（true）时整个复合直接结束，不走 then 出口。
	if (!TempArrays.Contains(true))
	{
		return false;
	}

	// 蓝图 IfThenElse_1：骨骼网格组件只有一个或没有时直接走 then 出口；
	// 多于一个时才检查剩余槽位（总槽位 - 已占用）是否足够容纳全部骨骼网格（IfThenElse_0）。
	if (SKmeshComponents.Num() > 1)
	{
		return (TempArrays.Num() - Pairs.Num()) >= SKmeshComponents.Num();
	}

	return true;
}

void AMyNinjaLiveActor::MyBeginOverlapDetection()
{
	// 蓝图 Sequence then_0：把交互体积的 BeginOverlap 委托绑定到 BeginOverlapComponent 事件体。
	if (IsValid(MyInteractionVolumeTemplate))
	{
		MyInteractionVolumeTemplate->OnComponentBeginOverlap.AddDynamic(
			this, &AMyNinjaLiveActor::MyBeginOverlapComponent);
	}

	// 蓝图 AddDelegate then 后紧接着执行：按过滤对象类型关闭交互体积对应通道的响应。
	MySetInteractionVolumeCollisionResponse();

	// 蓝图 Sequence then_1：遍历初始重叠 Actor，逐个走骨骼追踪流程（InitialActorsProcessed 处理中置 false）。
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

	// 蓝图 IfThenElse_4：排除列表中的 Actor 不处理（then 无连接）。
	if (MyExcludeSpecificActorsFromOverlap.Contains(OtherActor))
	{
		return;
	}

	// 蓝图 IfThenElse_1：组件带追踪标签时跳过下方过滤，直接进入所有者检查（else → IfThenElse_79）。
	if (!OtherComp->ComponentTags.Contains(MyTrackActorPrimitiveComponentsWithTag))
	{
		// 蓝图 IfThenElse_2：OtherActor 带追踪标签的骨骼网格数量不为 0 时直接走骨骼追踪（then）。
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

		// 蓝图 IfThenElse_8：碰撞类型过滤，未命中直接结束（else 无连接）。
		FString ObjType;
		TEnumAsByte<ECollisionChannel> CollisionType = ECC_WorldStatic;
		if (!MyCollisionTypeFilter2(MyOverlapFilterInclusiveObjType, OtherComp, ObjType, CollisionType))
		{
			return;
		}

		// 蓝图 IfThenElse_80：Pawn 通道的组件走骨骼追踪，否则落入 While 所有者检查（else → IfThenElse_79）。
		if (CollisionType == ECC_Pawn)
		{
			MyProcessOverlapActor(OtherActor);
			return;
		}
	}

	// 蓝图 IfThenElse_79：所有者是同类 NinjaLive Actor 时忽略（then 无连接）。
	AActor* CompOwner = OtherComp->GetOwner();
	if (IsValid(CompOwner) && CompOwner->GetClass() == GetClass())
	{
		return;
	}

	// 蓝图 IfThenElse_29：已在跟踪组件列表中的组件忽略（else 无连接）。
	if (NinjaLive->MyOverlappingComponents.Contains(OtherComp))
	{
		return;
	}

	// 蓝图 IfThenElse_6：排除过大的组件，通过后加入跟踪列表并标记有重叠。
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

	// 蓝图 IfThenElse_0：已在 MyOverlappingActors 中则跳过（else 无连接）。
	if (MyOverlappingActors.Contains(Actor))
	{
		return;
	}

	// 蓝图 Array_Add：加入 MyOverlappingActors。
	MyOverlappingActors.Add(Actor);

	// 蓝图 Select_1：优先带追踪标签的骨骼网格；没有时退回全部骨骼网格。
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

	// 蓝图 IfThenElse_7：可用临时数组槽位不足以容纳全部骨骼网格时结束（else 无连接）。
	TMap<int32, UPrimitiveComponent*> Pairs;
	for (const TPair<int32, TObjectPtr<UPrimitiveComponent>>& Pair : NinjaLive->MySkeletalMeshTempArrayPairs)
	{
		Pairs.Add(Pair.Key, Pair.Value.Get());
	}
	if (!MySimContainerCapacityFilter1(NinjaLive->MyListOfAvailableTempArrays, Pairs, SkeletalMeshes))
	{
		return;
	}

	// 蓝图 VariableSet_4：把精确骨骼名复制到候选数组 Temp2 后再遍历骨骼网格。
	MyOverlapFilterInclusiveBoneNameExactTemp2 = MyOverlapFilterInclusiveBoneNameExact;

	// 蓝图 MacroInstance_34：逐个骨骼网格分配骨骼名与临时数组槽位，全部完成后标记有重叠。
	for (USkeletalMeshComponent* SkeletalMesh : SkeletalMeshes)
	{
		if (!IsValid(SkeletalMesh))
		{
			continue;
		}

		const int32 NumBones = SkeletalMesh->GetNumBones();
		if (MyOverlapFilterInclusiveBoneNameExact.Num() != 0)
		{
			// 有精确骨骼名：清空收集数组，逐骨骼做精确匹配（节点注释：部分名被忽略）。
			MyOverlapFilterInclusiveBoneNameExactTemp.Reset();
			for (int32 Index = 0; Index < NumBones; ++Index)
			{
				const FName BoneName = SkeletalMesh->GetBoneName(Index);
				if (MyOverlapFilterInclusiveBoneNameExactTemp2.Contains(BoneName))
				{
					MyOverlapFilterInclusiveBoneNameExactTemp.Add(BoneName);
					// 蓝图 IfThenElse_5：未启用相似名追踪时把命中的骨骼名移出候选，避免后续重复。
					if (!MyForceTrackBonesWithSimilarName)
					{
						MyOverlapFilterInclusiveBoneNameExactTemp2.Remove(BoneName);
					}
				}
			}

			// 占槽：把收集到的精确骨骼名追加到临时数组槽位并登记映射（Map_Add），槽位置为已用。
			const int32 ArrayIndex = NinjaLive->MyListOfAvailableTempArrays.Find(true);
			if (ArrayIndex != INDEX_NONE)
			{
				NinjaLive->MyAppendToTempArray(ArrayIndex, MyOverlapFilterInclusiveBoneNameExactTemp);
				NinjaLive->MySkeletalMeshTempArrayPairs.Emplace(ArrayIndex, SkeletalMesh);
				NinjaLive->MyListOfAvailableTempArrays[ArrayIndex] = false;
			}
		}
		else
		{
			// 无精确骨骼名：部分名数组为空时全部骨骼加入，否则按包含匹配（忽略大小写）加入。
			for (int32 Index = 0; Index < NumBones; ++Index)
			{
				const FName BoneName = SkeletalMesh->GetBoneName(Index);
				if (MyOverlapFilterInclusiveBoneNamePartial.Num() == 0)
				{
					const int32 ArrayIndex = NinjaLive->MyListOfAvailableTempArrays.Find(true);
					if (ArrayIndex != INDEX_NONE)
					{
						NinjaLive->MyAddToTempArray(ArrayIndex, BoneName);
					}
					continue;
				}

				const FString BoneNameString = BoneName.ToString();
				for (const FString& Partial : MyOverlapFilterInclusiveBoneNamePartial)
				{
					if (BoneNameString.Contains(Partial, ESearchCase::IgnoreCase))
					{
						const int32 ArrayIndex = NinjaLive->MyListOfAvailableTempArrays.Find(true);
						if (ArrayIndex != INDEX_NONE)
						{
							NinjaLive->MyAddToTempArray(ArrayIndex, BoneName);
						}
					}
				}
			}

			// 收尾：登记该骨骼网格到临时数组槽位并置为已用（MacroInstance_38 完成后 Map_Add + SetRef）。
			const int32 ArrayIndex = NinjaLive->MyListOfAvailableTempArrays.Find(true);
			if (ArrayIndex != INDEX_NONE)
			{
				NinjaLive->MySkeletalMeshTempArrayPairs.Emplace(ArrayIndex, SkeletalMesh);
				NinjaLive->MyListOfAvailableTempArrays[ArrayIndex] = false;
			}
		}
	}

	// 蓝图 MacroInstance_34 Completed → 标记有重叠。
	NinjaLive->MyOverlap1 = true;
}

void AMyNinjaLiveActor::MyEndOverlapComponent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!IsValid(OtherComp))
	{
		// 事件体没有前导 IsValid 检查；但 C++ 侧防御空指针。
		return;
	}

	// 无碰撞对象且未开启强制追踪标志时不做任何事（蓝图 if 的 then 未连接）。
	if (OtherComp->GetCollisionEnabled() == ECollisionEnabled::NoCollision && MyForceTrackObjectsWithNocollisionFlag)
	{
		return;
	}

	UMyNinjaLiveComponent* NinjaLive = GetNinjaLiveComponent();
	if (!IsValid(NinjaLive))
	{
		return;
	}

	if (OtherComp->GetCollisionObjectType() == ECollisionChannel::ECC_Pawn)
	{
		if (MyOverlappingActors.Contains(OtherActor))
		{
			MyOverlappingActors.Remove(OtherActor);

			// 遍历该 Actor 的 SkeletalMesh，清理对应的临时数组槽位与映射。
			TArray<USkeletalMeshComponent*> SkeletalMeshes;
			if (IsValid(OtherActor))
			{
				OtherActor->GetComponents<USkeletalMeshComponent>(SkeletalMeshes);
			}
			bool bFoundMatch = false;
			for (USkeletalMeshComponent* SkeletalMesh : SkeletalMeshes)
			{
				int32 FoundKey = INDEX_NONE;
				for (const TPair<int32, TObjectPtr<UPrimitiveComponent>>& Pair : NinjaLive->MySkeletalMeshTempArrayPairs)
				{
					if (Pair.Value == SkeletalMesh)
					{
						FoundKey = Pair.Key;
						break;
					}
				}
				if (FoundKey != INDEX_NONE)
				{
					NinjaLive->MyListOfAvailableTempArrays[FoundKey] = true;
					NinjaLive->MyGetTempArray(FoundKey).Reset();
					NinjaLive->MySkeletalMeshTempArrayPairs.Remove(FoundKey);
					bFoundMatch = true;
				}
			}
			// 只在至少命中一个 Map 匹配（执行 Map_Remove 的 then）时更新 MyOverlap1，与蓝图一致；
			// 无匹配 SkeletalMesh 或 NotContains 分支的 else 均为空，不更新。
			if (bFoundMatch)
			{
				NinjaLive->MyOverlap1 = NinjaLive->MySkeletalMeshTempArrayPairs.Num() != 0
					|| NinjaLive->MyOverlappingComponents.Num() != 0;
			}
		}
	}
	else
	{
		// 非 Pawn 分支：两条路径（Find==-1 直接跳过移除，或 Find!=-1 移除）最终都会更新 MyOverlap1。
		NinjaLive->MyOverlappingComponents.Remove(OtherComp);
		NinjaLive->MyOverlap1 = NinjaLive->MySkeletalMeshTempArrayPairs.Num() != 0
			|| NinjaLive->MyOverlappingComponents.Num() != 0;
	}
}

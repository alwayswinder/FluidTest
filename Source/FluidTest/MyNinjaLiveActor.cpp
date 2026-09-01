// MyNinjaLiveActor.cpp — AMyNinjaLiveActor 实现

#include "MyNinjaLiveActor.h"

#include "MyNinjaLiveComponent.h"

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

// MyNinjaLiveActor.h — FluidNinjaLive 的 NinjaLive 蓝图对应的 C++ 父类（逐步迁移）
// 数组等数据属于 NinjaLiveComponent（见 UMyNinjaLiveComponent），Actor 通过组件引用访问。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyNinjaFluidEnums.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstance.h"
#include "MyNinjaLiveActor.generated.h"

class UMyNinjaLiveComponent;

/**
 * NinjaLive 蓝图 Actor 的 C++ 父类。
 */
UCLASS(Blueprintable, BlueprintType)
class FLUIDTEST_API AMyNinjaLiveActor : public AActor
{
	GENERATED_BODY()

public:
	AMyNinjaLiveActor();

	/** 获取 NinjaLiveComponent 组件引用 */
	UFUNCTION(BlueprintPure, Category = "FluidSim|Component")
	UMyNinjaLiveComponent* GetNinjaLiveComponent() const;

	/** 用户输入方式（UserInput_Enum，默认 Mouse single） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Input")
	EMyUserInput MyUserInputBasedInteraction = EMyUserInput::MouseSingle;

	/** 多触点输入中允许参与追踪的手指索引。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Input")
	TArray<bool> MyMultipleTouchLookup;

	/** 是否在 Pawn 靠近时激活模拟（性能优化，关闭则始终运行） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	bool MySimActivatedByPawnProximity = false;

	/** 是否禁用蓝图（关闭模拟） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	bool MyDisableBlueprint = false;

	/** 是否使用 TraceMesh 作为交互体积 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	bool MyUseTraceMeshAsInteractionVolume = false;

	/** 非持续交互时用于定位交互骨骼的包含列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<FName> MyOverlapFilterInclusiveBoneNameExact;

	/** 对应蓝图变量 OverlapFilterInclusiveObjType：初始重叠查询包含的对象类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<TEnumAsByte<EObjectTypeQuery>> MyOverlapFilterInclusiveObjType = {
		EObjectTypeQuery::ObjectTypeQuery1 };

	/** 对应蓝图变量 ExcludeSpecificActorsFromOverlap：初始重叠查询要忽略的 Actor。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<TObjectPtr<AActor>> MyExcludeSpecificActorsFromOverlap;

	/** 对应蓝图变量 TrackActorPrimitiveComponentsWithTag：允许加入 Primitive 重叠列表的组件标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	FName MyTrackActorPrimitiveComponentsWithTag = NAME_None;

	/** 对应蓝图变量 TrackActorSkeletalMeshComponentsWithTag：允许加入初始 Actor 列表的组件标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	FName MyTrackActorSkeletalMeshComponentsWithTag = NAME_None;

	/** 对应蓝图变量 OverlappingActorsInitial：初始化时识别出的重叠 Actor。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FluidSim|Interaction")
	TArray<TObjectPtr<AActor>> MyOverlappingActorsInitial;

	/** 对应蓝图变量 OverlapFilterInclusiveCollisionType：碰撞通道到对象类型查询的包含映射。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TMap<TEnumAsByte<ECollisionChannel>, TEnumAsByte<EObjectTypeQuery>> MyOverlapFilterInclusiveCollisionType;

	/** 根 SceneComponent */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FluidSim|Activation")
	TObjectPtr<USceneComponent> MyRoot = nullptr;

	/** 激活体积（BoxCollision，Pawn 进入时激活模拟，挂载在 MyRoot 下） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FluidSim|Activation")
	TObjectPtr<UBoxComponent> MyActivationVolume = nullptr;
	/** 交互体积（BoxCollision，用于交互输入） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FluidSim|Interaction")
	TObjectPtr<UBoxComponent> MyInteractionVolume = nullptr;
	/** 追踪网格（StaticMesh，用于射线检测交互） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FluidSim|Trace")
	TObjectPtr<UStaticMeshComponent> MyTraceMesh = nullptr;

	/** 对应蓝图变量 TraceMeshInactiveBehaviour：TraceMesh 不活动时的呈现行为。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	EMyInactiveBehaviour MyTraceMeshInactiveBehaviour = EMyInactiveBehaviour::HoldLastFrameWhenInactive;

	/** 对应蓝图变量 InactiveGrayMaterial：不活动时替换 TraceMesh 的灰色材质。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	TObjectPtr<UMaterialInstance> MyInactiveGrayMaterial = nullptr;

	/** 对应蓝图变量 InteractionVolumeTemplate：绑定结束重叠委托的交互体积组件。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TObjectPtr<UPrimitiveComponent> MyInteractionVolumeTemplate = nullptr;

	/** 对应蓝图变量 OverlappingActors：当前与交互体积重叠的 Actor 列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<TObjectPtr<AActor>> MyOverlappingActors;

	/** 对应蓝图变量 ForceTrackObjectsWithNocollisionFlag：是否强制追踪无碰撞对象。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	bool MyForceTrackObjectsWithNocollisionFlag = false;

	/** 对应蓝图变量 AutoExcludeLargeOverlappingObjects：是否排除大于交互体积的重叠组件。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	bool MyAutoExcludeLargeOverlappingObjects = false;

	/** 执行 SetInitialVisibility_2 复合：按 TraceMeshInactiveBehaviour 设置初始材质（灰色）或隐藏。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Trace")
	void MySetInitialVisibility2();

	/** 执行 EndOverlapDetection 复合：把 InteractionVolumeTemplate 的结束重叠委托绑定到 MyEndOverlapComponent。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Interaction")
	void MyEndOverlapDetection();

	/** 执行 ExcludeLargeObjects 复合：返回原复合节点的 true 分支条件。 */
	UFUNCTION(BlueprintPure, Category = "FluidSim|Interaction")
	bool MyExcludeLargeObjects(const USceneComponent* OverlapComponent) const;

	/** 执行 CollisionTypeFilter1 复合：筛选可处理的初始重叠对象类型。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Interaction")
	bool MyCollisionTypeFilter1(const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes,
		const UPrimitiveComponent* OverlapComponent, FString& ObjType,
		TEnumAsByte<ECollisionChannel>& CollisionType) const;

	/** 执行 InitialOverlapCheck 复合：等待碰撞通道就绪后建立初始重叠对象列表。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Interaction")
	void MyInitialOverlapCheck();

	/** 执行 BeginOverlapDetection > UE5 - SetInteractionVolumeCollisionResponse 复合：UE5 下按省略的过滤对象类型关闭交互体积对应通道的响应。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Interaction")
	void MySetInteractionVolumeCollisionResponse();

	/** 执行 BeginOverlapDetection > SimContainerCapacityFilter1 复合：返回是否走 then 出口（可用临时数组槽位足够容纳全部骨骼网格，且至少有一个可用槽位）。 */
	UFUNCTION(BlueprintPure, Category = "FluidSim|Interaction")
	bool MySimContainerCapacityFilter1(const TArray<bool>& TempArrays,
		const TMap<int32, UPrimitiveComponent*>& Pairs,
		const TArray<USkeletalMeshComponent*>& SKmeshComponents) const;

	/** 结束重叠回调：清理重叠组件/骨骼映射与临时数组槽位，并刷新 MyOverlap1。 */
	UFUNCTION()
	void MyEndOverlapComponent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	/** InitialOverlapCheck 等待追踪通道就绪的定时器。 */
	FTimerHandle MyInitialOverlapCheckTimer;
};

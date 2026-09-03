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
#include "Engine/EngineTypes.h"
#include "Materials/MaterialInstance.h"
#include "MyNinjaLiveActor.generated.h"

class UMyNinjaLiveComponent;
class UTextureRenderTarget2D;

/**
 * NinjaLive 蓝图 Actor 的 C++ 父类。
 */
UCLASS(Blueprintable, BlueprintType)
class FLUIDTEST_API AMyNinjaLiveActor : public AActor
{
	GENERATED_BODY()

public:
	AMyNinjaLiveActor();

	/** BeginPlay 初始化：按 DisableBlueprint / Pawn 接近激活分支执行激活体积、追踪网格与重叠检测设置。 */
	virtual void BeginPlay() override;

	/** Tick 事件：Pawn 接近激活检测、激活体积重叠配置与 TraceMesh 不活动状态切换。 */
	virtual void Tick(float DeltaSeconds) override;

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

	/** 对应蓝图变量 BeginPlaySupressed：Pawn 接近激活时抑制 BeginPlay 初始化的标记。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	bool MyBeginPlaySupressed = false;

	/** 对应蓝图变量 ActivationVolumeSize：激活体积盒体半尺寸（BeginPlay 中乘 50 后应用，默认值暂定）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	FVector MyActivationVolumeSize = FVector(4.0f, 4.0f, 2.0f);

	/** 对应蓝图变量 PawnInsideActivationBounds：Pawn 是否位于激活体积内（Tick 接近检测更新）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	bool MyPawnInsideActivationBounds = false;

	/** 对应蓝图变量 Activator：Pawn 接近激活检查的目标（为空时回退到 0 号玩家 Pawn）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	TObjectPtr<AActor> MyActivator = nullptr;

	/** 对应蓝图变量 ActivatorType：对激活者开启重叠响应的碰撞通道。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	TEnumAsByte<ECollisionChannel> MyActivatorType = ECC_Pawn;

	/** 对应蓝图变量 ActivatorProximityCheckFrequency：接近检测的间隔秒数（Delay 时长）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	double MyActivatorProximityCheckFrequency = 0.1;

	/** 对应蓝图变量 DeltaSeconds：最近一次 Tick 的时间增量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Simulation")
	double MyDeltaSeconds = 0.0;

	/** 对应蓝图变量 RT_DensityPreview：Pawn 离开时绘制密度缓冲预览的渲染目标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Activation")
	TObjectPtr<UTextureRenderTarget2D> MyRTDensityPreview = nullptr;

	/** 非持续交互时用于定位交互骨骼的包含列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<FName> MyOverlapFilterInclusiveBoneNameExact;

	/** 对应蓝图变量 OverlapFilterInclusiveBoneNameExactTemp：精确骨骼名匹配过程中暂存的骨骼名列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<FName> MyOverlapFilterInclusiveBoneNameExactTemp;

	/** 对应蓝图变量 OverlapFilterInclusiveBoneNameExactTemp2：精确骨骼名匹配用的候选列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<FName> MyOverlapFilterInclusiveBoneNameExactTemp2;

	/** 对应蓝图变量 OverlapFilterInclusiveBoneNamePartial：按部分名称匹配的骨骼名过滤数组。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<FString> MyOverlapFilterInclusiveBoneNamePartial;

	/** 对应蓝图变量 ForceTrackBonesWithSimilarName：允许追踪相似名称的额外骨骼。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	bool MyForceTrackBonesWithSimilarName = false;

	/** 对应蓝图变量 InitialActorsProcessed：初始重叠 Actor 是否已在 BeginOverlapDetection 中处理过。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	bool MyInitialActorsProcessed = false;

	/** 对应蓝图变量 OverlapFilterInclusiveObjType：初始重叠查询包含的对象类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<TEnumAsByte<EObjectTypeQuery>> MyOverlapFilterInclusiveObjType = {
		EObjectTypeQuery::ObjectTypeQuery1 };

	/** 对应蓝图变量 OverlapBasedInteraction：是否基于重叠方式交互（BeginPlay 时同步给组件）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	bool MyOverlapBasedInteraction = false;

	/** 对应蓝图变量 NinjaLIVECollisionExclude：BeginPlay 收集的同蓝图实例列表，追加进排除重叠列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	TArray<TObjectPtr<AActor>> MyNinjaLIVECollisionExclude;

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

	/** 对应蓝图变量 InteractionVolumeSize：交互体积盒体半尺寸（BeginPlay 中乘 50 后应用，默认值暂定）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Interaction")
	FVector MyInteractionVolumeSize = FVector(4.0f, 4.0f, 2.0f);

	/** 对应蓝图变量 TraceMeshSize：BeginPlay 中应用到 TraceMesh 的世界缩放。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Trace")
	FVector MyTraceMeshSize = FVector::OneVector;

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

	/** 执行 BeginOverlapDetection > CollisionTypeFilter2 复合：筛选可处理的交互组件对象类型（首个命中输出 ObjType / CollisionType）。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Interaction")
	bool MyCollisionTypeFilter2(const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes,
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

	/** 执行 BeginOverlapDetection 复合：绑定交互体积的重叠开始委托，并处理初始重叠 Actor 的骨骼追踪。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Interaction")
	void MyBeginOverlapDetection();

	/** 重叠开始事件回调（BeginOverlapComponent）：按过滤规则处理进入交互体积的组件/Actor。 */
	UFUNCTION()
	void MyBeginOverlapComponent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** 结束重叠回调：清理重叠组件/骨骼映射与临时数组槽位，并刷新 MyOverlap1。 */
	UFUNCTION()
	void MyEndOverlapComponent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	/** InitialOverlapCheck 等待追踪通道就绪的定时器。 */
	FTimerHandle MyInitialOverlapCheckTimer;

	/** 首次 Tick 的 DoOnce 门：只开启一次激活体积的重叠事件与激活者通道响应。 */
	bool MyActivatorSetupDone = false;

	/** 首次接近检查（Pawn 不在激活体积内）不活动状态布置的 DoOnce 门。 */
	bool MyInactiveShownOnce = false;

	/** 接近检测间隔定时器（对应蓝图 Delay）。 */
	FTimerHandle MyProximityCheckTimer;

	/** 延迟到期后检查：激活者是否进入/离开激活体积，并同步组件与 TraceMesh 状态。 */
	void MyProximityCheck();

	/** 首次 Pawn 不在激活体积内时按 InactiveBehaviour 布置 TraceMesh（SwitchEnum_1）。 */
	void MyApplyInitialInactiveState(UMyNinjaLiveComponent* NinjaLive);

	/** 处理单个候选 Actor（初始重叠或重叠开始事件）：加入 MyOverlappingActors、检查容量、为骨骼分配槽位。 */
	void MyProcessOverlapActor(AActor* Actor);
};

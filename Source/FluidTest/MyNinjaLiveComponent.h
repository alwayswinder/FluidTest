// MyNinjaLiveComponent.h — 复刻 FluidNinjaLive 的 NinjaLiveComponent 蓝图（逐步迁移）
//
// 迁移策略（小步增量，逐步替换蓝图）：
//   第 1 步：创建极简 C++ 父类，蓝图 NinjaLiveComponent 改为继承它（已完成）。
//   第 2 步（本文件）：迁移蓝图函数 ResetTempArrays 及其 40 个临时数组变量到 C++。
//             蓝图原有逻辑保持可用；C++ 侧新增同名（带 My 前缀）实现供逐步替换。
//   后续步骤：每次把蓝图中的一个函数/变量迁移为 C++ 虚函数/属性，
//             迁移一次、测试一次，直到完整替换。
//
// 蓝图位置：/Game/_MyTest/Fluid/Bp/NinjaLiveComponent（已从原 FluidNinjaLive 目录移入测试目录）
// 类名/函数/变量前缀 My：避免与蓝图内同名类型混淆。
// 注意：修改蓝图父类的操作在编辑器中进行（改蓝图后需重新编译保存），
//       C++ 侧只负责提供父类骨架。

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "MyNinjaFluidEnums.h"
#include "MyNinjaLiveComponent.generated.h"

/**
 * NinjaLiveComponent 蓝图组件的 C++ 父类。
 * 已迁移内容：
 *   - 函数 ResetTempArrays → MyResetTempArrays（清空 40 个临时 Name 数组）
 *   - 变量 TempArray0~TempArray39 → MyTempArray0~MyTempArray39
 * 蓝图侧同名函数/变量保留，待逐步迁移后删除。
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup = (FluidSim), meta = (BlueprintSpawnableComponent))
class FLUIDTEST_API UMyNinjaLiveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMyNinjaLiveComponent();

	// ------------------------------------------------------------------
	// 临时数组（复刻蓝图 TempArray0~TempArray39）
	// 蓝图类型为 Name 数组，对应 C++ TArray<FName>
	// ------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray4;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray6;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray7;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray8;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray9;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray11;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray12;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray13;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray14;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray15;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray16;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray17;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray18;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray19;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray20;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray21;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray22;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray23;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray24;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray25;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray26;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray27;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray28;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray29;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray30;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray31;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray32;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray33;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray34;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray35;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray36;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray37;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray38;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	TArray<FName> MyTempArray39;

	// ------------------------------------------------------------------
	// 复刻蓝图变量 RenderTargetsMap（Map：string → TextureRenderTarget2D）
	// ------------------------------------------------------------------
	/** RenderTarget 映射表（复刻蓝图 RenderTargetsMap，键为 string） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|RT")
	TMap<FString, TObjectPtr<UTextureRenderTarget2D>> MyRenderTargetsMap;

	// ------------------------------------------------------------------
	// 复刻蓝图变量 MapLengthTmp（int）
	// ------------------------------------------------------------------
	/** Map 长度临时值（复刻蓝图 MapLengthTmp） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Temp")
	int32 MyMapLengthTmp = 0;

	// ------------------------------------------------------------------
	// 复刻蓝图变量（VelocityHandlerForSimArea 用）
	// ------------------------------------------------------------------
	/** 模拟区域运动速度（复刻蓝图 VeloFromSimAreaMotion，double） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	double MyVeloFromSimAreaMotion = 0.0;

	/** TraceMesh 组件引用（复刻蓝图 TraceMeshComponent） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	TObjectPtr<UStaticMeshComponent> MyTraceMeshComponent = nullptr;

	/** TraceMesh 父级当前位置（复刻蓝图 TraceMeshParentPos） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	FVector MyTraceMeshParentPos = FVector::ZeroVector;

	/** TraceMesh 父级上一帧位置（复刻蓝图 TraceMeshParentLastPos） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Velocity")
	FVector MyTraceMeshParentLastPos = FVector::ZeroVector;

	// ------------------------------------------------------------------
	// 复刻蓝图变量（Enable OWNER Input 复合节点用）
	// ------------------------------------------------------------------
	/** 用户输入方式（复刻蓝图 UserInputBasedInteraction，UserInput_Enum，默认 None） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Input")
	EMyUserInput MyUserInputBasedInteraction = EMyUserInput::None;

	/** 是否显示鼠标光标（复刻蓝图 ShowMouseCursor） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FluidSim|Input")
	bool MyShowMouseCursor = true;

	// ------------------------------------------------------------------
	// 复刻蓝图函数 ResetTempArrays
	// 蓝图实现：按顺序对 TempArray0~39 调用 Array_Clear
	// ------------------------------------------------------------------
	/** 清空全部临时数组（复刻蓝图 ResetTempArrays） */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Temp")
	void MyResetTempArrays();

	// ------------------------------------------------------------------
	// 复刻蓝图函数 GetTempArray
	// 蓝图实现：40 选项 Select，按 Index 返回 TempArray0~39 中对应数组
	// ------------------------------------------------------------------
	/** 按索引返回对应的临时数组引用（复刻蓝图 GetTempArray，Index 范围 0~39）。
	 *  返回引用：蓝图下游对数组的修改（如 Array_Add）会直接写回组件内部数组，
	 *  与原版蓝图 Select 的引用传递语义一致。 */
	UFUNCTION(BlueprintPure, Category = "FluidSim|Temp")
	TArray<FName>& MyGetTempArray(int32 Index);

	// ------------------------------------------------------------------
	// 复刻蓝图 Array_Add：向指定临时数组添加一个元素
	// ------------------------------------------------------------------
	/** 向指定临时数组添加一个元素（复刻蓝图 Array_Add，ArrayIndex 范围 0~39） */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Temp")
	void MyAddToTempArray(int32 ArrayIndex, FName Item);

	// ------------------------------------------------------------------
	// 复刻蓝图 Array_Clear：清空指定临时数组
	// ------------------------------------------------------------------
	/** 清空指定临时数组（复刻蓝图 Array_Clear，ArrayIndex 范围 0~39） */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Temp")
	void MyClearTempArray(int32 ArrayIndex);

	// ------------------------------------------------------------------
	// 复刻蓝图 Array_Append：把另一个数组追加到指定临时数组末尾
	// ------------------------------------------------------------------
	/** 把另一个数组追加到指定临时数组末尾（复刻蓝图 Array_Append，ArrayIndex 范围 0~39） */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Temp")
	void MyAppendToTempArray(int32 ArrayIndex, const TArray<FName>& Items);

	// ------------------------------------------------------------------
	// 复刻蓝图函数 CompareMapLength
	// 蓝图实现（已核对完整节点数据）：
	//   MapLength = RenderTargetsMap.Length
	//   Added     = (LastIndex + 1) - FirstIndex
	//   Equal     = (Added + MapLengthTmp) == MapLength
	// ------------------------------------------------------------------
	/** 比较 Map 长度（复刻蓝图 CompareMapLength）。
	 *  输出：MapLength=当前 Map 长度；Added=(LastIndex+1)-FirstIndex；
	 *        Equal=(Added+MapLengthTmp) == MapLength */
	UFUNCTION(BlueprintPure, Category = "FluidSim|Temp")
	void MyCompareMapLength(int32 FirstIndex, int32 LastIndex, int32& MapLength, bool& Equal, int32& Added) const;

	// ------------------------------------------------------------------
	// 复刻蓝图函数 VelocityHandlerForSimArea
	// 蓝图实现（已核对完整节点数据，含默认值）：
	//   if (VeloFromSimAreaMotion != 0):
	//     WorldDir = (TraceMeshParentPos - TraceMeshParentLastPos) * 20.0
	//     LocalDir = InverseTransformDirection(TraceMeshTransform, WorldDir)
	//     Velocity = LocalDir * (VeloFromSimAreaMotion * CoEff)
	//   else: Velocity = (0,0,0)
	//   X = Velocity.X; Y = Velocity.Y * -1; Z = Velocity.Z
	//   TraceMeshTransform = MakeTransform(Location=(0,0,0), Rotation=TraceMeshComponent 旋转, Scale=(1,1,1))
	// ------------------------------------------------------------------
	/** 处理模拟区域速度（复刻蓝图 VelocityHandlerForSimArea）。
	 *  输入 CoEff（默认 -0.01）；输出分解后的 X/Y/Z 分量（Y 取反）。 */
	UFUNCTION(BlueprintPure, Category = "FluidSim|Velocity")
	void MyVelocityHandlerForSimArea(double CoEff, double& X, double& Y, double& Z) const;

	// ------------------------------------------------------------------
	// 复刻蓝图复合节点 "Enable OWNER Input"（EventGraph 内折叠图）
	// 蓝图实现（已核对完整节点数据）：
	//   if (UserInputBasedInteraction != MOUSE_SINGLE):
	//     if (Owner 是 NinjaLive 类):
	//       Owner.EnableInput(GetPlayerController(0))
	//       if (ShowMouseCursor): PC.bShowMouseCursor = true
	// ------------------------------------------------------------------
	/** 启用 Owner 的输入（复刻蓝图 "Enable OWNER Input" 复合节点）。
	 *  输入方式非鼠标时，若 Owner 是 NinjaLive Actor 则启用其输入并按需显示鼠标光标。 */
	UFUNCTION(BlueprintCallable, Category = "FluidSim|Input")
	void MyEnableOwnerInput();
};

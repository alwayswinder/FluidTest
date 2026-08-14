// MyNinjaLiveComponent.cpp — UMyNinjaLiveComponent 实现
// 已迁移：MyResetTempArrays（复刻蓝图 ResetTempArrays）

#include "MyNinjaLiveComponent.h"

UMyNinjaLiveComponent::UMyNinjaLiveComponent()
{
	// 不启用 Tick（蓝图侧若需要 Tick，可在蓝图里自行开启）。
	PrimaryComponentTick.bCanEverTick = false;
}

void UMyNinjaLiveComponent::MyResetTempArrays()
{
	// 复刻蓝图 ResetTempArrays：按顺序清空 TempArray0~TempArray39（Array_Clear）
	MyTempArray0.Empty();
	MyTempArray1.Empty();
	MyTempArray2.Empty();
	MyTempArray3.Empty();
	MyTempArray4.Empty();
	MyTempArray5.Empty();
	MyTempArray6.Empty();
	MyTempArray7.Empty();
	MyTempArray8.Empty();
	MyTempArray9.Empty();
	MyTempArray10.Empty();
	MyTempArray11.Empty();
	MyTempArray12.Empty();
	MyTempArray13.Empty();
	MyTempArray14.Empty();
	MyTempArray15.Empty();
	MyTempArray16.Empty();
	MyTempArray17.Empty();
	MyTempArray18.Empty();
	MyTempArray19.Empty();
	MyTempArray20.Empty();
	MyTempArray21.Empty();
	MyTempArray22.Empty();
	MyTempArray23.Empty();
	MyTempArray24.Empty();
	MyTempArray25.Empty();
	MyTempArray26.Empty();
	MyTempArray27.Empty();
	MyTempArray28.Empty();
	MyTempArray29.Empty();
	MyTempArray30.Empty();
	MyTempArray31.Empty();
	MyTempArray32.Empty();
	MyTempArray33.Empty();
	MyTempArray34.Empty();
	MyTempArray35.Empty();
	MyTempArray36.Empty();
	MyTempArray37.Empty();
	MyTempArray38.Empty();
	MyTempArray39.Empty();
}

TArray<FName>& UMyNinjaLiveComponent::MyGetTempArray(int32 Index)
{
	// 复刻蓝图 GetTempArray：40 选项 Select，按 Index 返回 TempArray0~39 中对应数组。
	// 返回引用：蓝图下游对数组的修改（如 Array_Add）会直接写回组件内部数组。
	// Index 越界时返回静态空数组。
	switch (Index)
	{
	case 0:  return MyTempArray0;
	case 1:  return MyTempArray1;
	case 2:  return MyTempArray2;
	case 3:  return MyTempArray3;
	case 4:  return MyTempArray4;
	case 5:  return MyTempArray5;
	case 6:  return MyTempArray6;
	case 7:  return MyTempArray7;
	case 8:  return MyTempArray8;
	case 9:  return MyTempArray9;
	case 10: return MyTempArray10;
	case 11: return MyTempArray11;
	case 12: return MyTempArray12;
	case 13: return MyTempArray13;
	case 14: return MyTempArray14;
	case 15: return MyTempArray15;
	case 16: return MyTempArray16;
	case 17: return MyTempArray17;
	case 18: return MyTempArray18;
	case 19: return MyTempArray19;
	case 20: return MyTempArray20;
	case 21: return MyTempArray21;
	case 22: return MyTempArray22;
	case 23: return MyTempArray23;
	case 24: return MyTempArray24;
	case 25: return MyTempArray25;
	case 26: return MyTempArray26;
	case 27: return MyTempArray27;
	case 28: return MyTempArray28;
	case 29: return MyTempArray29;
	case 30: return MyTempArray30;
	case 31: return MyTempArray31;
	case 32: return MyTempArray32;
	case 33: return MyTempArray33;
	case 34: return MyTempArray34;
	case 35: return MyTempArray35;
	case 36: return MyTempArray36;
	case 37: return MyTempArray37;
	case 38: return MyTempArray38;
	case 39: return MyTempArray39;
	default:
	{
		// 越界：返回静态空数组引用（不能返回临时对象的引用）
		static TArray<FName> EmptyArray;
		return EmptyArray;
	}
	}
}

void UMyNinjaLiveComponent::MyAddToTempArray(int32 ArrayIndex, FName Item)
{
	// 复刻蓝图 Array_Add：向指定临时数组添加一个元素
	TArray<FName>& ArrayRef = MyGetTempArray(ArrayIndex);
	ArrayRef.Add(Item);
}

void UMyNinjaLiveComponent::MyClearTempArray(int32 ArrayIndex)
{
	// 复刻蓝图 Array_Clear：清空指定临时数组
	TArray<FName>& ArrayRef = MyGetTempArray(ArrayIndex);
	ArrayRef.Empty();
}

void UMyNinjaLiveComponent::MyAppendToTempArray(int32 ArrayIndex, const TArray<FName>& Items)
{
	// 复刻蓝图 Array_Append：把另一个数组追加到指定临时数组末尾
	TArray<FName>& ArrayRef = MyGetTempArray(ArrayIndex);
	ArrayRef.Append(Items);
}

void UMyNinjaLiveComponent::MyCompareMapLength(int32 FirstIndex, int32 LastIndex, int32& MapLength, bool& Equal, int32& Added) const
{
	// 复刻蓝图 CompareMapLength（已核对完整节点数据，含默认值）：
	//   节点46 Add:      LastIndex + 1                     （B 为默认值 1）
	//   节点292 Subtract: (LastIndex + 1) - FirstIndex     → Added
	//   节点47 Add:      Added + MapLengthTmp
	//   节点236 Length:  MyRenderTargetsMap.Num()          → MapLength
	//   节点240 Equal:   (Added + MapLengthTmp) == MapLength → Equal
	MapLength = MyRenderTargetsMap.Num();
	Added = (LastIndex + 1) - FirstIndex;
	const int32 Tmp = Added + MyMapLengthTmp;
	Equal = (Tmp == MapLength);
}

void UMyNinjaLiveComponent::MyVelocityHandlerForSimArea(double CoEff, double& X, double& Y, double& Z) const
{
	// 复刻蓝图 VelocityHandlerForSimArea（已核对完整节点数据，含默认值）：
	//   节点0  NotEqual:   VeloFromSimAreaMotion != 0.0            → bPickA
	//   节点88 Multiply:   VeloFromSimAreaMotion * CoEff
	//   节点39 Subtract:   TraceMeshParentPos - TraceMeshParentLastPos
	//   节点8  Multiply:   (差值) * 20.0                            （B 默认 20）
	//   节点546 MakeTransform: Rotation=TraceMeshComponent 旋转, Location/Scale 默认
	//   节点545 InverseTransformDirection: 世界方向 → 局部方向
	//   节点237 Multiply:   LocalDir * (VeloFromSimAreaMotion * CoEff)
	//   节点266 SelectVector: bPickA ? 计算值 : (0,0,0)
	//   节点388 BreakVector → X/Z；Y 经节点0 Multiply(-1) 取反
	FVector Velocity = FVector::ZeroVector;

	if (MyVeloFromSimAreaMotion != 0.0)
	{
		// 世界空间位移方向（乘 20）
		const FVector WorldDir = (MyTraceMeshParentPos - MyTraceMeshParentLastPos) * 20.0;

		// 用 TraceMesh 组件的旋转构造 Transform（仅旋转，位置/缩放默认）
		const FTransform TraceMeshTransform(MyTraceMeshComponent
			? MyTraceMeshComponent->GetComponentRotation()
			: FRotator::ZeroRotator);

		// 世界方向 → TraceMesh 局部方向（InverseTransformDirection）
		const FVector LocalDir = TraceMeshTransform.InverseTransformVectorNoScale(WorldDir);

		// 乘速度系数
		Velocity = LocalDir * (MyVeloFromSimAreaMotion * CoEff);
	}

	X = Velocity.X;
	Y = Velocity.Y * -1.0;   // 蓝图 Y 分量乘 -1
	Z = Velocity.Z;
}

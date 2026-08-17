// MyNinjaLiveComponent.cpp — UMyNinjaLiveComponent 实现

#include "MyNinjaLiveComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "MyNinjaLiveActor.h"

UMyNinjaLiveComponent::UMyNinjaLiveComponent()
{
	// 不启用 Tick（蓝图侧若需要可在蓝图里自行开启）
	PrimaryComponentTick.bCanEverTick = false;
}

void UMyNinjaLiveComponent::MyResetTempArrays()
{
	// 清空 TempArray0~39
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
	// 按 Index 返回 TempArray0~39 中对应数组；越界返回静态空数组
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
		static TArray<FName> EmptyArray;
		return EmptyArray;
	}
	}
}

void UMyNinjaLiveComponent::MyAddToTempArray(int32 ArrayIndex, FName Item)
{
	MyGetTempArray(ArrayIndex).Add(Item);
}

void UMyNinjaLiveComponent::MyClearTempArray(int32 ArrayIndex)
{
	MyGetTempArray(ArrayIndex).Empty();
}

void UMyNinjaLiveComponent::MyAppendToTempArray(int32 ArrayIndex, const TArray<FName>& Items)
{
	MyGetTempArray(ArrayIndex).Append(Items);
}

void UMyNinjaLiveComponent::MyCompareMapLength(int32 FirstIndex, int32 LastIndex, int32& MapLength, bool& Equal, int32& Added) const
{
	MapLength = MyRenderTargetsMap.Num();
	Added = (LastIndex + 1) - FirstIndex;
	const int32 Tmp = Added + MyMapLengthTmp;
	Equal = (Tmp == MapLength);
}

void UMyNinjaLiveComponent::MyVelocityHandlerForSimArea(double CoEff, double& X, double& Y, double& Z) const
{
	// VeloFromSimAreaMotion 非零时：TraceMesh 前后帧位移 ×20 → 局部方向 × (速度×系数)
	FVector Velocity = FVector::ZeroVector;

	if (MyVeloFromSimAreaMotion != 0.0)
	{
		const FVector WorldDir = (MyTraceMeshParentPos - MyTraceMeshParentLastPos) * 20.0;

		const FTransform TraceMeshTransform(MyTraceMeshComponent
			? MyTraceMeshComponent->GetComponentRotation()
			: FRotator::ZeroRotator);

		const FVector LocalDir = TraceMeshTransform.InverseTransformVectorNoScale(WorldDir);

		Velocity = LocalDir * (MyVeloFromSimAreaMotion * CoEff);
	}

	X = Velocity.X;
	Y = Velocity.Y * -1.0;   // Y 取反
	Z = Velocity.Z;
}

bool UMyNinjaLiveComponent::MyCheckComponentOwner(AMyNinjaLiveActor*& AsNinjaLive) const
{
	// Owner 是否为 NinjaLive 类（用 C++ 父类判断，蓝图类继承自它）
	AsNinjaLive = nullptr;

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->IsA<AMyNinjaLiveActor>())
	{
		return false;
	}

	AsNinjaLive = Cast<AMyNinjaLiveActor>(OwnerActor);
	return true;
}

void UMyNinjaLiveComponent::MyEnableOwnerInput()
{
	// 输入方式为"无输入"时不处理
	if (MyUserInputBasedInteraction == EMyUserInput::None)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	// Owner 必须是 NinjaLive 类
	if (!OwnerActor->IsA<AMyNinjaLiveActor>())
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	OwnerActor->EnableInput(PC);

	if (MyShowMouseCursor)
	{
		PC->bShowMouseCursor = true;
	}
}

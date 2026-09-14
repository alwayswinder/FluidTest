#include "FluidTest/MyNinjaLiveComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMyNinjaLiveTempArraySlotsTest,
	"FluidTest.NinjaLive.TempArraySlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMyNinjaLiveTempArraySlotsTest::RunTest(const FString& Parameters)
{
	UMyNinjaLiveComponent* Component = NewObject<UMyNinjaLiveComponent>();
	TestNotNull(TEXT("应能创建 NinjaLive 组件"), Component);
	if (!Component)
	{
		return false;
	}

	Component->MyResetTempArraySlots();
	TestEqual(TEXT("临时数组槽位数量固定为 40"), Component->MyListOfAvailableTempArrays.Num(), 40);

	TSet<int32> AcquiredSlots;
	for (int32 Index = 0; Index < 40; ++Index)
	{
		const int32 Slot = Component->MyAcquireTempArraySlot();
		TestTrue(TEXT("容量未耗尽时必须成功申请槽位"), Slot != INDEX_NONE);
		TestFalse(TEXT("同一轮申请不能返回重复槽位"), AcquiredSlots.Contains(Slot));
		AcquiredSlots.Add(Slot);
	}

	TestEqual(TEXT("全部槽位占用后申请应失败"), Component->MyAcquireTempArraySlot(), INDEX_NONE);

	constexpr int32 ReleasedSlot = 17;
	Component->MyAddToTempArray(ReleasedSlot, TEXT("TestBone"));
	Component->MyReleaseTempArraySlot(ReleasedSlot);
	TestTrue(TEXT("释放后槽位应恢复为可用"), Component->MyListOfAvailableTempArrays[ReleasedSlot]);
	TestTrue(TEXT("释放后槽位内容应被清空"), Component->MyGetTempArray(ReleasedSlot).IsEmpty());
	TestEqual(TEXT("再次申请应复用已释放槽位"), Component->MyAcquireTempArraySlot(), ReleasedSlot);

	return true;
}

#endif

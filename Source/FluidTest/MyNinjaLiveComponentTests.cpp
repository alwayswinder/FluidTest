#include "FluidTest/MyNinjaLiveComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
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
	UStaticMeshComponent* DummyMesh = NewObject<UStaticMeshComponent>();
	Component->MySkeletalMeshTempArrayPairs.Add(ReleasedSlot, DummyMesh);
	Component->MyReleaseTempArraySlot(ReleasedSlot);
	TestTrue(TEXT("释放后槽位应恢复为可用"), Component->MyListOfAvailableTempArrays[ReleasedSlot]);
	TestTrue(TEXT("释放后槽位内容应被清空"), Component->MyGetTempArray(ReleasedSlot).IsEmpty());
	TestFalse(TEXT("释放后必须删除对应组件映射"), Component->MySkeletalMeshTempArrayPairs.Contains(ReleasedSlot));
	TestEqual(TEXT("再次申请应复用已释放槽位"), Component->MyAcquireTempArraySlot(), ReleasedSlot);

	Component->MyAddToTempArray(ReleasedSlot, TEXT("ReplayBone"));
	Component->MySkeletalMeshTempArrayPairs.Add(ReleasedSlot, DummyMesh);
	Component->MyResetTempArraySlots();
	TestTrue(TEXT("完整重置后组件映射必须为空"), Component->MySkeletalMeshTempArrayPairs.IsEmpty());
	for (int32 Index = 0; Index < 40; ++Index)
	{
		TestTrue(TEXT("完整重置后全部槽位都应可用"), Component->MyListOfAvailableTempArrays[Index]);
		TestTrue(TEXT("完整重置后全部槽位内容都应为空"), Component->MyGetTempArray(Index).IsEmpty());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMyNinjaLiveCachedMaterialParametersTest,
	"FluidTest.NinjaLive.CachedMaterialParameters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMyNinjaLiveCachedMaterialParametersTest::RunTest(const FString& Parameters)
{
	UMyNinjaLiveComponent* Component = NewObject<UMyNinjaLiveComponent>();
	TestNotNull(TEXT("应能创建 NinjaLive 组件"), Component);
	if (!Component)
	{
		return false;
	}

	UMaterialInterface* CompositeMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/FluidNinjaLive/Core/FluidSim/MI_Float/MI_CompositeAndGradient.MI_CompositeAndGradient"));
	UMaterialInterface* DivergenceMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/FluidNinjaLive/Core/FluidSim/MI_Float/MI_Divergence.MI_Divergence"));
	TestNotNull(TEXT("应能加载 Composite 材质"), CompositeMaterial);
	TestNotNull(TEXT("应能加载 Divergence 材质"), DivergenceMaterial);
	if (!CompositeMaterial || !DivergenceMaterial)
	{
		return false;
	}

	Component->MyMICompositeAndGradient = UMaterialInstanceDynamic::Create(CompositeMaterial, Component);
	Component->MyMIDivergence = UMaterialInstanceDynamic::Create(DivergenceMaterial, Component);
	Component->MyFlowFeedback = 0.25;
	Component->MyDivergence = 0.75;
	Component->MyBrushPuncture = 0.5;
	Component->MySetAdditionalFluidsimParams();

	TestEqual(TEXT("首次写入应更新 FlowFeedback"),
		Component->MyMICompositeAndGradient->K2_GetScalarParameterValue(TEXT("FlowFeedback")), 0.25f);
	TestEqual(TEXT("首次写入应更新 Divergence"),
		Component->MyMIDivergence->K2_GetScalarParameterValue(TEXT("Divergence")), 0.75f);
	TestEqual(TEXT("首次写入应更新 BrushPuncture"),
		Component->MyMIDivergence->K2_GetScalarParameterValue(TEXT("BrushPuncture")), 0.5f);

	Component->MyFlowFeedback = 0.625;
	Component->MyDivergence = 0.375;
	Component->MyBrushPuncture = 0.125;
	Component->MySetAdditionalFluidsimParams();

	TestEqual(TEXT("缓存索引应继续更新 FlowFeedback"),
		Component->MyMICompositeAndGradient->K2_GetScalarParameterValue(TEXT("FlowFeedback")), 0.625f);
	TestEqual(TEXT("缓存索引应继续更新 Divergence"),
		Component->MyMIDivergence->K2_GetScalarParameterValue(TEXT("Divergence")), 0.375f);
	TestEqual(TEXT("缓存索引应继续更新 BrushPuncture"),
		Component->MyMIDivergence->K2_GetScalarParameterValue(TEXT("BrushPuncture")), 0.125f);

	return true;
}

#endif

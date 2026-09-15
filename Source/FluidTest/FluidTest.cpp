

#include "FluidTest.h"
#include "FluidTest/MyNinjaFluidRenderPipeline.h"
#include "Modules/ModuleManager.h"

class FFluidTestModule final : public FDefaultGameModuleImpl
{
public:
	virtual void ShutdownModule() override
	{
		FMyNinjaFluidRenderPipeline::MyShutdown();
		FDefaultGameModuleImpl::ShutdownModule();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FFluidTestModule, FluidTest, "FluidTest");



using UnrealBuildTool;

public class FluidTest : ModuleRules
{
	public FluidTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "MediaAssets", "Niagara", "AssetRegistry" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });








	}
}

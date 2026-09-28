using UnrealBuildTool;

public class FadedNav : ModuleRules
{
	public FadedNav(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AIModule", "Niagara", "RenderCore", "SlateCore"
		});
	}
}

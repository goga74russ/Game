using UnrealBuildTool;

public class FadedNavTarget : TargetRules
{
	public FadedNavTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("FadedNav");
	}
}

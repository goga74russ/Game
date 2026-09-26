using UnrealBuildTool;

public class FadedNavEditorTarget : TargetRules
{
	public FadedNavEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("FadedNav");
	}
}

using UnrealBuildTool;
using System.Collections.Generic;

public class WebTestingEditorTarget : TargetRules
{
	public WebTestingEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V8;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_9;
		ExtraModuleNames.Add("WebTesting");
	}
}

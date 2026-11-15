using UnrealBuildTool;

public class ClaudeTestHelper : ModuleRules
{
	public ClaudeTestHelper(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"InputCore", "Slate", "SlateCore", "EnhancedInput", "GameplayTags", "HTTP", "Json", "JsonUtilities"
		});
	}
}

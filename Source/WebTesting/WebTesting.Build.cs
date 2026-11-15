using UnrealBuildTool;

public class WebTesting : ModuleRules
{
	public WebTesting(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "Slate", "SlateCore" });

		// The ground paint is read from the landscape by a level tool, in the editor only.
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("Landscape");
		}
	}
}

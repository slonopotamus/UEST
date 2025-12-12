using UnrealBuildTool;

public class UEST : ModuleRules
{
	public UEST(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"EngineSettings",
			"IrisCore",
			"TypedElementFramework",
		});
	}
}

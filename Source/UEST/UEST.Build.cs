using UnrealBuildTool;

public class UEST : ModuleRules
{
	public UEST(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Boost",
				"Core",
				"CoreUObject",
				"Engine",
			}
		);
	}
}

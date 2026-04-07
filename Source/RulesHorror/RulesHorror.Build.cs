// Copyright (c) 2026 장윤제. All rights reserved.

using UnrealBuildTool;

public class RulesHorror : ModuleRules
{
	public RulesHorror(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"SlateCore",
            "CommonLibrary",
            "CustomUI",
            "DeveloperSettings",
			"ItemCore",
			"InteractionSystem",
			"SaveGame",
        });

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"RulesHorror",
		});
	}
}

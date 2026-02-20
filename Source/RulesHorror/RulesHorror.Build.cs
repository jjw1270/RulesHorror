// Copyright Epic Games, Inc. All Rights Reserved.

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
            "UnrealEd"
        });

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"RulesHorror",
		});
	}
}

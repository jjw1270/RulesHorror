// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RulesHorrorEditor : ModuleRules
{
	public RulesHorrorEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
        });

		PrivateDependencyModuleNames.AddRange(new string[] {
            "RulesHorror",
            "Core",
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "BlueprintGraph",
            "KismetCompiler",
            "GraphEditor",
            "UnrealEd",
            "PropertyEditor",
            "CommonLibrary",
            "InputCore",
        });

		PublicIncludePaths.AddRange(new string[] {
			
        });
	}
}

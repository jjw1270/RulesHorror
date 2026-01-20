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
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"RulesHorror",
			"RulesHorror/Variant_Horror",
			"RulesHorror/Variant_Horror/UI",
			"RulesHorror/Variant_Shooter",
			"RulesHorror/Variant_Shooter/AI",
			"RulesHorror/Variant_Shooter/UI",
			"RulesHorror/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}

// Copyright (c) 2026 장윤제. All rights reserved.

using UnrealBuildTool;

public class InteractionSystemEditor : ModuleRules
{
	public InteractionSystemEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] 
			{ 
				"Core",
				"CoreUObject",
				"Engine"
			});
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "UnrealEd",
                "ComponentVisualizers",
                "InteractionSystem",
				"CommonLibrary"
            });
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				
			});
	}
}

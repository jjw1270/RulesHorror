// Copyright (c) 2026 장윤제. All rights reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class RulesHorrorTarget : TargetRules
{
	public RulesHorrorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("RulesHorror");
	}
}

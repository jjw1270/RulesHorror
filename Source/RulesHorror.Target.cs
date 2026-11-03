// Copyright (c) 2026 장윤제. All rights reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class RulesHorrorTarget : TargetRules
{
	public RulesHorrorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("RulesHorror");
	}
}

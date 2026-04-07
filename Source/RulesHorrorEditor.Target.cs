// Copyright (c) 2026 장윤제. All rights reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class RulesHorrorEditorTarget : TargetRules
{
	public RulesHorrorEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("RulesHorror");
    }
}

// Copyright (c) 2026 장윤제. All rights reserved.


#include "SaveGame/RulesHorrorProgressDefines.h"

namespace RulesHorrorProgress
{
	bool IsValidFlag(ERulesHorrorProgressFlag _flag)
	{
		return _flag > ERulesHorrorProgressFlag::None && _flag < ERulesHorrorProgressFlag::Max;
	}

	FName ToSaveKey(ERulesHorrorProgressFlag _flag)
	{
		if (IsValidFlag(_flag) == false)
			return NAME_None;

		return FName(StaticEnum<ERulesHorrorProgressFlag>()->GetNameStringByValue(static_cast<int64>(_flag)));
	}

	ERulesHorrorProgressFlag GetUnlockedFlag(ERulesHorrorScenario _scenario)
	{
		switch (_scenario)
		{
		case ERulesHorrorScenario::Scenario_01_PCBang:		return ERulesHorrorProgressFlag::Scenario_01_Unlocked;
		case ERulesHorrorScenario::Scenario_02_Mugunghwa:	return ERulesHorrorProgressFlag::Scenario_02_Unlocked;
		case ERulesHorrorScenario::Scenario_03_Forest:		return ERulesHorrorProgressFlag::Scenario_03_Unlocked;
		default:											return ERulesHorrorProgressFlag::None;
		}
	}

	ERulesHorrorProgressFlag GetClearFlag(ERulesHorrorScenario _scenario)
	{
		switch (_scenario)
		{
		case ERulesHorrorScenario::Scenario_01_PCBang:		return ERulesHorrorProgressFlag::Scenario_01_Clear;
		case ERulesHorrorScenario::Scenario_02_Mugunghwa:	return ERulesHorrorProgressFlag::Scenario_02_Clear;
		case ERulesHorrorScenario::Scenario_03_Forest:		return ERulesHorrorProgressFlag::Scenario_03_Clear;
		default:											return ERulesHorrorProgressFlag::None;
		}
	}

	ERulesHorrorProgressFlag GetBrandFlag(ERulesHorrorScenario _scenario)
	{
		switch (_scenario)
		{
		case ERulesHorrorScenario::Scenario_01_PCBang:		return ERulesHorrorProgressFlag::Brand_01_Oppression;
		case ERulesHorrorScenario::Scenario_02_Mugunghwa:	return ERulesHorrorProgressFlag::Brand_02_Bystanding;
		case ERulesHorrorScenario::Scenario_03_Forest:		return ERulesHorrorProgressFlag::Brand_03_Exploitation;
		default:											return ERulesHorrorProgressFlag::None;
		}
	}

	int32 ParseChapterNumber(const FStorySceneID& _scene_id)
	{
		if (_scene_id.IsValid() == false)
			return INDEX_NONE;

		const FString scene_id = _scene_id.Get().ToString();
		if (scene_id.Len() < 5 || scene_id.StartsWith(TEXT("CH"), ESearchCase::CaseSensitive) == false || scene_id[4] != TEXT('_'))
			return INDEX_NONE;

		if (FChar::IsDigit(scene_id[2]) == false || FChar::IsDigit(scene_id[3]) == false)
			return INDEX_NONE;

		return (scene_id[2] - TEXT('0')) * 10 + (scene_id[3] - TEXT('0'));
	}
}

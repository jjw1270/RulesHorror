// Copyright (c) 2026 장윤제. All rights reserved.


#include "SaveGame/RulesHorrorSaveGame.h"

void URulesHorrorSaveGame::ClearData()
{
	Super::ClearData();

	if (CanModify() == false)
		return;

	_StoryFlowRef.Reset();
}

bool URulesHorrorSaveGame::IsEmpty() const
{
	return Super::IsEmpty() && !_StoryFlowRef.IsValid();
}

void URulesHorrorSaveGame::SaveStoryFlowRef(const FStoryFlowRef& _value)
{
	if (CanModify() == false)
		return;

	_StoryFlowRef = _value;
}

const FStoryFlowRef& URulesHorrorSaveGame::GetStoryFlowRef() const
{
	return _StoryFlowRef;
}

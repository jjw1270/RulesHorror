// Copyright (c) 2026 장윤제. All rights reserved.


#include "SaveGame/RulesHorrorSaveGame.h"

const FName URulesHorrorSaveGame::CurrentChapterKey(TEXT("CurrentChapter"));

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

	const int32 chapter = RulesHorrorProgress::ParseChapterNumber(_value.SceneID);
	if (chapter != INDEX_NONE)
	{
		SaveIntData(CurrentChapterKey, chapter);
	}
}

const FStoryFlowRef& URulesHorrorSaveGame::GetStoryFlowRef() const
{
	return _StoryFlowRef;
}

bool URulesHorrorSaveGame::SaveProgressFlag(ERulesHorrorProgressFlag _flag, bool _value)
{
	const FName key = RulesHorrorProgress::ToSaveKey(_flag);
	if (key.IsNone())
		return false;

	return SaveBoolData(key, _value);
}

bool URulesHorrorSaveGame::GetProgressFlag(ERulesHorrorProgressFlag _flag) const
{
	bool value = false;
	FindSavedBoolData(RulesHorrorProgress::ToSaveKey(_flag), value);
	return value;
}

int32 URulesHorrorSaveGame::GetCurrentChapter() const
{
	int32 chapter = 0;
	FindSavedIntData(CurrentChapterKey, chapter);
	return chapter;
}

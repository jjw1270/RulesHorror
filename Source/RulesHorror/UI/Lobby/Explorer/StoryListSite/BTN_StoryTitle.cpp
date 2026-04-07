// Copyright (c) 2026 장윤제. All rights reserved.


#include "BTN_StoryTitle.h"
#include "Item/RulesHorrorItemHelper.h"

void UBTN_StoryTitle::SetStoryID(FItemID_Story _story_id)
{
	if (_StoryID == _story_id)
		return;

	_StoryID = _story_id;

	const auto& story_data = URulesHorrorItemHelper::GetStoryItemRow(_StoryID);

	SetTitle(story_data.GetDisplayName());
}

// Copyright Epic Games, Inc. All Rights Reserved.


#include "RulesHorrorItemHelper.h"

const FStoryTableRow& URulesHorrorItemHelper::GetStoryItemRow(FItemID_Story _story_id)
{
	auto item_row_ptr = FindItemRow<FStoryTableRow>(_story_id);

	if (IsValid(item_row_ptr))
	{
		return *item_row_ptr;
	}

	static const FStoryTableRow  s_null;
	return s_null;
}

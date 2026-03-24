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

TArray<FStoryTableRow> URulesHorrorItemHelper::GetAllStoryItemRows()
{
	const auto item_ptr_rows = GetAllItemRowsByType<FStoryTableRow>(EItemType::Story);

	TArray<FStoryTableRow> item_rows;
	item_rows.Reserve(item_ptr_rows.Num());

	for (auto item_ptr : item_ptr_rows)
	{
		if (IsValid(item_ptr))
		{
			item_rows.Add(*item_ptr);
		}
	}

	return item_rows;
}

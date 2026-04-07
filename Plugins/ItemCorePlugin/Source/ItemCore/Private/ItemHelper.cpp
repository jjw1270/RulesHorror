// Copyright (c) 2026 장윤제. All rights reserved.

#include "ItemHelper.h"


bool UItemHelper::FindItemRow(FItemID _item_id, FItemTableRow& _out_item_row)
{
	auto item_row_ptr = FindItemRow<FItemTableRow>(_item_id);

	if (IsInvalid(item_row_ptr))
		return false;

	_out_item_row = *item_row_ptr;
	return true;
}

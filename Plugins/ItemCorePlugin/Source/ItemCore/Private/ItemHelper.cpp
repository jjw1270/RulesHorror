// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemHelper.h"


const FItemTableRow& UItemHelper::GetItemRow(FItemID _item_id)
{
	auto item_row_ptr = FindItemRow<FItemTableRow>(_item_id);

	if (IsValid(item_row_ptr))
	{
		return *item_row_ptr;
	}

	static const FItemTableRow  s_null;
	return s_null;
}

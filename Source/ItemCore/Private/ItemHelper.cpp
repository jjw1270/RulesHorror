// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemHelper.h"


bool UItemHelper::FindItemRow(FItemID _item_id, FItemTableRow& _out_item_row)
{
	auto item_row_ptr = FindItemRow<FItemTableRow>(_item_id);

	if (IsInvalid(item_row_ptr))
		return false;

	_out_item_row = *item_row_ptr;
	return true;
}

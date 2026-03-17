// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemTableRow.h"
#include "CommonUtils.h"
#if WITH_EDITOR
#include "Engine/Engine.h"
#include "ItemRegistrySubsystem.h"
#endif


#if WITH_EDITOR
const FName FItemTableRow::ItemTableLog(TEXT("ItemTable"));

void FItemTableRow::OnDataTableChanged(const UDataTable* _in_data_table, const FName _in_row_name)
{
	Super::OnDataTableChanged(_in_data_table, _in_row_name);

	HandleItemIDChanged(_in_data_table);
}

void FItemTableRow::OnPostDataImport(const UDataTable* _in_data_table, const FName _in_row_name, TArray<FString>& _out_collected_import_problems)
{
	Super::OnPostDataImport(_in_data_table, _in_row_name, _out_collected_import_problems);

	CachedItemID = ItemID;
}

EDataValidationResult FItemTableRow::IsDataValid(FDataValidationContext& _context) const
{
	if (ItemID.IsValid() == false)
	{
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}

void FItemTableRow::HandleItemIDChanged(const UDataTable* _in_data_table)
{
	if (IsInvalid(_in_data_table))
		return;

	if (IsValidEnumValue(TableItemType, true))
	{
		if (ItemID.GetType() != TableItemType)
		{
			EDITOR_MESSAGE_ERROR(ItemTableLog, TEXT("ItemType은 %s 고정입니다!"), *TEnumToString(TableItemType, true));
			return;
		}
	}

	if (CachedItemID.IsSet())
	{
		if (CachedItemID.GetValue() == ItemID)
			return;

		CachedItemID = ItemID;
	}
	else
	{
		CachedItemID = ItemID;
		return;
	}

	if (ItemID.IsValid() == false && ItemID != FItemID::Zero)
		return;

	if (IsValid(GEngine))
	{
		auto item_registry = GEngine->GetEngineSubsystem<UItemRegistrySubsystem>();
		if (IsValid(item_registry))
		{
			item_registry->RefreshItemTable(_in_data_table);
		}
	}
}
#endif
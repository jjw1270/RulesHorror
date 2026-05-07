// Copyright (c) 2026 장윤제. All rights reserved.

#include "ItemRegistrySubsystem.h"
#include "ItemCore.h"
#include "ItemDeveloperSettings.h"
#include "ItemRegistryDataAsset.h"
#include "CommonUtils.h"

void UItemRegistrySubsystem::Initialize(FSubsystemCollectionBase& _collection)
{
	Super::Initialize(_collection);

#if WITH_EDITOR
	// 에디터에서만 이 시점에 Refresh, 실제 빌드에선 UItemRegistryValidationSubsystem에서 호출함.
	RefreshRegistry();
#endif
}

void UItemRegistrySubsystem::Deinitialize()
{
	ClearItemIndex();
	ClearRegisteredItemTables();

	Super::Deinitialize();
}

bool UItemRegistrySubsystem::RegisterItemTablesFromDataAsset()
{
	const auto settings = GetDefault<UItemDeveloperSettings>();
	if (IsInvalid(settings))
	{
		EDITOR_MESSAGE_ERROR(ItemRegistryLog, TEXT("ItemDeveloperSettings is null."));
		return false;
	}

	if (settings->_ItemRegistryDataAsset.IsNull())
	{
		EDITOR_MESSAGE_ERROR(ItemRegistryLog, TEXT("ItemRegistryDataAsset is not set."));
		return false;
	}

	const UItemRegistryDataAsset* registry_asset = settings->_ItemRegistryDataAsset.LoadSynchronous();
	if (IsInvalid(registry_asset))
	{
		EDITOR_MESSAGE_ERROR(ItemRegistryLog, TEXT("Failed to load ItemRegistryDataAsset '%s'."), *settings->_ItemRegistryDataAsset.ToString());
		return false;
	}

	bool is_success = true;
	for (const FItemTableReference& item_table_reference : registry_asset->_ItemTables)
	{
		if (RegisterItemTable(item_table_reference.GetDataTable()) == false)
		{
			is_success = false;
		}
	}

	return is_success;
}

bool UItemRegistrySubsystem::IsSupportedItemTable(const UDataTable* _item_table) const
{
	if (IsInvalid(_item_table))
		return false;

	const auto row_struct = _item_table->GetRowStruct();
	if (IsInvalid(row_struct))
		return false;

	return row_struct->IsChildOf(FItemTableRow::StaticStruct());
}

bool UItemRegistrySubsystem::RegisterItemTable(const UDataTable* _item_table)
{
	if (IsInvalid(_item_table))
	{
		EDITOR_MESSAGE_ERROR(ItemRegistryLog, TEXT("Table is null."));
		return false;
	}

	if (!IsSupportedItemTable(_item_table))
	{
		EDITOR_MESSAGE_ERROR(ItemRegistryLog, TEXT("Table '%s' has unsupported RowStruct."), *_item_table->GetName());
		return false;
	}

	if (_RegisteredItemTables.Contains(_item_table))
	{
		return true;
	}

	_RegisteredItemTables.Add(_item_table);
	return true;
}

void UItemRegistrySubsystem::ClearRegisteredItemTables()
{
	_RegisteredItemTables.Reset();
}

bool UItemRegistrySubsystem::BuildItemIndex()
{
	bool is_success = true;

	for (auto item_table : _RegisteredItemTables)
	{
		if (IndexItemTable(item_table) == false)
		{
			is_success = false;
		}
	}

	return is_success;
}

bool UItemRegistrySubsystem::IndexItemTable(const UDataTable* _item_table)
{
	if (IsInvalid(_item_table))
	{
		EDITOR_MESSAGE_ERROR(ItemRegistryLog, TEXT("Table is null."));
		return false;
	}

	if (IsSupportedItemTable(_item_table) == false)
	{
		EDITOR_MESSAGE_ERROR(ItemRegistryLog, TEXT("Table '%s' has unsupported RowStruct."), *_item_table->GetName());
		return false;
	}

	const auto row_struct = _item_table->GetRowStruct();
	check(row_struct != nullptr);

	bool is_success = true;

	for (const auto& row_pair : _item_table->GetRowMap())
	{
		const FName row_name = row_pair.Key;
		const uint8* row_data = row_pair.Value;

		if (IsInvalid(row_data))
		{
			EDITOR_MESSAGE_ERROR(ItemRegistryLog, TEXT("Table '%s' Row '%s' row_data is null."), *_item_table->GetName(), *row_name.ToString());
			is_success = false;
			continue;
		}

		const auto item_row = reinterpret_cast<const FItemTableRow*>(row_data);
		const FItemID& item_id = item_row->ItemID;

		FItemIDValidationResult item_id_validate = item_id.Validate();

		if (item_id_validate.IsValid())
		{
			if (Contains(item_id))
			{
				const FItemRowReference* existing_reference = Find(item_id);

				EDITOR_MESSAGE_ERROR(ItemRegistryLog, TEXT("중복 ItemID : %d [Table=%s, Row=%s],[Table=%s, Row=%s]"),
					(int32)item_id, *_item_table->GetName(), *row_name.ToString(),
					(existing_reference && existing_reference->DataTable) ? *existing_reference->DataTable->GetName() : TEXT("None"), existing_reference ? *existing_reference->RowName.ToString() : TEXT("None"));

				is_success = false;
				continue;
			}
		}
		else
		{
			EDITOR_MESSAGE_ERROR(ItemRegistryLog, TEXT("[Table=%s, Row=%s] Invalid ItemID : %d, %s"), 
				*_item_table->GetName(), *row_name.ToString(),
				(int32)item_id, *item_id_validate.Reason);
			
			is_success = false;
			continue;
		}

		FItemRowReference row_reference;
		row_reference.DataTable = _item_table;
		row_reference.RowName = row_name;
		row_reference.RowStruct = row_struct;

		_ItemTypeIndexMap.FindOrAdd(item_id.GetType()).ItemIDToRow.Add(item_id, MoveTemp(row_reference));
	}

	return is_success;
}

void UItemRegistrySubsystem::ClearItemIndex()
{
	_ItemTypeIndexMap.Reset();
}

const FItemRowReference* UItemRegistrySubsystem::Find(const FItemID& _item_id) const
{
	auto item_type_index_ptr = _ItemTypeIndexMap.Find(_item_id.GetType());
	if (IsValid(item_type_index_ptr))
	{
		return item_type_index_ptr->ItemIDToRow.Find(_item_id);
	}

	return nullptr;
}

bool UItemRegistrySubsystem::RefreshRegistry()
{
	EDITOR_MESSAGE_CLEAR(ItemRegistryLog);
	EDITOR_MESSAGE_LOG(ItemRegistryLog, TEXT("Refresh Registry..."));

	ClearRegisteredItemTables();
	ClearItemIndex();

	const bool is_register_success = RegisterItemTablesFromDataAsset();
	const bool is_build_success = BuildItemIndex();

	EDITOR_MESSAGE_LOG(ItemRegistryLog, TEXT("Register=%s, BuildIndex=%s, RegisteredTables=%d, IndexedItems=%d"),
		is_register_success ? TEXT("Success") : TEXT("Failed"),
		is_build_success ? TEXT("Success") : TEXT("Failed"),
		_RegisteredItemTables.Num(), GetItemCount());

	const bool is_success = is_register_success && is_build_success;

	if (is_success)
	{
		EDITOR_NOTIFY_LOG(TEXT("Refresh Item Registry Success!"));
	}
	else
	{
		EDITOR_NOTIFY_ERROR(TEXT("Refresh Item Registry Failed!"));
	}

	return is_success;
}

bool UItemRegistrySubsystem::RefreshItemTable(const UDataTable* _item_table)
{
	if (IsInvalid(_item_table))
	{
		EDITOR_MESSAGE_ERROR(ItemRegistryLog, TEXT("Failed : Table is null."));
		return false;
	}

	EDITOR_MESSAGE_LOG(ItemRegistryLog, TEXT("Refresh Item Table Requested : %s"), *_item_table->GetName());

	return RefreshRegistry();
}

bool UItemRegistrySubsystem::Contains(const FItemID& _item_id) const
{
	auto item_type_index_ptr = _ItemTypeIndexMap.Find(_item_id.GetType());
	if (IsValid(item_type_index_ptr))
	{
		return item_type_index_ptr->ItemIDToRow.Contains(_item_id);
	}

	return false;
}

int32 UItemRegistrySubsystem::GetItemCount() const
{
	int32 count = 0;

	for (const auto& item_type_pair : _ItemTypeIndexMap)
	{
		count += item_type_pair.Value.ItemIDToRow.Num();
	}

	return count;
}

int32 UItemRegistrySubsystem::GetTypeItemCount(EItemType _item_type) const
{
	auto item_type_index_ptr = _ItemTypeIndexMap.Find(_item_type);
	if (IsValid(item_type_index_ptr))
	{
		return item_type_index_ptr->ItemIDToRow.Num();
	}

	return 0;
}

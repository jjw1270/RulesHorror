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
	_LastRefreshSucceeded = false;
	_HasSuccessfulRegistry = false;

	Super::Deinitialize();
}

bool UItemRegistrySubsystem::RegisterItemTablesFromDataAsset(TArray<const UDataTable*>& _out_registered_item_tables) const
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
		if (RegisterItemTable(item_table_reference.GetDataTable(), _out_registered_item_tables) == false)
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

bool UItemRegistrySubsystem::RegisterItemTable(const UDataTable* _item_table, TArray<const UDataTable*>& _out_registered_item_tables) const
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

	if (_out_registered_item_tables.Contains(_item_table))
	{
		return true;
	}

	_out_registered_item_tables.Add(_item_table);
	return true;
}

void UItemRegistrySubsystem::ClearRegisteredItemTables()
{
	_RegisteredItemTables.Reset();
}

bool UItemRegistrySubsystem::BuildItemIndex(const TArray<const UDataTable*>& _registered_item_tables, TMap<EItemType, FItemTypeIndex>& _out_item_type_index_map) const
{
	bool is_success = true;

	for (auto item_table : _registered_item_tables)
	{
		if (IndexItemTable(item_table, _out_item_type_index_map) == false)
		{
			is_success = false;
		}
	}

	return is_success;
}

bool UItemRegistrySubsystem::IndexItemTable(const UDataTable* _item_table, TMap<EItemType, FItemTypeIndex>& _out_item_type_index_map) const
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
			if (Contains(item_id, _out_item_type_index_map))
			{
				const FItemRowReference* existing_reference = Find(item_id, _out_item_type_index_map);

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

		_out_item_type_index_map.FindOrAdd(item_id.GetType()).ItemIDToRow.Add(item_id, MoveTemp(row_reference));
	}

	return is_success;
}

void UItemRegistrySubsystem::ClearItemIndex()
{
	_ItemTypeIndexMap.Reset();
}

const FItemRowReference* UItemRegistrySubsystem::Find(const FItemID& _item_id) const
{
	return Find(_item_id, _ItemTypeIndexMap);
}

const FItemRowReference* UItemRegistrySubsystem::Find(const FItemID& _item_id, const TMap<EItemType, FItemTypeIndex>& _item_type_index_map) const
{
	auto item_type_index_ptr = _item_type_index_map.Find(_item_id.GetType());
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

	TArray<const UDataTable*> new_registered_item_tables;
	TMap<EItemType, FItemTypeIndex> new_item_type_index_map;

	const bool is_register_success = RegisterItemTablesFromDataAsset(new_registered_item_tables);
	const bool is_build_success = BuildItemIndex(new_registered_item_tables, new_item_type_index_map);
	const int32 new_item_count = GetItemCount(new_item_type_index_map);

	EDITOR_MESSAGE_LOG(ItemRegistryLog, TEXT("Register=%s, BuildIndex=%s, CandidateTables=%d, CandidateItems=%d, ActiveItems=%d"),
		is_register_success ? TEXT("Success") : TEXT("Failed"),
		is_build_success ? TEXT("Success") : TEXT("Failed"),
		new_registered_item_tables.Num(), new_item_count, GetItemCount());

	const bool is_success = is_register_success && is_build_success;

	if (is_success)
	{
		_RegisteredItemTables = MoveTemp(new_registered_item_tables);
		_ItemTypeIndexMap = MoveTemp(new_item_type_index_map);
		_LastRefreshSucceeded = true;
		_HasSuccessfulRegistry = true;

		EDITOR_MESSAGE_LOG(ItemRegistryLog, TEXT("Applied Item Registry. RegisteredTables=%d, IndexedItems=%d"),
			_RegisteredItemTables.Num(), GetItemCount());
		EDITOR_NOTIFY_LOG(TEXT("Refresh Item Registry Success!"));
	}
	else
	{
		_LastRefreshSucceeded = false;

		if (_HasSuccessfulRegistry)
		{
			EDITOR_MESSAGE_WARNING(ItemRegistryLog, TEXT("Refresh failed. Keeping previous successful registry. ActiveTables=%d, ActiveItems=%d"),
				_RegisteredItemTables.Num(), GetItemCount());
		}
		else
		{
			EDITOR_MESSAGE_WARNING(ItemRegistryLog, TEXT("Refresh failed. No successful registry is available. Active registry is empty."));
		}

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
	return Contains(_item_id, _ItemTypeIndexMap);
}

bool UItemRegistrySubsystem::Contains(const FItemID& _item_id, const TMap<EItemType, FItemTypeIndex>& _item_type_index_map) const
{
	auto item_type_index_ptr = _item_type_index_map.Find(_item_id.GetType());
	if (IsValid(item_type_index_ptr))
	{
		return item_type_index_ptr->ItemIDToRow.Contains(_item_id);
	}

	return false;
}

int32 UItemRegistrySubsystem::GetItemCount() const
{
	return GetItemCount(_ItemTypeIndexMap);
}

int32 UItemRegistrySubsystem::GetItemCount(const TMap<EItemType, FItemTypeIndex>& _item_type_index_map) const
{
	int32 count = 0;

	for (const auto& item_type_pair : _item_type_index_map)
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

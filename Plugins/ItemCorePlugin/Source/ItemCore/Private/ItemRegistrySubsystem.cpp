// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemRegistrySubsystem.h"
#include "ItemCore.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Modules/ModuleManager.h"
#include "ItemDeveloperSettings.h"
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

bool UItemRegistrySubsystem::AutoRegisterItemTables()
{
	FAssetRegistryModule& asset_registry_module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& asset_registry = asset_registry_module.Get();

	const TArray<FString> search_paths = GetTableSearchPaths();

	if (search_paths.IsEmpty())
	{
		EDITOR_MESSAGE_WARNING(ItemRegistryLog, TEXT("No search paths."));
		return false;
	}

	asset_registry.ScanPathsSynchronous(search_paths, false, false);

	FARFilter filter;
	filter.bRecursivePaths = true;
	filter.bRecursiveClasses = false;
	filter.ClassPaths.Add(UDataTable::StaticClass()->GetClassPathName());

	for (const auto& search_path : search_paths)
	{
		filter.PackagePaths.Add(*search_path);
	}

	TArray<FAssetData> asset_data_list;
	asset_registry.GetAssets(filter, asset_data_list);

	bool is_success = true;
	for (const auto& asset_data : asset_data_list)
	{
		UDataTable* item_table = Cast<UDataTable>(asset_data.GetAsset());
		if (IsInvalid(item_table))
		{
			EDITOR_MESSAGE_WARNING(ItemRegistryLog, TEXT("AutoRegisterItemTables - Failed to load asset '%s' as UDataTable."), *asset_data.GetObjectPathString());
			is_success = false;
			continue;
		}

		if (RegisterItemTable(item_table) == false)
		{
			EDITOR_MESSAGE_WARNING(ItemRegistryLog, TEXT("AutoRegisterItemTables - Skipped '%s'."), *item_table->GetName());
		}
	}

	return is_success;
}

TArray<FString> UItemRegistrySubsystem::GetTableSearchPaths() const
{
	TArray<FString> paths;

	const auto settings = GetDefault<UItemDeveloperSettings>();

	if (IsValid(settings))
	{
		for (const FDirectoryPath& path : settings->_ItemTableSearchPaths)
		{
			if (path.Path.IsEmpty())
				continue;

			paths.AddUnique(path.Path);
		}
	}

	return paths;
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
		_TableToItemIDs.FindOrAdd(_item_table).ItemIDs.Add(item_id);
	}

	return is_success;
}

void UItemRegistrySubsystem::ClearItemIndex()
{
	_ItemTypeIndexMap.Reset();
	_TableToItemIDs.Reset();
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

	const bool is_auto_register_success = AutoRegisterItemTables();
	const bool is_build_success = BuildItemIndex();

	EDITOR_MESSAGE_LOG(ItemRegistryLog, TEXT("AutoRegister=%s, BuildIndex=%s, RegisteredTables=%d, IndexedItems=%d"),
		is_auto_register_success ? TEXT("Success") : TEXT("Failed"),
		is_build_success ? TEXT("Success") : TEXT("Failed"),
		_RegisteredItemTables.Num(), GetItemCount());

	const bool is_success = is_auto_register_success && is_build_success;

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

	if (IsSupportedItemTable(_item_table) == false)
	{
		EDITOR_MESSAGE_ERROR(ItemRegistryLog, TEXT("Failed : Table '%s' has unsupported RowStruct."), *_item_table->GetName());
		return false;
	}

	EDITOR_MESSAGE_LOG(ItemRegistryLog, TEXT("Refresh Item Table : %s"), *_item_table->GetName());

	// 혹시 아직 등록 안 된 테이블이면 등록
	RegisterItemTable(_item_table);

	// 기존 인덱스에서 이 테이블 소속 항목 제거
	RemoveItemIndexByTable(_item_table);

	// 다시 인덱싱
	const bool is_success = IndexItemTable(_item_table);

	if (is_success)
	{
		EDITOR_MESSAGE_LOG(ItemRegistryLog, TEXT("Success! IndexedItems=%d"), GetItemCount());
		EDITOR_NOTIFY_LOG(TEXT("Refresh Item Registry Success"));
	}
	else
	{
		EDITOR_MESSAGE_LOG(ItemRegistryLog, TEXT("Failed"));
		EDITOR_NOTIFY_ERROR(TEXT("Refresh Item Registry Failed"));
	}

	return is_success;
}

void UItemRegistrySubsystem::RemoveItemIndexByTable(const UDataTable* _item_table)
{
	if (IsInvalid(_item_table))
		return;

	auto item_id_list = _TableToItemIDs.Find(_item_table);
	if (IsInvalid(item_id_list))
		return;

	for (const FItemID& item_id : item_id_list->ItemIDs)
	{
		auto item_type_index_ptr = _ItemTypeIndexMap.Find(item_id.GetType());
		if (IsValid(item_type_index_ptr))
		{
			item_type_index_ptr->ItemIDToRow.Remove(item_id);

			if (item_type_index_ptr->ItemIDToRow.IsEmpty())
			{
				_ItemTypeIndexMap.Remove(item_id.GetType());
			}
		}
	}

	_TableToItemIDs.Remove(_item_table);
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

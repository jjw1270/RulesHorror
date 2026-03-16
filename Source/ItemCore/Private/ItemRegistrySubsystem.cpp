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

	RefreshRegistry();
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

bool UItemRegistrySubsystem::RegisterItemTable(const UDataTable* _item_table)
{
	if (IsInvalid(_item_table))
	{
		EDITOR_MESSAGE_WARNING(ItemRegistryLog, TEXT("_item_table is null."));
		return false;
	}

	if (!IsSupportedItemTable(_item_table))
	{
		EDITOR_MESSAGE_WARNING(ItemRegistryLog, TEXT("Table '%s' has unsupported RowStruct."), *_item_table->GetName());
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
		EDITOR_MESSAGE_WARNING(ItemRegistryLog, TEXT("_item_table is null."));
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
			if (_ItemIndex.Contains(item_id))
			{
				const FItemRowReference* existing_reference = _ItemIndex.Find(item_id);

				EDITOR_MESSAGE_ERROR(ItemRegistryLog, TEXT("중복 ItemID!!(Table=%s, Row=%s)"),
					(existing_reference && existing_reference->DataTable) ? *existing_reference->DataTable->GetName() : TEXT("None"), existing_reference ? *existing_reference->RowName.ToString() : TEXT("None"));

				EDITOR_NOTIFY_ERROR(TEXT("중복 ItemID!! (Table=%s, Row=%s)"),
					(existing_reference && existing_reference->DataTable) ? *existing_reference->DataTable->GetName() : TEXT("None"), existing_reference ? *existing_reference->RowName.ToString() : TEXT("None"));

				is_success = false;
				continue;
			}
		}
		else
		{
			EDITOR_MESSAGE_ERROR(ItemRegistryLog, TEXT("Invalid ItemID : %d, %s \n(Table=%s, Row=%s)"), (int32)item_id, *item_id_validate.Reason,
				*_item_table->GetName(), *row_name.ToString());

			EDITOR_NOTIFY_ERROR(TEXT("Invalid ItemID : %d, %s \n(Table=%s, Row=%s)"), (int32)item_id , *item_id_validate.Reason,
				*_item_table->GetName(), *row_name.ToString());
			
			is_success = false;
			continue;
		}

		FItemRowReference row_reference;
		row_reference.DataTable = _item_table;
		row_reference.RowName = row_name;
		row_reference.RowStruct = row_struct;

		_ItemIndex.Add(item_id, MoveTemp(row_reference));
	}

	return is_success;
}

void UItemRegistrySubsystem::ClearItemIndex()
{
	_ItemIndex.Reset();
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

bool UItemRegistrySubsystem::RefreshRegistry()
{
	ClearRegisteredItemTables();
	ClearItemIndex();

	const bool is_auto_register_success = AutoRegisterItemTables();
	const bool is_build_success = BuildItemIndex();

	EDITOR_MESSAGE_LOG(ItemRegistryLog, TEXT("RefreshRegistry - AutoRegister=%s, BuildIndex=%s, RegisteredTables=%d, IndexedItems=%d"),
		is_auto_register_success ? TEXT("Success") : TEXT("Failed"),
		is_build_success ? TEXT("Success") : TEXT("Failed"),
		_RegisteredItemTables.Num(),
		_ItemIndex.Num());

	const bool is_success = is_auto_register_success && is_build_success;

	if (is_success)
	{
		EDITOR_MESSAGE_LOG(ItemRegistryLog, TEXT("Refresh Item Registry Success!"));
		EDITOR_NOTIFY_LOG(TEXT("Refresh Item Registry Success!"));
	}

	return is_success;
}

bool UItemRegistrySubsystem::RefreshItemTable(const UDataTable* _item_table)
{
	if (IsInvalid(_item_table))
	{
		EDITOR_MESSAGE_WARNING(ItemRegistryLog, TEXT("_item_table is null."));
		return false;
	}

	if (IsSupportedItemTable(_item_table) == false)
	{
		EDITOR_MESSAGE_WARNING(ItemRegistryLog, TEXT("Table '%s' has unsupported RowStruct."), *_item_table->GetName());
		return false;
	}

	// 혹시 아직 등록 안 된 테이블이면 등록
	RegisterItemTable(_item_table);

	// 기존 인덱스에서 이 테이블 소속 항목 제거
	RemoveItemIndexByTable(_item_table);

	// 다시 인덱싱
	const bool is_success = IndexItemTable(_item_table);

	EDITOR_MESSAGE_LOG(ItemRegistryLog, TEXT("RefreshItemTable - Table=%s, Result=%s, IndexedItems=%d"),
		*_item_table->GetName(),
		is_success ? TEXT("Success") : TEXT("Failed"),
		_ItemIndex.Num());

	return is_success;
}

void UItemRegistrySubsystem::RemoveItemIndexByTable(const UDataTable* _item_table)
{
	if (IsInvalid(_item_table))
		return;

	TArray<FItemID> remove_item_ids;
	remove_item_ids.Reserve(_ItemIndex.Num());

	for (const auto& item_pair : _ItemIndex)
	{
		const FItemRowReference& row_reference = item_pair.Value;

		if (row_reference.DataTable == _item_table)
		{
			remove_item_ids.Add(item_pair.Key);
		}
	}

	for (const FItemID& item_id : remove_item_ids)
	{
		_ItemIndex.Remove(item_id);
	}
}

bool UItemRegistrySubsystem::ContainsItemID(const FItemID& _item_id) const
{
	return _ItemIndex.Contains(_item_id);
}

const FItemRowReference* UItemRegistrySubsystem::FindItemRowReference(const FItemID& _item_id) const
{
	return _ItemIndex.Find(_item_id);
}

int32 UItemRegistrySubsystem::GetItemCount() const
{
	return _ItemIndex.Num();
}

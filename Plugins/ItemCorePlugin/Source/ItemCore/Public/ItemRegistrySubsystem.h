// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "ItemTableRow.h"
#include "ItemRegistrySubsystem.generated.h"



// ItemID->DataTable / RowName / RowStruct 참조 정보
USTRUCT()
struct FItemRowReference
{
	GENERATED_BODY()
	
public:
	UPROPERTY(VisibleAnywhere)
	const UDataTable* DataTable = nullptr;

	UPROPERTY(VisibleAnywhere)
	FName RowName = NAME_None;

	// 실제 DataTable 의 RowStruct 
	UPROPERTY(VisibleAnywhere)
	const UScriptStruct* RowStruct = nullptr;

public:
	bool IsValid() const
	{
		return ::IsValid(DataTable)
			&& !RowName.IsNone()
			&& ::IsValid(RowStruct);
	}
};

USTRUCT()
struct FItemTypeIndex
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<FItemID, FItemRowReference> ItemIDToRow;
};

USTRUCT()
struct FItemIDList
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FItemID> ItemIDs;
};

UCLASS()
class ITEMCORE_API UItemRegistrySubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

protected:
	// 인덱싱 대상 테이블
	UPROPERTY(Transient)
	TArray<const UDataTable*> _RegisteredItemTables;

	UPROPERTY(Transient)
	TMap<EItemType, FItemTypeIndex> _ItemTypeIndexMap;

	UPROPERTY(Transient)
	TMap<const UDataTable*, FItemIDList> _TableToItemIDs;

public:
	virtual void Initialize(FSubsystemCollectionBase& _collection) override;
	virtual void Deinitialize() override;

protected:
	bool AutoRegisterItemTables();
	TArray<FString> GetTableSearchPaths() const;

	// _item_table 의 RowStruct 가 FItemTableRow 계열인지 검사
	bool IsSupportedItemTable(const UDataTable* _item_table) const;

	bool RegisterItemTable(const UDataTable* _item_table);
	void ClearRegisteredItemTables();

	bool BuildItemIndex();

	bool IndexItemTable(const UDataTable* _item_table);
	void ClearItemIndex();

	const FItemRowReference* Find(const FItemID& _item_id) const;

public:
	bool RefreshRegistry();
	bool RefreshItemTable(const UDataTable* _item_table);
	void RemoveItemIndexByTable(const UDataTable* _item_table);

public:
	// 타입 안전 Row 조회
	template <typename T = FItemTableRow>
	const T* FindItemRow(const FItemID& _item_id) const
	{
		static_assert(TIsDerivedFrom<T, FItemTableRow>::IsDerived, "T must derive from FItemTableRow.");

		const FItemTypeIndex* item_type_index = _ItemTypeIndexMap.Find(_item_id.GetType());
		if (IsInvalid(item_type_index))
			return nullptr;

		const FItemRowReference* row_reference = item_type_index->ItemIDToRow.Find(_item_id);
		if (IsInvalid(row_reference))
			return nullptr;

		if (IsAnyInvalid(row_reference->RowStruct, row_reference->DataTable))
			return nullptr;

		if (row_reference->RowStruct->IsChildOf(T::StaticStruct()) == false)
			return nullptr;

		const uint8* const* row_data_ptr = row_reference->DataTable->GetRowMap().Find(row_reference->RowName);
		if (IsAnyInvalid(row_data_ptr, *row_data_ptr))
			return nullptr;

		return reinterpret_cast<const T*>(*row_data_ptr);
	}

	template <typename T = FItemTableRow>
	TArray<const T*> GetItemRowsByType(EItemType _item_type) const
	{
		static_assert(TIsDerivedFrom<T, FItemTableRow>::IsDerived, "T must derive from FItemTableRow.");

		TArray<const T*> rows;

		const FItemTypeIndex* item_type_index = _ItemTypeIndexMap.Find(_item_type);
		if (IsInvalid(item_type_index))
			return rows;

		rows.Reserve(item_type_index->ItemIDToRow.Num());

		for (const auto& item_pair : item_type_index->ItemIDToRow)
		{
			const FItemRowReference& row_reference = item_pair.Value;

			if (IsAnyInvalid(row_reference.RowStruct, row_reference.DataTable))
				continue;

			if (row_reference.RowStruct->IsChildOf(T::StaticStruct()) == false)
				continue;

			const uint8* const* row_data_ptr = row_reference.DataTable->GetRowMap().Find(row_reference.RowName);
			if (IsAnyInvalid(row_data_ptr, *row_data_ptr))
				continue;

			rows.Add(reinterpret_cast<const T*>(*row_data_ptr));
		}

		return rows;
	}

	bool Contains(const FItemID& _item_id) const;

	int32 GetItemCount() const;
	int32 GetTypeItemCount(EItemType _item_type) const;
	
};

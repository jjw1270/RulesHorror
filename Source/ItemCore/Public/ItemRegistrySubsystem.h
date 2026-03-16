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

UCLASS()
class ITEMCORE_API UItemRegistrySubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

protected:
	// 인덱싱 대상 테이블
	UPROPERTY(Transient)
	TArray<const UDataTable*> _RegisteredItemTables;

	// ItemID -> Row 위치 정보
	UPROPERTY(Transient)
	TMap<FItemID, FItemRowReference> _ItemIndex;

public:
	virtual void Initialize(FSubsystemCollectionBase& _collection) override;
	virtual void Deinitialize() override;

protected:
	bool AutoRegisterItemTables();
	TArray<FString> GetTableSearchPaths() const;

	bool RegisterItemTable(const UDataTable* _item_table);
	void ClearRegisteredItemTables();

	bool BuildItemIndex();

	bool IndexItemTable(const UDataTable* _item_table);
	void ClearItemIndex();

	// _item_table 의 RowStruct 가 FItemTableRow 계열인지 검사
	bool IsSupportedItemTable(const UDataTable* _item_table) const;

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

		const FItemRowReference* row_reference = _ItemIndex.Find(_item_id);
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

	bool ContainsItemID(const FItemID& _item_id) const;

	const FItemRowReference* FindItemRowReference(const FItemID& _item_id) const;

	int32 GetItemCount() const;
};

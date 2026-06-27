// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemID.h"
#include "ItemTableRow.generated.h"

UENUM(BlueprintType, meta = (ShortToolTip = "아이템 개발/출시 상태"))
enum class EItemDevelopmentState : uint8
{
	NotUsed				UMETA(Tooltip = "사용하지 않음, 샘플 등"),

	Developing			UMETA(Tooltip = "개발 중"),
	Ready					UMETA(Tooltip = "출시 가능"),
	Shipping				UMETA(Tooltip = "Shipping"),
};

USTRUCT(BlueprintType, meta = (ShortToolTip = "Item DataTable 기본 Row"))
struct ITEMCORE_API FItemTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ToolTip = "Registry 조회와 중복 검증에 사용하는 고유 아이템 식별자"))
	FItemID ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ToolTip = "UI, 로그, 디버그 표시 등에 사용할 아이템 이름"))
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (MultiLine = true, ToolTip = "UI/툴팁 등에 사용할 아이템 설명"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ToolTip = "빌드 설정별 사용 가능 여부를 판단하는 개발/출시 상태"))
	EItemDevelopmentState DevState = EItemDevelopmentState::NotUsed;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Item", meta = (MultiLine = true, ToolTip = "에디터에서만 사용하는 제작 메모. 런타임 데이터로 사용하지 않음"))
	FText DevComment;
#endif

public:
	FItemTableRow()
		: FItemTableRow(EItemType::NA)
	{
	}

	FItemTableRow(EItemType _type)
	{
		if (FItemID::IsUsableItemType(_type) == false)
			return;

		if(ItemID == FItemID::Zero)
		{
			ItemID.SetType(_type);
		}

#if WITH_EDITOR
		TableItemType = _type;
#endif
	}

public:
	bool IsUsableItem() const;

	FText GetDisplayName() const;

#if !UE_BUILD_SHIPPING
private:
	static bool DEBUG_ShowItemIDOnDisplayName;

public:
	static void ToggleShowItemIDOnDisplayName()
	{
		DEBUG_ShowItemIDOnDisplayName = !DEBUG_ShowItemIDOnDisplayName;
	}
#endif

#if WITH_EDITORONLY_DATA
private:
	EItemType TableItemType = EItemType::NA;

	TOptional<FItemID> CachedItemID;

	static const FName ItemTableLog;
#endif

public:
#if WITH_EDITOR
	virtual void OnDataTableChanged(const UDataTable* _in_data_table, const FName _in_row_name) override;
	virtual void OnPostDataImport(const UDataTable* _in_data_table, const FName _in_row_name, TArray<FString>& _out_collected_import_problems) override;
	
	virtual EDataValidationResult IsDataValid(FDataValidationContext& _context) const override;

protected:
	void HandleItemIDChanged(const UDataTable* _in_data_table);
#endif
};

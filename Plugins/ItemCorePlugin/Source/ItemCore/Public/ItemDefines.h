// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemDefines.generated.h"


UENUM(BlueprintType, meta = (ShortToolTip = "아이템 대분류"))
enum class EItemType : uint8
{
	NA = 0								UMETA(Hidden),

	Story								UMETA(ToolTip = "스토리"),

	MAX								UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(EItemType, EItemType::MAX);

// 따로 SubType을 연결하지 않으면 이 Enum을 기본으로 사용합니다.
UENUM(BlueprintType, meta = (ShortToolTip = "기본 아이템 세부 분류"))
enum class EDefaultItemSubType : uint8
{
	NA = 0					UMETA(Hidden),
	Default					UMETA(ToolTip = "기본 SubType")
};

USTRUCT()
struct FItemIDValidationResult
{
	GENERATED_BODY()

private:
	bool _IsValid = true;

public:
	FString Reason;

public:
	FItemIDValidationResult() = default;

	FItemIDValidationResult(bool _is_valid, const FString& _reason)
		: _IsValid(_is_valid), Reason(_reason)
	{
	}

	bool IsValid() const
	{
		return _IsValid;
	}

	bool IsInvalid() const
	{
		return !_IsValid;
	}
};

// FItemTableRow 계열 RowStruct를 가진 DataTable 참조.
USTRUCT(BlueprintType, meta = (ShortToolTip = "Item Registry에 등록할 DataTable 참조"))
struct ITEMCORE_API FItemTableReference
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ToolTip = "FItemTableRow 계열 RowStruct를 가진 Item DataTable"))
	TObjectPtr<UDataTable> DataTable = nullptr;

public:
	FItemTableReference() = default;

	explicit FItemTableReference(UDataTable* _data_table)
		: DataTable(_data_table)
	{
	}

	UDataTable* GetDataTable() const
	{
		return DataTable.Get();
	}
};

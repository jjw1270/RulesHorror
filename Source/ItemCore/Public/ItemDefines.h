// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemDefines.generated.h"


UENUM(BlueprintType)
enum class EItemType : uint8
{
	NA = 0								UMETA(Hidden),

	RESERVED						UMETA(ToolTip = "이 값을 지우고 Item Type을 추가하여 사용하세요!"),

	MAX								UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(EItemType, EItemType::MAX);

// 따로 SubType을 연결하지 않으면 이 Enum을 기본으로 사용합니다.
UENUM(BlueprintType)
enum class EDefaultItemSubType : uint8
{
	NA = 0					UMETA(Hidden),
	Default
};

USTRUCT()
struct FItemIDValidationResult
{
	GENERATED_BODY()

public:
	bool _IsValid = true;
	FString _Reason;

public:
	FItemIDValidationResult() = default;

	FItemIDValidationResult(bool _is_valid, const FString& _reason)
		: _IsValid(_is_valid), _Reason(_reason)
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

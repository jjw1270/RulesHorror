// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ItemTableRow.h"
#include "Item/RulesHorrorItemIDs.h"
#include "RulesHorrorItemDefines.generated.h"

USTRUCT(BlueprintType)
struct RULESHORROR_API FStoryTableRow : public FItemTableRow
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString AdditionalSiteAddress = FString();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UWorld> StoryMap;

public:
	FStoryTableRow()
		: FItemTableRow(EItemType::Story)
	{
	}

#if WITH_EDITOR
public:
	virtual EDataValidationResult IsDataValid(FDataValidationContext& _context) const override;
#endif
};

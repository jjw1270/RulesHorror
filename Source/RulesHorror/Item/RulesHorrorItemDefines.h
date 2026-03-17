// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ItemTableRow.h"
#include "RulesHorrorItemDefines.generated.h"

USTRUCT(BlueprintType)
struct RULESHORROR_API FStoryTableRow : public FItemTableRow
{
	GENERATED_BODY()


#if WITH_EDITOR
public:
	virtual EDataValidationResult IsDataValid(FDataValidationContext& _context) const override;
#endif
};

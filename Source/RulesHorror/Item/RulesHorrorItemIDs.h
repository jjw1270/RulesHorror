// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ItemID.h"
#include "RulesHorrorItemIDs.generated.h"

USTRUCT(BlueprintType)
struct RULESHORROR_API FItemID_Story : public FItemID
{
	GENERATED_BODY()

public:
	FItemID_Story() noexcept
		: FItemID(EItemType::Story, 0, 0)
	{
	}

	FItemID_Story(const FItemID& _other)
		: FItemID(_other)
	{
	}
};

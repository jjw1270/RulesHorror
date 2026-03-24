// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ItemHelper.h"
#include "Item/RulesHorrorItemIDs.h"
#include "Item/RulesHorrorItemDefines.h"
#include "RulesHorrorItemHelper.generated.h"

UCLASS()
class RULESHORROR_API URulesHorrorItemHelper : public UItemHelper
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Item")
	static const FStoryTableRow& GetStoryItemRow(FItemID_Story _story_id);

};

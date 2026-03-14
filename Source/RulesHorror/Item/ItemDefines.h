// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Item/ItemID.h"
#include "ItemDefines.generated.h"

USTRUCT(BlueprintType)
struct RULESHORROR_API FItemConfig : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FItemID ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;
};

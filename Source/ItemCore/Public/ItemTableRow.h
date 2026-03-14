// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemID.h"
#include "ItemTableRow.generated.h"

USTRUCT(BlueprintType)
struct ITEMCORE_API FItemTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FItemID ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;
};

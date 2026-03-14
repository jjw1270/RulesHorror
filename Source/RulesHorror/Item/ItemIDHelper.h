// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Item/ItemID.h"
#include "ItemIDHelper.generated.h"

UCLASS()
class RULESHORROR_API UItemIDHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, meta = (BlueprintInternalUseOnly = "true"))
	static FItemID MakeItemID(EItemType _type, uint8 _sub_type, int32 _serial);

	UFUNCTION(BlueprintPure, Category = "ItemID")
	static void BreakItemID(FItemID _item_id, EItemType& _type, uint8& _sub_type, int32& _serial);

	UFUNCTION(BlueprintPure, Category = "ItemID")
	static EItemType GetType(FItemID _item_id);

	UFUNCTION(BlueprintPure, Category = "ItemID")
	static uint8 GetSubType(FItemID _item_id);

	UFUNCTION(BlueprintPure, Category = "ItemID")
	static int32 GetSerial(FItemID _item_id);

	UFUNCTION(BlueprintPure, Category = "ItemID")
	static int32 ToInteger(FItemID _item_id);

	UFUNCTION(BlueprintPure, Category = "ItemID")
	static FString ToString(FItemID _item_id);

	UFUNCTION(BlueprintPure, Category = "ItemID")
	static bool IsValid(FItemID _item_id, FString& _out_reason);
};

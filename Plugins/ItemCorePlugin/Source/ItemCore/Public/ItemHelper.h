// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ItemRegistrySubsystem.h"
#include "ItemHelper.generated.h"

UCLASS()
class ITEMCORE_API UItemHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
protected:
	template <typename T>
	static const T* FindItemRow(FItemID _item_id)
	{
		if (IsInvalid(GEngine))
			return nullptr;

		auto item_registry = GEngine->GetEngineSubsystem<UItemRegistrySubsystem>();
		if (IsInvalid(item_registry))
			return nullptr;

		return item_registry->FindItemRow<T>(_item_id);
	}

public:
	UFUNCTION(BlueprintPure, Category = "Item")
	static const FItemTableRow& GetItemRow(FItemID _item_id);
};

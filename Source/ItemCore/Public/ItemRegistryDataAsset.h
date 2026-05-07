// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemDefines.h"
#include "ItemRegistryDataAsset.generated.h"


UCLASS(BlueprintType)
class ITEMCORE_API UItemRegistryDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TArray<FItemTableReference> _ItemTables;

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};

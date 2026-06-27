// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemDefines.h"
#include "ItemRegistryDataAsset.generated.h"


UCLASS(BlueprintType, meta = (DisplayName = "Item Registry Data Asset"))
class ITEMCORE_API UItemRegistryDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ToolTip = "Item Registry가 인덱싱할 Item DataTable 목록"))
	TArray<FItemTableReference> _ItemTables;

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};

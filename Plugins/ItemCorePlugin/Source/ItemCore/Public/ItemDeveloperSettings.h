// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ItemDeveloperSettings.generated.h"

class UItemRegistryDataAsset;


UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Item Developer Settings"))
class ITEMCORE_API UItemDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Config, Category = "Item", meta = (ToolTip = "Item Registry가 읽을 UItemRegistryDataAsset soft reference"))
	TSoftObjectPtr<UItemRegistryDataAsset> _ItemRegistryDataAsset;
};

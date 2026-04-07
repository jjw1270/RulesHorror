// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ItemDeveloperSettings.generated.h"


UCLASS(Config = Game, DefaultConfig)
class ITEMCORE_API UItemDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Config, Category = "Item", meta = (ContentDir, LongPackageName))
	TArray<FDirectoryPath> _ItemTableSearchPaths;
};

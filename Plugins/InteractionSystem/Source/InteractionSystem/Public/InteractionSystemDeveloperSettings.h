// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InteractionSystemDeveloperSettings.generated.h"


UCLASS(Config = Game, DefaultConfig)
class INTERACTIONSYSTEM_API UInteractionSystemDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Config)
	TSoftObjectPtr<class UMaterialInterface> _OverlayMaterialClass = nullptr;
};

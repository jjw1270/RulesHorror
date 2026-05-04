// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "StoryFlowDefines.h"
#include "RulesHorrorDeveloperSettings.generated.h"


UCLASS(Config = Game, DefaultConfig)
class RULESHORROR_API URulesHorrorDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Config)
	FStorySceneID _IntroSceneID;

	UPROPERTY(EditAnywhere, Config)
	FStorySceneID _StoryStartSceneID;
	
};

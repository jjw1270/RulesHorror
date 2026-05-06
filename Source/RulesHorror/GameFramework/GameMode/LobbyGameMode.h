// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode/RulesHorrorGameMode.h"
#include "LobbyGameMode.generated.h"

/*
* 
 */
UCLASS(abstract)
class ALobbyGameMode : public ARulesHorrorGameMode
{
	GENERATED_BODY()

public:
	ALobbyGameMode();

protected:
	virtual void StartPlay() override;
};

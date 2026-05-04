// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController/RulesHorrorPlayerController.h"
#include "LobbyPlayerController.generated.h"

UCLASS(abstract)
class RULESHORROR_API ALobbyPlayerController : public ARulesHorrorPlayerController
{
	GENERATED_BODY()

public:
	ALobbyPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float _delta_time) override;
	
};

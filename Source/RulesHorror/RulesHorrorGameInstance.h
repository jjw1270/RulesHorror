// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "RulesHorrorGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class RULESHORROR_API URulesHorrorGameInstance : public UGameInstance
{
	GENERATED_BODY()

protected:
	virtual void Shutdown() override;
	
public:
	UFUNCTION(BlueprintCallable)
	void QuitGame();

	UFUNCTION(BlueprintCallable)
	void PauseGame(bool _is_pause);
};

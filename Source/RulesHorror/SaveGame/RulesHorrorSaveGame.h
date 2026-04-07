// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CustomSaveGame.h"
#include "RulesHorrorSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class RULESHORROR_API URulesHorrorSaveGame : public UCustomSaveGame
{
	GENERATED_BODY()

public:
	virtual void ClearData() override;
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WidgetPlayerController.h"
#include "LobbyPlayerController.generated.h"

UCLASS(abstract)
class RULESHORROR_API ALobbyPlayerController : public AWidgetPlayerController
{
	GENERATED_BODY()

public:
	ALobbyPlayerController();

protected:
	virtual void BeginPlay() override;

};

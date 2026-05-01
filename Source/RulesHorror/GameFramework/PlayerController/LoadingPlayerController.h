// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController/RulesHorrorPlayerController.h"
#include "LoadingPlayerController.generated.h"

UCLASS(abstract)
class RULESHORROR_API ALoadingPlayerController : public ARulesHorrorPlayerController
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UUI_Loading> _LoadingWidgetClass = nullptr;

protected:
	virtual void BeginPlay() override;

};

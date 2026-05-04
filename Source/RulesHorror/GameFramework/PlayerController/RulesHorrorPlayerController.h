// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "WidgetPlayerController.h"
#include "RulesHorrorPlayerController.generated.h"


/**
 *  Manages the input mapping context.
 *  Overrides the Player Camera Manager class.
 */
UCLASS(abstract)
class RULESHORROR_API ARulesHorrorPlayerController : public AWidgetPlayerController
{
	GENERATED_BODY()

public:
	ARulesHorrorPlayerController();

protected:
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<class UInputMappingContext*> DefaultMappingContexts;

	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;

};

// Copyright Epic Games, Inc. All Rights Reserved.

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
	/** Constructor */
	ARulesHorrorPlayerController();

protected:
	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<class UInputMappingContext*> DefaultMappingContexts;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Possessed pawn initialization */
	virtual void OnPossess(APawn* aPawn) override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

};

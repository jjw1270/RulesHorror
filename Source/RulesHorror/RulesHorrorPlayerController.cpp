// Copyright Epic Games, Inc. All Rights Reserved.


#include "RulesHorrorPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "RulesHorrorCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "RulesHorrorCharacter.h"

ARulesHorrorPlayerController::ARulesHorrorPlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = ARulesHorrorCameraManager::StaticClass();
}

void ARulesHorrorPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void ARulesHorrorPlayerController::OnPossess(APawn* aPawn)
{
	Super::OnPossess(aPawn);

	// only spawn UI on local player controllers
	if (IsLocalPlayerController())
	{
		// set up the UI for the character
		if (ARulesHorrorCharacter* RulesHorrorCharacter = Cast<ARulesHorrorCharacter>(aPawn))
		{
			// create the UI
			/*if (!HorrorUI)
			{
				HorrorUI = CreateWidget<UHorrorUI>(this, HorrorUIClass);
				HorrorUI->AddToViewport(0);
			}

			HorrorUI->SetupCharacter(HorrorCharacter);*/
		}
	}
}

void ARulesHorrorPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}
		}
	}
}

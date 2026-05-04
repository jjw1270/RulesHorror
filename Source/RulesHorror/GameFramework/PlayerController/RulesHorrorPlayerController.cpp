// Copyright (c) 2026 장윤제. All rights reserved.


#include "RulesHorrorPlayerController.h"
#include "RulesHorrorUtils.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "GameFramework/RulesHorrorCameraManager.h"
#include "GameFramework/Pawn/RulesHorrorCharacter.h"

ARulesHorrorPlayerController::ARulesHorrorPlayerController()
{
	PlayerCameraManagerClass = ARulesHorrorCameraManager::StaticClass();
}

void ARulesHorrorPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void ARulesHorrorPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (IsLocalPlayerController() == false)
		return;

	auto enhanced_input_local_player_subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (IsValid(enhanced_input_local_player_subsystem))
	{
		for (UInputMappingContext* current_context : DefaultMappingContexts)
		{
			enhanced_input_local_player_subsystem->AddMappingContext(current_context, 0);
		}
	}
}

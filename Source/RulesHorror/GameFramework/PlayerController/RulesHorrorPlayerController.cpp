// Copyright (c) 2026 장윤제. All rights reserved.


#include "RulesHorrorPlayerController.h"
#include "RulesHorrorUtils.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "GameFramework/RulesHorrorCameraManager.h"
#include "GameFramework/Pawn/RulesHorrorCharacter.h"
#include "SaveGameSubsystem.h"

ARulesHorrorPlayerController::ARulesHorrorPlayerController()
{
	PlayerCameraManagerClass = ARulesHorrorCameraManager::StaticClass();
}

void ARulesHorrorPlayerController::BeginPlay()
{
	Super::BeginPlay();

	auto save_game_subsystem = URulesHorrorUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(this);
	if (IsValid(save_game_subsystem))
	{
		bool load_game_success = save_game_subsystem->LoadGame();

#if WITH_EDITOR
		if (load_game_success == false && _IsLobby == false)
		{
			// 원활한 테스트를 위해 로비를 통하지 않고 NewGame
			save_game_subsystem->NewGame();
		}
	}
#endif
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

// Copyright (c) 2026 장윤제. All rights reserved.


#include "LobbyPlayerController.h"
#include "RulesHorrorUtils.h"
#include "SaveGameSubsystem.h"

ALobbyPlayerController::ALobbyPlayerController()
{
}

void ALobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	auto save_game_subsystem = URulesHorrorUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(this);
	if (IsInvalid(save_game_subsystem))
		return;

	save_game_subsystem->LoadGame();
}

void ALobbyPlayerController::Tick(float _delta_time)
{
	Super::Tick(_delta_time);
}

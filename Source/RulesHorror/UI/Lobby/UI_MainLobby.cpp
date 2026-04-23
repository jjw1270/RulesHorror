// Copyright (c) 2026 장윤제. All rights reserved.


#include "UI_MainLobby.h"
#include "RulesHorrorUtils.h"
#include "SaveGameSubsystem.h"

void UUI_MainLobby::NativeConstruct()
{
	Super::NativeConstruct();

	const auto save_game = USaveGameHelper::GetSaveGame_ReadOnly(this);
	SetEnableLoadGame(IsValid(save_game));
}

void UUI_MainLobby::OnClick_NewGame()
{
	auto save_game_subsystem = URulesHorrorUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(this);
	if (IsInvalid(save_game_subsystem))
		return;

	save_game_subsystem->NewGame();

	// todo : map jump
}

void UUI_MainLobby::OnClick_LoadGame()
{
	// todo : map jump
}

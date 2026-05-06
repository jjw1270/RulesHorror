// Copyright (c) 2026 장윤제. All rights reserved.


#include "UI_MainLobby.h"
#include "RulesHorrorUtils.h"
#include "RulesHorrorGameInstance.h"
#include "SaveGameSubsystem.h"

void UUI_MainLobby::NativeConstruct()
{
	Super::NativeConstruct();

	auto save_game_subsystem = URulesHorrorUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(this);
	if (IsValid(save_game_subsystem))
	{
		save_game_subsystem->_OnAsyncLoadGameFinished.AddDynamic(this, &UUI_MainLobby::OnAsyncLoadGameFinished);
	}
}

void UUI_MainLobby::NativeDestruct()
{
	auto save_game_subsystem = URulesHorrorUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(this);
	if (IsValid(save_game_subsystem))
	{
		save_game_subsystem->_OnAsyncLoadGameFinished.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UUI_MainLobby::OnClick_StartNewGame()
{
	auto game_inst = GetGameInstance<URulesHorrorGameInstance>();
	if (IsInvalid(game_inst))
		return;

	game_inst->StartNewGame();
}

void UUI_MainLobby::OnClick_StartSavedGame()
{
	auto game_inst = GetGameInstance<URulesHorrorGameInstance>();
	if (IsInvalid(game_inst))
		return;

	game_inst->StartSavedGame();
}

void UUI_MainLobby::OnAsyncLoadGameFinished(bool _is_success)
{
	auto game_inst = GetGameInstance<URulesHorrorGameInstance>();
	if (IsInvalid(game_inst))
		return;

	SetCanStartSavedGame(game_inst->CanStartSavedGame());
}

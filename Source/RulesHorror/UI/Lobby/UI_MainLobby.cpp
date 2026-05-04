// Copyright (c) 2026 장윤제. All rights reserved.


#include "UI_MainLobby.h"
#include "RulesHorrorUtils.h"
#include "RulesHorrorGameInstance.h"

void UUI_MainLobby::NativeConstruct()
{
	Super::NativeConstruct();

	auto game_inst = GetGameInstance<URulesHorrorGameInstance>();
	if (IsInvalid(game_inst))
		return;

	SetCanStartSavedGame(game_inst->CanStartSavedGame());
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

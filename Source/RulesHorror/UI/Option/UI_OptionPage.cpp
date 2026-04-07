// Copyright (c) 2026 장윤제. All rights reserved.


#include "UI_OptionPage.h"
#include "RulesHorrorGameInstance.h"

void UUI_OptionPage::NativeConstruct()
{
	Super::NativeConstruct();

	auto game_inst = GetGameInstance<URulesHorrorGameInstance>();
	if (IsValid(game_inst))
	{
		game_inst->PauseGame(true);
	}
}

void UUI_OptionPage::NativeDestruct()
{
	auto game_inst = GetGameInstance<URulesHorrorGameInstance>();
	if (IsValid(game_inst))
	{
		game_inst->PauseGame(false);
	}

	Super::NativeDestruct();
}

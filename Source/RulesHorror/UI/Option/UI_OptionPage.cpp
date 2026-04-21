// Copyright (c) 2026 장윤제. All rights reserved.


#include "UI_OptionPage.h"
#include "RulesHorrorGameInstance.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

void UUI_OptionPage::OnShow_Implementation()
{
	Super::OnShow_Implementation();

	auto game_inst = GetGameInstance<URulesHorrorGameInstance>();
	if (IsValid(game_inst))
	{
		game_inst->PauseGame(true);
	}
}

void UUI_OptionPage::OnHide_Implementation(EWidgetHideType _hide_type)
{
	auto game_inst = GetGameInstance<URulesHorrorGameInstance>();
	if (IsValid(game_inst))
	{
		game_inst->PauseGame(false);
	}

	UWidgetBlueprintLibrary::SetFocusToGameViewport();

	Super::OnHide_Implementation(_hide_type);
}

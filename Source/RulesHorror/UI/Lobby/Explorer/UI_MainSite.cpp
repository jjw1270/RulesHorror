// Fill out your copyright notice in the Description page of Project Settings.


#include "UI_MainSite.h"
#include "SaveGame/SaveGameSubsystem.h"

void UUI_MainSite::NativeConstruct()
{
	Super::NativeConstruct();

	const auto save_game = USaveGameHelper::GetSaveGame(this);
	if (IsValid(save_game))
	{
		SetNickName(FText::FromString(save_game->GetNickName()));
	}
}

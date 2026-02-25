// Fill out your copyright notice in the Description page of Project Settings.


#include "SaveGame/SaveGameSubsystem.h"
#include "RulesHorrorUtils.h"
#include "Kismet/GameplayStatics.h"

void USaveGameSubsystem::Initialize(FSubsystemCollectionBase& _collection)
{
	Super::Initialize(_collection);

	_SaveGame = Cast<URulesHorrorSaveGame>(UGameplayStatics::LoadGameFromSlot(_SaveGameSlotName, 0));

	if (IsInvalid(_SaveGame))
	{
		_SaveGame = Cast<URulesHorrorSaveGame>(UGameplayStatics::CreateSaveGameObject(URulesHorrorSaveGame::StaticClass()));
	}

	if (IsInvalid(_SaveGame))
	{
		TRACE_ERROR(TEXT("SaveGame 초기화 실패"));
		return;
	}
}

void USaveGameSubsystem::SaveGameToSlot()
{
	if (IsInvalid(_SaveGame))
	{
		TRACE_ERROR(TEXT("SaveGame Is Invalid"));
		return;
	}

	UGameplayStatics::SaveGameToSlot(_SaveGame, _SaveGameSlotName, 0);
}

URulesHorrorSaveGame* USaveGameHelper::GetSaveGame_Editable(const UObject* _world_ctx)
{
	auto save_game_subsys = URulesHorrorUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(_world_ctx);
	if (IsValid(save_game_subsys))
	{
		return save_game_subsys->_SaveGame;
	}

	return nullptr;
}

const URulesHorrorSaveGame* USaveGameHelper::GetSaveGame(const UObject* _world_ctx)
{
	return GetSaveGame_Editable(_world_ctx);
}

void USaveGameHelper::SaveGame(const UObject* _world_ctx)
{
	auto save_game_subsys = URulesHorrorUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(_world_ctx);
	if (IsInvalid(save_game_subsys))
		return;

	save_game_subsys->SaveGameToSlot();
}

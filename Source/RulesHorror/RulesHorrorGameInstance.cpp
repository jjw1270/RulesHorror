// Copyright (c) 2026 장윤제. All rights reserved.


#include "RulesHorrorGameInstance.h"
#include "RulesHorrorUtils.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "RulesHorrorDeveloperSettings.h"
#include "SaveGameSubsystem.h"
#include "SaveGame/RulesHorrorSaveGame.h"
#include "StoryFlowSubsystem.h"

#if WITH_EDITOR
#include "UI/Office/UI_Monitor.h"
#endif

namespace
{
	constexpr int32 RulesHorrorSaveGameSlotIndex = 0;
}

void URulesHorrorGameInstance::Shutdown()
{
#if WITH_EDITOR
	UUI_Monitor::ResetLastActiveWidgetindex();
#endif

	Super::Shutdown();
}

void URulesHorrorGameInstance::QuitGame()
{
	auto pc = URulesHorrorUtils::GetLocalPlayerController(this);
	if (IsInvalid(pc))
		return;

	UKismetSystemLibrary::QuitGame(this, pc, EQuitPreference::Quit, false);
}

void URulesHorrorGameInstance::PauseGame(bool _is_pause)
{
	auto pc = URulesHorrorUtils::GetLocalPlayerController(this);
	if (IsInvalid(pc))
		return;	

	pc->SetPause(_is_pause);

	// UGameplayStatics::SetGlobalTimeDilation(GetWorld(), _is_pause ? 0.0f : 1.0f);
}

bool URulesHorrorGameInstance::CanStartSavedGame() const
{
	const auto save_game = Cast<URulesHorrorSaveGame>(USaveGameHelper::GetSaveGameFromSlot_ReadOnly(this, RulesHorrorSaveGameSlotIndex));
	if (IsValid(save_game))
	{
		const auto& saved_story_flow_ref = save_game->GetStoryFlowRef();
		return saved_story_flow_ref.IsValid();
	}

	return false;
}

void URulesHorrorGameInstance::StartNewGame()
{
	auto save_game_subsystem = GetSubsystem<USaveGameSubsystem>();
	if (IsInvalid(save_game_subsystem))
	{
		TRACE_ERROR(TEXT("save game subsystem invalid."));
		return;
	}

	auto story_flow_subsystem = GetSubsystem<UStoryFlowSubsystem>();
	if (IsInvalid(story_flow_subsystem))
	{
		TRACE_ERROR(TEXT("story flow subsystem invalid."));
		return;
	}

	const bool save_game_slot_ready = save_game_subsystem->IsValidSaveGameSlotIndex(RulesHorrorSaveGameSlotIndex)
		? save_game_subsystem->ResetGameSlot(RulesHorrorSaveGameSlotIndex)
		: save_game_subsystem->CreateNewGameSlot(RulesHorrorSaveGameSlotIndex);
	if (save_game_slot_ready == false)
	{
		TRACE_ERROR(TEXT("ready save game slot failed."));
		return;
	}

	auto dev_setting = GetDefault<URulesHorrorDeveloperSettings>();
	if (IsInvalid(dev_setting))
	{
		TRACE_ERROR(TEXT("Rules Horror dev setting invalid."));
		return;
	}

	if (dev_setting->_StoryStartSceneID.IsValid() == false)
	{
		TRACE_ERROR(TEXT("Rules Horror dev setting _StoryStartSceneID is empty."));
		return;
	}

	story_flow_subsystem->StartFromScene(dev_setting->_StoryStartSceneID);

	auto save_game = Cast<URulesHorrorSaveGame>(save_game_subsystem->GetCurrentSaveGame());
	if (IsInvalid(save_game))
	{
		TRACE_ERROR(TEXT("save game invalid."));
		return;
	}

	save_game->SaveStoryFlowRef(story_flow_subsystem->GetPendingStartRef());
	save_game_subsystem->SaveGameSlot(RulesHorrorSaveGameSlotIndex);
}

void URulesHorrorGameInstance::StartSavedGame()
{
	auto save_game_subsystem = GetSubsystem<USaveGameSubsystem>();
	if (IsInvalid(save_game_subsystem))
	{
		TRACE_ERROR(TEXT("save game subsystem invalid."));
		return;
	}

	if (save_game_subsystem->LoadGameSlot(RulesHorrorSaveGameSlotIndex) == false)
	{
		TRACE_ERROR(TEXT("load save game slot failed."));
		return;
	}

	const auto save_game = Cast<URulesHorrorSaveGame>(save_game_subsystem->GetCurrentSaveGame());
	if (IsInvalid(save_game))
	{
		TRACE_ERROR(TEXT("save game invalid."));
		return;
	}

	auto story_flow_subsystem = URulesHorrorUtils::GetGameInstanceSubsystem<UStoryFlowSubsystem>(this);
	if (IsInvalid(story_flow_subsystem))
	{
		TRACE_ERROR(TEXT("story flow subsystem invalid."));
		return;
	}

	story_flow_subsystem->StartFromRef(save_game->GetStoryFlowRef());
}
